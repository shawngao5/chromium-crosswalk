// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/form_autofill_util.h"

#include <map>
#include <set>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/memory/scoped_vector.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/common/autofill_data_validation.h"
#include "components/autofill/core/common/autofill_switches.h"
#include "components/autofill/core/common/form_data.h"
#include "components/autofill/core/common/form_field_data.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/platform/WebVector.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebElement.h"
#include "third_party/WebKit/public/web/WebElementCollection.h"
#include "third_party/WebKit/public/web/WebFormControlElement.h"
#include "third_party/WebKit/public/web/WebFormElement.h"
#include "third_party/WebKit/public/web/WebInputElement.h"
#include "third_party/WebKit/public/web/WebLabelElement.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebNode.h"
#include "third_party/WebKit/public/web/WebNodeList.h"
#include "third_party/WebKit/public/web/WebOptionElement.h"
#include "third_party/WebKit/public/web/WebSelectElement.h"
#include "third_party/WebKit/public/web/WebTextAreaElement.h"

using blink::WebDocument;
using blink::WebElement;
using blink::WebElementCollection;
using blink::WebFormControlElement;
using blink::WebFormElement;
using blink::WebFrame;
using blink::WebInputElement;
using blink::WebLabelElement;
using blink::WebNode;
using blink::WebNodeList;
using blink::WebOptionElement;
using blink::WebSelectElement;
using blink::WebTextAreaElement;
using blink::WebString;
using blink::WebVector;

namespace autofill {
namespace {

// A bit field mask for FillForm functions to not fill some fields.
enum FieldFilterMask {
  FILTER_NONE                      = 0,
  FILTER_DISABLED_ELEMENTS         = 1 << 0,
  FILTER_READONLY_ELEMENTS         = 1 << 1,
  FILTER_NON_FOCUSABLE_ELEMENTS    = 1 << 2,
  FILTER_ALL_NON_EDITABLE_ELEMENTS = FILTER_DISABLED_ELEMENTS |
                                     FILTER_READONLY_ELEMENTS |
                                     FILTER_NON_FOCUSABLE_ELEMENTS,
};

RequirementsMask ExtractionRequirements() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
             switches::kRespectAutocompleteOffForAutofill)
             ? REQUIRE_AUTOCOMPLETE
             : REQUIRE_NONE;
}

void TruncateString(base::string16* str, size_t max_length) {
  if (str->length() > max_length)
    str->resize(max_length);
}

bool IsOptionElement(const WebElement& element) {
  CR_DEFINE_STATIC_LOCAL(WebString, kOption, ("option"));
  return element.hasHTMLTagName(kOption);
}

bool IsScriptElement(const WebElement& element) {
  CR_DEFINE_STATIC_LOCAL(WebString, kScript, ("script"));
  return element.hasHTMLTagName(kScript);
}

bool IsNoScriptElement(const WebElement& element) {
  CR_DEFINE_STATIC_LOCAL(WebString, kNoScript, ("noscript"));
  return element.hasHTMLTagName(kNoScript);
}

bool HasTagName(const WebNode& node, const blink::WebString& tag) {
  return node.isElementNode() && node.toConst<WebElement>().hasHTMLTagName(tag);
}

bool IsAutofillableElement(const WebFormControlElement& element) {
  const WebInputElement* input_element = toWebInputElement(&element);
  return IsAutofillableInputElement(input_element) ||
         IsSelectElement(element) ||
         IsTextAreaElement(element);
}

bool IsElementInControlElementSet(
    const WebElement& element,
    const std::vector<WebFormControlElement>& control_elements) {
  if (!element.isFormControlElement())
    return false;
  const WebFormControlElement form_control_element =
      element.toConst<WebFormControlElement>();
  return std::find(control_elements.begin(),
                   control_elements.end(),
                   form_control_element) != control_elements.end();
}

bool IsElementInsideFormOrFieldSet(const WebElement& element) {
  for (WebNode parent_node = element.parentNode();
       !parent_node.isNull();
       parent_node = parent_node.parentNode()) {
    if (!parent_node.isElementNode())
      continue;

    WebElement cur_element = parent_node.to<WebElement>();
    if (cur_element.hasHTMLTagName("form") ||
        cur_element.hasHTMLTagName("fieldset")) {
      return true;
    }
  }
  return false;
}

// Check whether the given field satisfies the REQUIRE_AUTOCOMPLETE requirement.
bool SatisfiesRequireAutocomplete(const WebInputElement& input_element) {
  return input_element.autoComplete();
}

// Appends |suffix| to |prefix| so that any intermediary whitespace is collapsed
// to a single space.  If |force_whitespace| is true, then the resulting string
// is guaranteed to have a space between |prefix| and |suffix|.  Otherwise, the
// result includes a space only if |prefix| has trailing whitespace or |suffix|
// has leading whitespace.
// A few examples:
//  * CombineAndCollapseWhitespace("foo", "bar", false)       -> "foobar"
//  * CombineAndCollapseWhitespace("foo", "bar", true)        -> "foo bar"
//  * CombineAndCollapseWhitespace("foo ", "bar", false)      -> "foo bar"
//  * CombineAndCollapseWhitespace("foo", " bar", false)      -> "foo bar"
//  * CombineAndCollapseWhitespace("foo", " bar", true)       -> "foo bar"
//  * CombineAndCollapseWhitespace("foo   ", "   bar", false) -> "foo bar"
//  * CombineAndCollapseWhitespace(" foo", "bar ", false)     -> " foobar "
//  * CombineAndCollapseWhitespace(" foo", "bar ", true)      -> " foo bar "
const base::string16 CombineAndCollapseWhitespace(
    const base::string16& prefix,
    const base::string16& suffix,
    bool force_whitespace) {
  base::string16 prefix_trimmed;
  base::TrimPositions prefix_trailing_whitespace =
      base::TrimWhitespace(prefix, base::TRIM_TRAILING, &prefix_trimmed);

  // Recursively compute the children's text.
  base::string16 suffix_trimmed;
  base::TrimPositions suffix_leading_whitespace =
      base::TrimWhitespace(suffix, base::TRIM_LEADING, &suffix_trimmed);

  if (prefix_trailing_whitespace || suffix_leading_whitespace ||
      force_whitespace) {
    return prefix_trimmed + base::ASCIIToUTF16(" ") + suffix_trimmed;
  } else {
    return prefix_trimmed + suffix_trimmed;
  }
}

// This is a helper function for the FindChildText() function (see below).
// Search depth is limited with the |depth| parameter.
// |divs_to_skip| is a list of <div> tags to ignore if encountered.
base::string16 FindChildTextInner(const WebNode& node,
                                  int depth,
                                  const std::set<WebNode>& divs_to_skip) {
  if (depth <= 0 || node.isNull())
    return base::string16();

  // Skip over comments.
  if (node.nodeType() == WebNode::CommentNode)
    return FindChildTextInner(node.nextSibling(), depth - 1, divs_to_skip);

  if (node.nodeType() != WebNode::ElementNode &&
      node.nodeType() != WebNode::TextNode)
    return base::string16();

  // Ignore elements known not to contain inferable labels.
  if (node.isElementNode()) {
    const WebElement element = node.toConst<WebElement>();
    if (IsOptionElement(element) ||
        IsScriptElement(element) ||
        IsNoScriptElement(element) ||
        (element.isFormControlElement() &&
         IsAutofillableElement(element.toConst<WebFormControlElement>()))) {
      return base::string16();
    }

    if (element.hasHTMLTagName("div") && ContainsKey(divs_to_skip, node))
      return base::string16();
  }

  // Extract the text exactly at this node.
  base::string16 node_text = node.nodeValue();

  // Recursively compute the children's text.
  // Preserve inter-element whitespace separation.
  base::string16 child_text =
      FindChildTextInner(node.firstChild(), depth - 1, divs_to_skip);
  bool add_space = node.nodeType() == WebNode::TextNode && node_text.empty();
  node_text = CombineAndCollapseWhitespace(node_text, child_text, add_space);

  // Recursively compute the siblings' text.
  // Again, preserve inter-element whitespace separation.
  base::string16 sibling_text =
      FindChildTextInner(node.nextSibling(), depth - 1, divs_to_skip);
  add_space = node.nodeType() == WebNode::TextNode && node_text.empty();
  node_text = CombineAndCollapseWhitespace(node_text, sibling_text, add_space);

  return node_text;
}

// Same as FindChildText() below, but with a list of div nodes to skip.
// TODO(thestig): See if other FindChildText() callers can benefit from this.
base::string16 FindChildTextWithIgnoreList(
    const WebNode& node,
    const std::set<WebNode>& divs_to_skip) {
  if (node.isTextNode())
    return node.nodeValue();

  WebNode child = node.firstChild();

  const int kChildSearchDepth = 10;
  base::string16 node_text =
      FindChildTextInner(child, kChildSearchDepth, divs_to_skip);
  base::TrimWhitespace(node_text, base::TRIM_ALL, &node_text);
  return node_text;
}

// Returns the aggregated values of the descendants of |element| that are
// non-empty text nodes.  This is a faster alternative to |innerText()| for
// performance critical operations.  It does a full depth-first search so can be
// used when the structure is not directly known.  However, unlike with
// |innerText()|, the search depth and breadth are limited to a fixed threshold.
// Whitespace is trimmed from text accumulated at descendant nodes.
base::string16 FindChildText(const WebNode& node) {
  return FindChildTextWithIgnoreList(node, std::set<WebNode>());
}

// Shared function for InferLabelFromPrevious() and InferLabelFromNext().
base::string16 InferLabelFromSibling(const WebFormControlElement& element,
                                     bool forward) {
  base::string16 inferred_label;
  WebNode sibling = element;
  while (true) {
    sibling = forward ? sibling.nextSibling() : sibling.previousSibling();
    if (sibling.isNull())
      break;

    // Skip over comments.
    WebNode::NodeType node_type = sibling.nodeType();
    if (node_type == WebNode::CommentNode)
      continue;

    // Otherwise, only consider normal HTML elements and their contents.
    if (node_type != WebNode::TextNode &&
        node_type != WebNode::ElementNode)
      break;

    // A label might be split across multiple "lightweight" nodes.
    // Coalesce any text contained in multiple consecutive
    //  (a) plain text nodes or
    //  (b) inline HTML elements that are essentially equivalent to text nodes.
    CR_DEFINE_STATIC_LOCAL(WebString, kBold, ("b"));
    CR_DEFINE_STATIC_LOCAL(WebString, kStrong, ("strong"));
    CR_DEFINE_STATIC_LOCAL(WebString, kSpan, ("span"));
    CR_DEFINE_STATIC_LOCAL(WebString, kFont, ("font"));
    if (sibling.isTextNode() ||
        HasTagName(sibling, kBold) || HasTagName(sibling, kStrong) ||
        HasTagName(sibling, kSpan) || HasTagName(sibling, kFont)) {
      base::string16 value = FindChildText(sibling);
      // A text node's value will be empty if it is for a line break.
      bool add_space = sibling.isTextNode() && value.empty();
      inferred_label =
          CombineAndCollapseWhitespace(value, inferred_label, add_space);
      continue;
    }

    // If we have identified a partial label and have reached a non-lightweight
    // element, consider the label to be complete.
    base::string16 trimmed_label;
    base::TrimWhitespace(inferred_label, base::TRIM_ALL, &trimmed_label);
    if (!trimmed_label.empty())
      break;

    // <img> and <br> tags often appear between the input element and its
    // label text, so skip over them.
    CR_DEFINE_STATIC_LOCAL(WebString, kImage, ("img"));
    CR_DEFINE_STATIC_LOCAL(WebString, kBreak, ("br"));
    if (HasTagName(sibling, kImage) || HasTagName(sibling, kBreak))
      continue;

    // We only expect <p> and <label> tags to contain the full label text.
    CR_DEFINE_STATIC_LOCAL(WebString, kPage, ("p"));
    CR_DEFINE_STATIC_LOCAL(WebString, kLabel, ("label"));
    if (HasTagName(sibling, kPage) || HasTagName(sibling, kLabel))
      inferred_label = FindChildText(sibling);

    break;
  }

  base::TrimWhitespace(inferred_label, base::TRIM_ALL, &inferred_label);
  return inferred_label;
}

// Helper for |InferLabelForElement()| that infers a label, if possible, from
// a previous sibling of |element|,
// e.g. Some Text <input ...>
// or   Some <span>Text</span> <input ...>
// or   <p>Some Text</p><input ...>
// or   <label>Some Text</label> <input ...>
// or   Some Text <img><input ...>
// or   <b>Some Text</b><br/> <input ...>.
base::string16 InferLabelFromPrevious(const WebFormControlElement& element) {
  return InferLabelFromSibling(element, false /* forward? */);
}

// Same as InferLabelFromPrevious(), but in the other direction.
// Useful for cases like: <span><input type="checkbox">Label For Checkbox</span>
base::string16 InferLabelFromNext(const WebFormControlElement& element) {
  return InferLabelFromSibling(element, true /* forward? */);
}

// Helper for |InferLabelForElement()| that infers a label, if possible, from
// placeholder text,
base::string16 InferLabelFromPlaceholder(const WebFormControlElement& element) {
  CR_DEFINE_STATIC_LOCAL(WebString, kPlaceholder, ("placeholder"));
  if (element.hasAttribute(kPlaceholder))
    return element.getAttribute(kPlaceholder);

  return base::string16();
}

// Helper for |InferLabelForElement()| that infers a label, if possible, from
// enclosing list item,
// e.g. <li>Some Text<input ...><input ...><input ...></tr>
base::string16 InferLabelFromListItem(const WebFormControlElement& element) {
  WebNode parent = element.parentNode();
  CR_DEFINE_STATIC_LOCAL(WebString, kListItem, ("li"));
  while (!parent.isNull() && parent.isElementNode() &&
         !parent.to<WebElement>().hasHTMLTagName(kListItem)) {
    parent = parent.parentNode();
  }

  if (!parent.isNull() && HasTagName(parent, kListItem))
    return FindChildText(parent);

  return base::string16();
}

// Helper for |InferLabelForElement()| that infers a label, if possible, from
// surrounding table structure,
// e.g. <tr><td>Some Text</td><td><input ...></td></tr>
// or   <tr><th>Some Text</th><td><input ...></td></tr>
// or   <tr><td><b>Some Text</b></td><td><b><input ...></b></td></tr>
// or   <tr><th><b>Some Text</b></th><td><b><input ...></b></td></tr>
base::string16 InferLabelFromTableColumn(const WebFormControlElement& element) {
  CR_DEFINE_STATIC_LOCAL(WebString, kTableCell, ("td"));
  WebNode parent = element.parentNode();
  while (!parent.isNull() && parent.isElementNode() &&
         !parent.to<WebElement>().hasHTMLTagName(kTableCell)) {
    parent = parent.parentNode();
  }

  if (parent.isNull())
    return base::string16();

  // Check all previous siblings, skipping non-element nodes, until we find a
  // non-empty text block.
  base::string16 inferred_label;
  WebNode previous = parent.previousSibling();
  CR_DEFINE_STATIC_LOCAL(WebString, kTableHeader, ("th"));
  while (inferred_label.empty() && !previous.isNull()) {
    if (HasTagName(previous, kTableCell) || HasTagName(previous, kTableHeader))
      inferred_label = FindChildText(previous);

    previous = previous.previousSibling();
  }

  return inferred_label;
}

// Helper for |InferLabelForElement()| that infers a label, if possible, from
// surrounding table structure,
// e.g. <tr><td>Some Text</td></tr><tr><td><input ...></td></tr>
base::string16 InferLabelFromTableRow(const WebFormControlElement& element) {
  CR_DEFINE_STATIC_LOCAL(WebString, kTableRow, ("tr"));
  WebNode parent = element.parentNode();
  while (!parent.isNull() && parent.isElementNode() &&
         !parent.to<WebElement>().hasHTMLTagName(kTableRow)) {
    parent = parent.parentNode();
  }

  if (parent.isNull())
    return base::string16();

  // Check all previous siblings, skipping non-element nodes, until we find a
  // non-empty text block.
  base::string16 inferred_label;
  WebNode previous = parent.previousSibling();
  while (inferred_label.empty() && !previous.isNull()) {
    if (HasTagName(previous, kTableRow))
      inferred_label = FindChildText(previous);

    previous = previous.previousSibling();
  }

  return inferred_label;
}

// Helper for |InferLabelForElement()| that infers a label, if possible, from
// a surrounding div table,
// e.g. <div>Some Text<span><input ...></span></div>
// e.g. <div>Some Text</div><div><input ...></div>
base::string16 InferLabelFromDivTable(const WebFormControlElement& element) {
  WebNode node = element.parentNode();
  bool looking_for_parent = true;
  std::set<WebNode> divs_to_skip;

  // Search the sibling and parent <div>s until we find a candidate label.
  base::string16 inferred_label;
  CR_DEFINE_STATIC_LOCAL(WebString, kDiv, ("div"));
  CR_DEFINE_STATIC_LOCAL(WebString, kTable, ("table"));
  CR_DEFINE_STATIC_LOCAL(WebString, kFieldSet, ("fieldset"));
  while (inferred_label.empty() && !node.isNull()) {
    if (HasTagName(node, kDiv)) {
      if (looking_for_parent)
        inferred_label = FindChildTextWithIgnoreList(node, divs_to_skip);
      else
        inferred_label = FindChildText(node);

      // Avoid sibling DIVs that contain autofillable fields.
      if (!looking_for_parent && !inferred_label.empty()) {
        CR_DEFINE_STATIC_LOCAL(WebString, kSelector,
                               ("input, select, textarea"));
        blink::WebExceptionCode ec = 0;
        WebElement result_element = node.querySelector(kSelector, ec);
        if (!result_element.isNull()) {
          inferred_label.clear();
          divs_to_skip.insert(node);
        }
      }

      looking_for_parent = false;
    } else if (looking_for_parent &&
               (HasTagName(node, kTable) || HasTagName(node, kFieldSet))) {
      // If the element is in a table or fieldset, its label most likely is too.
      break;
    }

    if (node.previousSibling().isNull()) {
      // If there are no more siblings, continue walking up the tree.
      looking_for_parent = true;
    }

    node = looking_for_parent ? node.parentNode() : node.previousSibling();
  }

  return inferred_label;
}

// Helper for |InferLabelForElement()| that infers a label, if possible, from
// a surrounding definition list,
// e.g. <dl><dt>Some Text</dt><dd><input ...></dd></dl>
// e.g. <dl><dt><b>Some Text</b></dt><dd><b><input ...></b></dd></dl>
base::string16 InferLabelFromDefinitionList(
    const WebFormControlElement& element) {
  CR_DEFINE_STATIC_LOCAL(WebString, kDefinitionData, ("dd"));
  WebNode parent = element.parentNode();
  while (!parent.isNull() && parent.isElementNode() &&
         !parent.to<WebElement>().hasHTMLTagName(kDefinitionData))
    parent = parent.parentNode();

  if (parent.isNull() || !HasTagName(parent, kDefinitionData))
    return base::string16();

  // Skip by any intervening text nodes.
  WebNode previous = parent.previousSibling();
  while (!previous.isNull() && previous.isTextNode())
    previous = previous.previousSibling();

  CR_DEFINE_STATIC_LOCAL(WebString, kDefinitionTag, ("dt"));
  if (previous.isNull() || !HasTagName(previous, kDefinitionTag))
    return base::string16();

  return FindChildText(previous);
}

// Returns true if the closest ancestor is a <div> and not a <td>.
// Returns false if the closest ancestor is a <td> tag,
// or if there is no <div> or <td> ancestor.
bool ClosestAncestorIsDivAndNotTD(const WebFormControlElement& element) {
  for (WebNode parent_node = element.parentNode();
       !parent_node.isNull();
       parent_node = parent_node.parentNode()) {
    if (!parent_node.isElementNode())
      continue;

    WebElement cur_element = parent_node.to<WebElement>();
    if (cur_element.hasHTMLTagName("div"))
      return true;
    if (cur_element.hasHTMLTagName("td"))
      return false;
  }
  return false;
}

// Infers corresponding label for |element| from surrounding context in the DOM,
// e.g. the contents of the preceding <p> tag or text element.
base::string16 InferLabelForElement(const WebFormControlElement& element) {
  base::string16 inferred_label;
  if (IsCheckableElement(toWebInputElement(&element))) {
    inferred_label = InferLabelFromNext(element);
    if (!inferred_label.empty())
      return inferred_label;
  }

  inferred_label = InferLabelFromPrevious(element);
  if (!inferred_label.empty())
    return inferred_label;

  // If we didn't find a label, check for placeholder text.
  inferred_label = InferLabelFromPlaceholder(element);
  if (!inferred_label.empty())
    return inferred_label;

  // If we didn't find a label, check for list item case.
  inferred_label = InferLabelFromListItem(element);
  if (!inferred_label.empty())
    return inferred_label;

  // If we didn't find a label, check for definition list case.
  inferred_label = InferLabelFromDefinitionList(element);
  if (!inferred_label.empty())
    return inferred_label;

  bool check_div_first = ClosestAncestorIsDivAndNotTD(element);
  if (check_div_first) {
    // If we didn't find a label, check for div table case first since it's the
    // closest ancestor.
    inferred_label = InferLabelFromDivTable(element);
    if (!inferred_label.empty())
      return inferred_label;
  }

  // If we didn't find a label, check for table cell case.
  inferred_label = InferLabelFromTableColumn(element);
  if (!inferred_label.empty())
    return inferred_label;

  // If we didn't find a label, check for table row case.
  inferred_label = InferLabelFromTableRow(element);
  if (!inferred_label.empty())
    return inferred_label;

  if (!check_div_first) {
    // If we didn't find a label from the table, check for div table case if we
    // haven't already.
    inferred_label = InferLabelFromDivTable(element);
  }
  return inferred_label;
}

// Fills |option_strings| with the values of the <option> elements present in
// |select_element|.
void GetOptionStringsFromElement(const WebSelectElement& select_element,
                                 std::vector<base::string16>* option_values,
                                 std::vector<base::string16>* option_contents) {
  DCHECK(!select_element.isNull());

  option_values->clear();
  option_contents->clear();
  WebVector<WebElement> list_items = select_element.listItems();

  // Constrain the maximum list length to prevent a malicious site from DOS'ing
  // the browser, without entirely breaking autocomplete for some extreme
  // legitimate sites: http://crbug.com/49332 and http://crbug.com/363094
  if (list_items.size() > kMaxListSize)
    return;

  option_values->reserve(list_items.size());
  option_contents->reserve(list_items.size());
  for (size_t i = 0; i < list_items.size(); ++i) {
    if (IsOptionElement(list_items[i])) {
      const WebOptionElement option = list_items[i].toConst<WebOptionElement>();
      option_values->push_back(option.value());
      option_contents->push_back(option.text());
    }
  }
}

// The callback type used by |ForEachMatchingFormField()|.
typedef void (*Callback)(const FormFieldData&,
                         bool, /* is_initiating_element */
                         blink::WebFormControlElement*);

void ForEachMatchingFormFieldCommon(
    std::vector<WebFormControlElement>* control_elements,
    const WebElement& initiating_element,
    const FormData& data,
    FieldFilterMask filters,
    bool force_override,
    Callback callback) {
  DCHECK(control_elements);
  if (control_elements->size() != data.fields.size()) {
    // This case should be reachable only for pathological websites and tests,
    // which add or remove form fields while the user is interacting with the
    // Autofill popup.
    return;
  }

  // It's possible that the site has injected fields into the form after the
  // page has loaded, so we can't assert that the size of the cached control
  // elements is equal to the size of the fields in |form|.  Fortunately, the
  // one case in the wild where this happens, paypal.com signup form, the fields
  // are appended to the end of the form and are not visible.
  for (size_t i = 0; i < control_elements->size(); ++i) {
    WebFormControlElement* element = &(*control_elements)[i];

    if (base::string16(element->nameForAutofill()) != data.fields[i].name) {
      // This case should be reachable only for pathological websites, which
      // rename form fields while the user is interacting with the Autofill
      // popup.  I (isherman) am not aware of any such websites, and so am
      // optimistically including a NOTREACHED().  If you ever trip this check,
      // please file a bug against me.
      NOTREACHED();
      continue;
    }

    bool is_initiating_element = (*element == initiating_element);

    // Only autofill empty fields and the field that initiated the filling,
    // i.e. the field the user is currently editing and interacting with.
    const WebInputElement* input_element = toWebInputElement(element);
    if (!force_override && !is_initiating_element &&
        ((IsAutofillableInputElement(input_element) ||
          IsTextAreaElement(*element)) &&
         !element->value().isEmpty()))
      continue;

    if (((filters & FILTER_DISABLED_ELEMENTS) && !element->isEnabled()) ||
        ((filters & FILTER_READONLY_ELEMENTS) && element->isReadOnly()) ||
        ((filters & FILTER_NON_FOCUSABLE_ELEMENTS) && !element->isFocusable()))
      continue;

    callback(data.fields[i], is_initiating_element, element);
  }
}

// For each autofillable field in |data| that matches a field in the |form|,
// the |callback| is invoked with the corresponding |form| field data.
void ForEachMatchingFormField(const WebFormElement& form_element,
                              const WebElement& initiating_element,
                              const FormData& data,
                              FieldFilterMask filters,
                              bool force_override,
                              Callback callback) {
  std::vector<WebFormControlElement> control_elements =
      ExtractAutofillableElementsInForm(form_element, ExtractionRequirements());
  ForEachMatchingFormFieldCommon(&control_elements, initiating_element, data,
                                 filters, force_override, callback);
}

// For each autofillable field in |data| that matches a field in the set of
// unowned autofillable form fields, the |callback| is invoked with the
// corresponding |data| field.
void ForEachMatchingUnownedFormField(const WebElement& initiating_element,
                                     const FormData& data,
                                     FieldFilterMask filters,
                                     bool force_override,
                                     Callback callback) {
  if (initiating_element.isNull())
    return;

  std::vector<WebFormControlElement> control_elements =
      GetUnownedAutofillableFormFieldElements(
          initiating_element.document().all(), nullptr);
  if (!IsElementInControlElementSet(initiating_element, control_elements))
    return;

  ForEachMatchingFormFieldCommon(&control_elements, initiating_element, data,
                                 filters, force_override, callback);
}

// Sets the |field|'s value to the value in |data|.
// Also sets the "autofilled" attribute, causing the background to be yellow.
void FillFormField(const FormFieldData& data,
                   bool is_initiating_node,
                   blink::WebFormControlElement* field) {
  // Nothing to fill.
  if (data.value.empty())
    return;

  if (!data.is_autofilled)
    return;

  WebInputElement* input_element = toWebInputElement(field);
  if (IsCheckableElement(input_element)) {
    input_element->setChecked(data.is_checked, true);
  } else {
    base::string16 value = data.value;
    if (IsTextInput(input_element) || IsMonthInput(input_element)) {
      // If the maxlength attribute contains a negative value, maxLength()
      // returns the default maxlength value.
      TruncateString(&value, input_element->maxLength());
    }
    field->setValue(value, true);
  }

  field->setAutofilled(true);

  if (is_initiating_node &&
      ((IsTextInput(input_element) || IsMonthInput(input_element)) ||
       IsTextAreaElement(*field))) {
    int length = field->value().length();
    field->setSelectionRange(length, length);
    // Clear the current IME composition (the underline), if there is one.
    field->document().frame()->unmarkText();
  }
}

// Sets the |field|'s "suggested" (non JS visible) value to the value in |data|.
// Also sets the "autofilled" attribute, causing the background to be yellow.
void PreviewFormField(const FormFieldData& data,
                      bool is_initiating_node,
                      blink::WebFormControlElement* field) {
  // Nothing to preview.
  if (data.value.empty())
    return;

  if (!data.is_autofilled)
    return;

  // Preview input, textarea and select fields. For input fields, excludes
  // checkboxes and radio buttons, as there is no provision for
  // setSuggestedCheckedValue in WebInputElement.
  WebInputElement* input_element = toWebInputElement(field);
  if (IsTextInput(input_element) || IsMonthInput(input_element)) {
    // If the maxlength attribute contains a negative value, maxLength()
    // returns the default maxlength value.
    input_element->setSuggestedValue(
      data.value.substr(0, input_element->maxLength()));
    input_element->setAutofilled(true);
  } else if (IsTextAreaElement(*field) || IsSelectElement(*field)) {
    field->setSuggestedValue(data.value);
    field->setAutofilled(true);
  }

  if (is_initiating_node &&
      (IsTextInput(input_element) || IsTextAreaElement(*field))) {
    // Select the part of the text that the user didn't type.
    int start = field->value().length();
    int end = field->suggestedValue().length();
    field->setSelectionRange(start, end);
  }
}

// Recursively checks whether |node| or any of its children have a non-empty
// bounding box. The recursion depth is bounded by |depth|.
bool IsWebNodeVisibleImpl(const blink::WebNode& node, const int depth) {
  if (depth < 0)
    return false;
  if (node.hasNonEmptyBoundingBox())
    return true;

  // The childNodes method is not a const method. Therefore it cannot be called
  // on a const reference. Therefore we need a const cast.
  const blink::WebNodeList& children =
      const_cast<blink::WebNode&>(node).childNodes();
  size_t length = children.length();
  for (size_t i = 0; i < length; ++i) {
    const blink::WebNode& item = children.item(i);
    if (IsWebNodeVisibleImpl(item, depth - 1))
      return true;
  }
  return false;
}

bool ExtractFieldsFromControlElements(
    const WebVector<WebFormControlElement>& control_elements,
    RequirementsMask requirements,
    ExtractMask extract_mask,
    ScopedVector<FormFieldData>* form_fields,
    std::vector<bool>* fields_extracted,
    std::map<WebFormControlElement, FormFieldData*>* element_map) {
  for (size_t i = 0; i < control_elements.size(); ++i) {
    const WebFormControlElement& control_element = control_elements[i];

    if (!IsAutofillableElement(control_element))
      continue;

    const WebInputElement* input_element = toWebInputElement(&control_element);
    if (requirements & REQUIRE_AUTOCOMPLETE &&
        IsAutofillableInputElement(input_element) &&
        !SatisfiesRequireAutocomplete(*input_element))
      continue;

    // Create a new FormFieldData, fill it out and map it to the field's name.
    FormFieldData* form_field = new FormFieldData;
    WebFormControlElementToFormField(control_element, extract_mask, form_field);
    form_fields->push_back(form_field);
    (*element_map)[control_element] = form_field;
    (*fields_extracted)[i] = true;
  }

  // If we failed to extract any fields, give up.  Also, to avoid overly
  // expensive computation, we impose a maximum number of allowable fields.
  if (form_fields->empty() || form_fields->size() > kMaxParseableFields)
    return false;
  return true;
}

// For each label element, get the corresponding form control element, use the
// form control element's name as a key into the
// <WebFormControlElement, FormFieldData> map to find the previously created
// FormFieldData and set the FormFieldData's label to the
// label.firstChild().nodeValue() of the label element.
void MatchLabelsAndFields(
    const WebElementCollection& labels,
    std::map<WebFormControlElement, FormFieldData*>* element_map) {
  CR_DEFINE_STATIC_LOCAL(WebString, kFor, ("for"));
  CR_DEFINE_STATIC_LOCAL(WebString, kHidden, ("hidden"));

  for (WebElement item = labels.firstItem(); !item.isNull();
       item = labels.nextItem()) {
    WebLabelElement label = item.to<WebLabelElement>();
    WebFormControlElement field_element =
        label.correspondingControl().to<WebFormControlElement>();
    FormFieldData* field_data = nullptr;

    if (field_element.isNull()) {
      // Sometimes site authors will incorrectly specify the corresponding
      // field element's name rather than its id, so we compensate here.
      base::string16 element_name = label.getAttribute(kFor);
      if (element_name.empty())
        continue;
      // Look through the list for elements with this name. There can actually
      // be more than one. In this case, the label may not be particularly
      // useful, so just discard it.
      for (const auto& iter : *element_map) {
        if (iter.second->name == element_name) {
          if (field_data) {
            field_data = nullptr;
            break;
          } else {
            field_data = iter.second;
          }
        }
      }
    } else if (!field_element.isFormControlElement() ||
               field_element.formControlType() == kHidden) {
      continue;
    } else {
      // Typical case: look up |field_data| in |element_map|.
      auto iter = element_map->find(field_element);
      if (iter == element_map->end())
        continue;
      field_data = iter->second;
    }

    if (!field_data)
      continue;

    base::string16 label_text = FindChildText(label);

    // Concatenate labels because some sites might have multiple label
    // candidates.
    if (!field_data->label.empty() && !label_text.empty())
      field_data->label += base::ASCIIToUTF16(" ");
    field_data->label += label_text;
  }
}

// Common function shared by WebFormElementToFormData() and
// UnownedFormElementsAndFieldSetsToFormData(). Either pass in:
// 1) |form_element| and an empty |fieldsets|.
// or
// 2) a NULL |form_element|.
//
// If |field| is not NULL, then |form_control_element| should be not NULL.
bool FormOrFieldsetsToFormData(
    const blink::WebFormElement* form_element,
    const blink::WebFormControlElement* form_control_element,
    const std::vector<blink::WebElement>& fieldsets,
    const WebVector<WebFormControlElement>& control_elements,
    RequirementsMask requirements,
    ExtractMask extract_mask,
    FormData* form,
    FormFieldData* field) {
  CR_DEFINE_STATIC_LOCAL(WebString, kLabel, ("label"));

  if (form_element)
    DCHECK(fieldsets.empty());
  if (field)
    DCHECK(form_control_element);

  // A map from a FormFieldData's name to the FormFieldData itself.
  std::map<WebFormControlElement, FormFieldData*> element_map;

  // The extracted FormFields. We use pointers so we can store them in
  // |element_map|.
  ScopedVector<FormFieldData> form_fields;

  // A vector of bools that indicate whether each field in the form meets the
  // requirements and thus will be in the resulting |form|.
  std::vector<bool> fields_extracted(control_elements.size(), false);

  if (!ExtractFieldsFromControlElements(control_elements, requirements,
                                        extract_mask, &form_fields,
                                        &fields_extracted, &element_map)) {
    return false;
  }

  if (form_element) {
    // Loop through the label elements inside the form element.  For each label
    // element, get the corresponding form control element, use the form control
    // element's name as a key into the <name, FormFieldData> map to find the
    // previously created FormFieldData and set the FormFieldData's label to the
    // label.firstChild().nodeValue() of the label element.
    WebElementCollection labels =
        form_element->getElementsByHTMLTagName(kLabel);
    DCHECK(!labels.isNull());
    MatchLabelsAndFields(labels, &element_map);
  } else {
    // Same as the if block, but for all the labels in fieldsets.
    for (size_t i = 0; i < fieldsets.size(); ++i) {
      WebElementCollection labels =
          fieldsets[i].getElementsByHTMLTagName(kLabel);
      DCHECK(!labels.isNull());
      MatchLabelsAndFields(labels, &element_map);
    }
  }

  // Loop through the form control elements, extracting the label text from
  // the DOM.  We use the |fields_extracted| vector to make sure we assign the
  // extracted label to the correct field, as it's possible |form_fields| will
  // not contain all of the elements in |control_elements|.
  for (size_t i = 0, field_idx = 0;
       i < control_elements.size() && field_idx < form_fields.size(); ++i) {
    // This field didn't meet the requirements, so don't try to find a label
    // for it.
    if (!fields_extracted[i])
      continue;

    const WebFormControlElement& control_element = control_elements[i];
    if (form_fields[field_idx]->label.empty())
      form_fields[field_idx]->label = InferLabelForElement(control_element);
    TruncateString(&form_fields[field_idx]->label, kMaxDataLength);

    if (field && *form_control_element == control_element)
      *field = *form_fields[field_idx];

    ++field_idx;
  }

  // Copy the created FormFields into the resulting FormData object.
  for (const auto& iter : form_fields)
    form->fields.push_back(*iter);
  return true;
}

}  // namespace

const size_t kMaxParseableFields = 200;

bool IsMonthInput(const WebInputElement* element) {
  CR_DEFINE_STATIC_LOCAL(WebString, kMonth, ("month"));
  return element && !element->isNull() && element->formControlType() == kMonth;
}

// All text fields, including password fields, should be extracted.
bool IsTextInput(const WebInputElement* element) {
  return element && !element->isNull() && element->isTextField();
}

bool IsSelectElement(const WebFormControlElement& element) {
  // Static for improved performance.
  CR_DEFINE_STATIC_LOCAL(WebString, kSelectOne, ("select-one"));
  return !element.isNull() && element.formControlType() == kSelectOne;
}

bool IsTextAreaElement(const WebFormControlElement& element) {
  // Static for improved performance.
  CR_DEFINE_STATIC_LOCAL(WebString, kTextArea, ("textarea"));
  return !element.isNull() && element.formControlType() == kTextArea;
}

bool IsCheckableElement(const WebInputElement* element) {
  if (!element || element->isNull())
    return false;

  return element->isCheckbox() || element->isRadioButton();
}

bool IsAutofillableInputElement(const WebInputElement* element) {
  return IsTextInput(element) ||
         IsMonthInput(element) ||
         IsCheckableElement(element);
}

const base::string16 GetFormIdentifier(const WebFormElement& form) {
  base::string16 identifier = form.name();
  CR_DEFINE_STATIC_LOCAL(WebString, kId, ("id"));
  if (identifier.empty())
    identifier = form.getAttribute(kId);

  return identifier;
}

bool IsWebNodeVisible(const blink::WebNode& node) {
  // In the bug http://crbug.com/237216 the form's bounding box is empty
  // however the form has non empty children. Thus we need to look at the
  // form's children.
  int kNodeSearchDepth = 2;
  return IsWebNodeVisibleImpl(node, kNodeSearchDepth);
}

std::vector<blink::WebFormControlElement> ExtractAutofillableElementsFromSet(
    const WebVector<WebFormControlElement>& control_elements,
    RequirementsMask requirements) {
  std::vector<blink::WebFormControlElement> autofillable_elements;
  for (size_t i = 0; i < control_elements.size(); ++i) {
    WebFormControlElement element = control_elements[i];
    if (!IsAutofillableElement(element))
      continue;

    if (requirements & REQUIRE_AUTOCOMPLETE) {
      // TODO(isherman): WebKit currently doesn't handle the autocomplete
      // attribute for select or textarea elements, but it probably should.
      const WebInputElement* input_element =
          toWebInputElement(&control_elements[i]);
      if (IsAutofillableInputElement(input_element) &&
          !SatisfiesRequireAutocomplete(*input_element))
        continue;
    }

    autofillable_elements.push_back(element);
  }
  return autofillable_elements;
}

std::vector<WebFormControlElement> ExtractAutofillableElementsInForm(
    const WebFormElement& form_element,
    RequirementsMask requirements) {
  WebVector<WebFormControlElement> control_elements;
  form_element.getFormControlElements(control_elements);

  return ExtractAutofillableElementsFromSet(control_elements, requirements);
}

void WebFormControlElementToFormField(const WebFormControlElement& element,
                                      ExtractMask extract_mask,
                                      FormFieldData* field) {
  DCHECK(field);
  DCHECK(!element.isNull());
  CR_DEFINE_STATIC_LOCAL(WebString, kAutocomplete, ("autocomplete"));
  CR_DEFINE_STATIC_LOCAL(WebString, kRole, ("role"));

  // The label is not officially part of a WebFormControlElement; however, the
  // labels for all form control elements are scraped from the DOM and set in
  // WebFormElementToFormData.
  field->name = element.nameForAutofill();
  field->form_control_type = base::UTF16ToUTF8(element.formControlType());
  field->autocomplete_attribute =
      base::UTF16ToUTF8(element.getAttribute(kAutocomplete));
  if (field->autocomplete_attribute.size() > kMaxDataLength) {
    // Discard overly long attribute values to avoid DOS-ing the browser
    // process.  However, send over a default string to indicate that the
    // attribute was present.
    field->autocomplete_attribute = "x-max-data-length-exceeded";
  }
  if (LowerCaseEqualsASCII(element.getAttribute(kRole), "presentation"))
    field->role = FormFieldData::ROLE_ATTRIBUTE_PRESENTATION;

  if (!IsAutofillableElement(element))
    return;

  const WebInputElement* input_element = toWebInputElement(&element);
  if (IsAutofillableInputElement(input_element) ||
      IsTextAreaElement(element)) {
    field->is_autofilled = element.isAutofilled();
    field->is_focusable = element.isFocusable();
    field->should_autocomplete = element.autoComplete();
    field->text_direction = element.directionForFormData() ==
        "rtl" ? base::i18n::RIGHT_TO_LEFT : base::i18n::LEFT_TO_RIGHT;
  }

  if (IsAutofillableInputElement(input_element)) {
    if (IsTextInput(input_element))
      field->max_length = input_element->maxLength();

    field->is_checkable = IsCheckableElement(input_element);
    field->is_checked = input_element->isChecked();
  } else if (IsTextAreaElement(element)) {
    // Nothing more to do in this case.
  } else if (extract_mask & EXTRACT_OPTIONS) {
    // Set option strings on the field if available.
    DCHECK(IsSelectElement(element));
    const WebSelectElement select_element = element.toConst<WebSelectElement>();
    GetOptionStringsFromElement(select_element,
                                &field->option_values,
                                &field->option_contents);
  }

  if (!(extract_mask & EXTRACT_VALUE))
    return;

  base::string16 value = element.value();

  if (IsSelectElement(element) && (extract_mask & EXTRACT_OPTION_TEXT)) {
    const WebSelectElement select_element = element.toConst<WebSelectElement>();
    // Convert the |select_element| value to text if requested.
    WebVector<WebElement> list_items = select_element.listItems();
    for (size_t i = 0; i < list_items.size(); ++i) {
      if (IsOptionElement(list_items[i])) {
        const WebOptionElement option_element =
            list_items[i].toConst<WebOptionElement>();
        if (option_element.value() == value) {
          value = option_element.text();
          break;
        }
      }
    }
  }

  // Constrain the maximum data length to prevent a malicious site from DOS'ing
  // the browser: http://crbug.com/49332
  TruncateString(&value, kMaxDataLength);

  field->value = value;
}

bool WebFormElementToFormData(
    const blink::WebFormElement& form_element,
    const blink::WebFormControlElement& form_control_element,
    RequirementsMask requirements,
    ExtractMask extract_mask,
    FormData* form,
    FormFieldData* field) {
  const WebFrame* frame = form_element.document().frame();
  if (!frame)
    return false;

  if (requirements & REQUIRE_AUTOCOMPLETE && !form_element.autoComplete())
    return false;

  form->name = GetFormIdentifier(form_element);
  form->origin = frame->document().url();
  form->action = frame->document().completeURL(form_element.action());
  form->user_submitted = form_element.wasUserSubmitted();

  // If the completed URL is not valid, just use the action we get from
  // WebKit.
  if (!form->action.is_valid())
    form->action = GURL(form_element.action());

  WebVector<WebFormControlElement> control_elements;
  form_element.getFormControlElements(control_elements);

  std::vector<blink::WebElement> dummy_fieldset;
  return FormOrFieldsetsToFormData(&form_element, &form_control_element,
                                   dummy_fieldset, control_elements,
                                   requirements, extract_mask, form, field);
}

std::vector<WebFormControlElement>
GetUnownedAutofillableFormFieldElements(
    const WebElementCollection& elements,
    std::vector<WebElement>* fieldsets) {
  std::vector<WebFormControlElement> unowned_fieldset_children;
  for (WebElement element = elements.firstItem();
       !element.isNull();
       element = elements.nextItem()) {
    if (element.isFormControlElement()) {
      WebFormControlElement control = element.to<WebFormControlElement>();
      if (control.form().isNull())
        unowned_fieldset_children.push_back(control);
    }

    if (fieldsets && element.hasHTMLTagName("fieldset") &&
        !IsElementInsideFormOrFieldSet(element)) {
      fieldsets->push_back(element);
    }
  }
  return ExtractAutofillableElementsFromSet(unowned_fieldset_children,
                                            REQUIRE_NONE);
}

bool UnownedFormElementsAndFieldSetsToFormData(
    const std::vector<blink::WebElement>& fieldsets,
    const std::vector<blink::WebFormControlElement>& control_elements,
    const blink::WebFormControlElement* element,
    const GURL& origin,
    RequirementsMask requirements,
    ExtractMask extract_mask,
    FormData* form,
    FormFieldData* field) {
  form->origin = origin;
  form->user_submitted = false;
  form->is_form_tag = false;

  return FormOrFieldsetsToFormData(nullptr, element, fieldsets,
                                   control_elements, requirements, extract_mask,
                                   form, field);
}

bool FindFormAndFieldForFormControlElement(const WebFormControlElement& element,
                                           FormData* form,
                                           FormFieldData* field,
                                           RequirementsMask requirements) {
  if (!IsAutofillableElement(element))
    return false;

  ExtractMask extract_mask =
      static_cast<ExtractMask>(EXTRACT_VALUE | EXTRACT_OPTIONS);
  const WebFormElement form_element = element.form();
  if (form_element.isNull()) {
    // No associated form, try the synthetic form for unowned form elements.
    WebDocument document = element.document();
    std::vector<WebElement> fieldsets;
    std::vector<WebFormControlElement> control_elements =
        GetUnownedAutofillableFormFieldElements(document.all(), &fieldsets);
    return UnownedFormElementsAndFieldSetsToFormData(
        fieldsets, control_elements, &element, document.url(), requirements,
        extract_mask, form, field);
  }

  return WebFormElementToFormData(form_element,
                                  element,
                                  requirements,
                                  extract_mask,
                                  form,
                                  field);
}

void FillForm(const FormData& form, const WebFormControlElement& element) {
  WebFormElement form_element = element.form();
  if (form_element.isNull()) {
    ForEachMatchingUnownedFormField(element,
                                    form,
                                    FILTER_ALL_NON_EDITABLE_ELEMENTS,
                                    false, /* dont force override */
                                    &FillFormField);
    return;
  }

  ForEachMatchingFormField(form_element,
                           element,
                           form,
                           FILTER_ALL_NON_EDITABLE_ELEMENTS,
                           false, /* dont force override */
                           &FillFormField);
}

void FillFormIncludingNonFocusableElements(const FormData& form_data,
                                           const WebFormElement& form_element) {
  if (form_element.isNull()) {
    NOTREACHED();
    return;
  }

  FieldFilterMask filter_mask = static_cast<FieldFilterMask>(
      FILTER_DISABLED_ELEMENTS | FILTER_READONLY_ELEMENTS);
  ForEachMatchingFormField(form_element,
                           WebInputElement(),
                           form_data,
                           filter_mask,
                           true, /* force override */
                           &FillFormField);
}

void PreviewForm(const FormData& form, const WebFormControlElement& element) {
  WebFormElement form_element = element.form();
  if (form_element.isNull()) {
    ForEachMatchingUnownedFormField(element,
                                    form,
                                    FILTER_ALL_NON_EDITABLE_ELEMENTS,
                                    false, /* dont force override */
                                    &PreviewFormField);
    return;
  }

  ForEachMatchingFormField(form_element,
                           element,
                           form,
                           FILTER_ALL_NON_EDITABLE_ELEMENTS,
                           false, /* dont force override */
                           &PreviewFormField);
}

bool ClearPreviewedFormWithElement(const WebFormControlElement& element,
                                   bool was_autofilled) {
  WebFormElement form_element = element.form();
  std::vector<WebFormControlElement> control_elements;
  if (form_element.isNull()) {
    control_elements = GetUnownedAutofillableFormFieldElements(
        element.document().all(), nullptr);
    if (!IsElementInControlElementSet(element, control_elements))
      return false;
  } else {
    control_elements = ExtractAutofillableElementsInForm(
        form_element, ExtractionRequirements());
  }

  for (size_t i = 0; i < control_elements.size(); ++i) {
    // There might be unrelated elements in this form which have already been
    // auto-filled.  For example, the user might have already filled the address
    // part of a form and now be dealing with the credit card section.  We only
    // want to reset the auto-filled status for fields that were previewed.
    WebFormControlElement control_element = control_elements[i];

    // Only text input, textarea and select elements can be previewed.
    WebInputElement* input_element = toWebInputElement(&control_element);
    if (!IsTextInput(input_element) &&
        !IsMonthInput(input_element) &&
        !IsTextAreaElement(control_element) &&
        !IsSelectElement(control_element))
      continue;

    // If the element is not auto-filled, we did not preview it,
    // so there is nothing to reset.
    if (!control_element.isAutofilled())
      continue;

    if ((IsTextInput(input_element) ||
         IsMonthInput(input_element) ||
         IsTextAreaElement(control_element) ||
         IsSelectElement(control_element)) &&
        control_element.suggestedValue().isEmpty())
      continue;

    // Clear the suggested value. For the initiating node, also restore the
    // original value.
    if (IsTextInput(input_element) || IsMonthInput(input_element) ||
        IsTextAreaElement(control_element)) {
      control_element.setSuggestedValue(WebString());
      bool is_initiating_node = (element == control_element);
      if (is_initiating_node) {
        control_element.setAutofilled(was_autofilled);
        // Clearing the suggested value in the focused node (above) can cause
        // selection to be lost. We force selection range to restore the text
        // cursor.
        int length = control_element.value().length();
        control_element.setSelectionRange(length, length);
      } else {
        control_element.setAutofilled(false);
      }
    } else if (IsSelectElement(control_element)) {
      control_element.setSuggestedValue(WebString());
      control_element.setAutofilled(false);
    }
  }

  return true;
}

bool IsWebpageEmpty(const blink::WebFrame* frame) {
  blink::WebDocument document = frame->document();

  return IsWebElementEmpty(document.head()) &&
         IsWebElementEmpty(document.body());
}

bool IsWebElementEmpty(const blink::WebElement& element) {
  // This array contains all tags which can be present in an empty page.
  const char* const kAllowedValue[] = {
    "script",
    "meta",
    "title",
  };
  const size_t kAllowedValueLength = arraysize(kAllowedValue);

  if (element.isNull())
    return true;
  // The childNodes method is not a const method. Therefore it cannot be called
  // on a const reference. Therefore we need a const cast.
  const blink::WebNodeList& children =
      const_cast<blink::WebElement&>(element).childNodes();
  for (size_t i = 0; i < children.length(); ++i) {
    const blink::WebNode& item = children.item(i);

    if (item.isTextNode() &&
        !base::ContainsOnlyChars(item.nodeValue().utf8(),
                                 base::kWhitespaceASCII))
      return false;

    // We ignore all other items with names which begin with
    // the character # because they are not html tags.
    if (item.nodeName().utf8()[0] == '#')
      continue;

    bool tag_is_allowed = false;
    // Test if the item name is in the kAllowedValue array
    for (size_t allowed_value_index = 0;
         allowed_value_index < kAllowedValueLength; ++allowed_value_index) {
      if (HasTagName(item,
                     WebString::fromUTF8(kAllowedValue[allowed_value_index]))) {
        tag_is_allowed = true;
        break;
      }
    }
    if (!tag_is_allowed)
      return false;
  }
  return true;
}

gfx::RectF GetScaledBoundingBox(float scale, WebElement* element) {
  gfx::Rect bounding_box(element->boundsInViewportSpace());
  return gfx::RectF(bounding_box.x() * scale,
                    bounding_box.y() * scale,
                    bounding_box.width() * scale,
                    bounding_box.height() * scale);
}

}  // namespace autofill
