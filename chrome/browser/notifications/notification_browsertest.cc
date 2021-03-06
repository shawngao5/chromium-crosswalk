// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <deque>
#include <string>

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/simple_test_clock.h"
#include "base/time/clock.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/notifications/desktop_notification_profile_util.h"
#include "chrome/browser/notifications/notification.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "components/infobars/core/infobar.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/base/net_util.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/window_open_disposition.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/message_center_observer.h"
#include "url/gurl.h"

namespace {

const char kExpectedIconUrl[] = "/notifications/no_such_file.png";

enum InfobarAction {
  DISMISS = 0,
  ALLOW,
  DENY,
};

class NotificationChangeObserver {
public:
  virtual ~NotificationChangeObserver() {}
  virtual bool Wait() = 0;
};

class MessageCenterChangeObserver
    : public message_center::MessageCenterObserver,
      public NotificationChangeObserver {
 public:
  MessageCenterChangeObserver()
      : notification_received_(false) {
    message_center::MessageCenter::Get()->AddObserver(this);
  }

  ~MessageCenterChangeObserver() override {
    message_center::MessageCenter::Get()->RemoveObserver(this);
  }

  // NotificationChangeObserver:
  bool Wait() override {
    if (notification_received_)
      return true;

    message_loop_runner_ = new content::MessageLoopRunner;
    message_loop_runner_->Run();
    return notification_received_;
  }

  // message_center::MessageCenterObserver:
  void OnNotificationAdded(const std::string& notification_id) override {
    OnMessageCenterChanged();
  }

  void OnNotificationRemoved(const std::string& notification_id,
                             bool by_user) override {
    OnMessageCenterChanged();
  }

  void OnNotificationUpdated(const std::string& notification_id) override {
    OnMessageCenterChanged();
  }

  void OnMessageCenterChanged() {
    notification_received_ = true;
    if (message_loop_runner_.get())
      message_loop_runner_->Quit();
  }

  bool notification_received_;
  scoped_refptr<content::MessageLoopRunner> message_loop_runner_;

  DISALLOW_COPY_AND_ASSIGN(MessageCenterChangeObserver);
};

}  // namespace

class NotificationsTest : public InProcessBrowserTest {
 public:
  NotificationsTest() {}

 protected:
  int GetNotificationCount();
  int GetNotificationPopupCount();

  void CloseBrowserWindow(Browser* browser);
  void CrashTab(Browser* browser, int index);

  void DenyOrigin(const GURL& origin);
  void AllowOrigin(const GURL& origin);
  void AllowAllOrigins();
  void SetDefaultContentSetting(ContentSetting setting);

  void VerifyInfoBar(const Browser* browser, int index);
  std::string CreateNotification(Browser* browser,
                                 bool wait_for_new_balloon,
                                 const char* icon,
                                 const char* title,
                                 const char* body,
                                 const char* replace_id);
  std::string CreateSimpleNotification(Browser* browser,
                                       bool wait_for_new_balloon);
  bool RequestPermissionAndWait(Browser* browser);
  bool CancelNotification(const char* notification_id, Browser* browser);
  bool PerformActionOnInfoBar(Browser* browser,
                              InfobarAction action,
                              size_t infobar_index,
                              int tab_index);
  void GetPrefsByContentSetting(ContentSetting setting,
                                ContentSettingsForOneType* settings);
  bool CheckOriginInSetting(const ContentSettingsForOneType& settings,
                            const GURL& origin);

  GURL GetTestPageURLForFile(const std::string& file) const {
    return embedded_test_server()->GetURL(
      std::string("/notifications/") + file);
  }

  GURL GetTestPageURL() const {
    return GetTestPageURLForFile("notification_tester.html");
  }

 private:
  void DropOriginPreference(const GURL& origin);
};

int NotificationsTest::GetNotificationCount() {
  return message_center::MessageCenter::Get()->NotificationCount();
}

int NotificationsTest::GetNotificationPopupCount() {
  return message_center::MessageCenter::Get()->GetPopupNotifications().size();
}

void NotificationsTest::CloseBrowserWindow(Browser* browser) {
  content::WindowedNotificationObserver observer(
      chrome::NOTIFICATION_BROWSER_CLOSED,
      content::Source<Browser>(browser));
  browser->window()->Close();
  observer.Wait();
}

void NotificationsTest::CrashTab(Browser* browser, int index) {
  content::CrashTab(browser->tab_strip_model()->GetWebContentsAt(index));
}

void NotificationsTest::DenyOrigin(const GURL& origin) {
  DropOriginPreference(origin);
  DesktopNotificationProfileUtil::DenyPermission(browser()->profile(), origin);
}

void NotificationsTest::AllowOrigin(const GURL& origin) {
  DropOriginPreference(origin);
  DesktopNotificationProfileUtil::GrantPermission(browser()->profile(), origin);
}

void NotificationsTest::AllowAllOrigins() {
  // Reset all origins
  browser()->profile()->GetHostContentSettingsMap()->ClearSettingsForOneType(
       CONTENT_SETTINGS_TYPE_NOTIFICATIONS);
  SetDefaultContentSetting(CONTENT_SETTING_ALLOW);
 }

void NotificationsTest::SetDefaultContentSetting(ContentSetting setting) {
  browser()->profile()->GetHostContentSettingsMap()->SetDefaultContentSetting(
      CONTENT_SETTINGS_TYPE_NOTIFICATIONS, setting);
}

void NotificationsTest::VerifyInfoBar(const Browser* browser, int index) {
  InfoBarService* infobar_service = InfoBarService::FromWebContents(
      browser->tab_strip_model()->GetWebContentsAt(index));

  ASSERT_EQ(1U, infobar_service->infobar_count());
  ConfirmInfoBarDelegate* confirm_infobar =
      infobar_service->infobar_at(0)->delegate()->AsConfirmInfoBarDelegate();
  ASSERT_TRUE(confirm_infobar);
  int buttons = confirm_infobar->GetButtons();
  EXPECT_TRUE(buttons & ConfirmInfoBarDelegate::BUTTON_OK);
  EXPECT_TRUE(buttons & ConfirmInfoBarDelegate::BUTTON_CANCEL);
}

std::string NotificationsTest::CreateNotification(
    Browser* browser,
    bool wait_for_new_balloon,
    const char* icon,
    const char* title,
    const char* body,
    const char* replace_id) {
  std::string script = base::StringPrintf(
      "createNotification('%s', '%s', '%s', '%s');",
      icon, title, body, replace_id);

  MessageCenterChangeObserver observer;
  std::string result;
  bool success = content::ExecuteScriptAndExtractString(
      browser->tab_strip_model()->GetActiveWebContents(),
      script,
      &result);
  if (success && result != "-1" && wait_for_new_balloon)
    success = observer.Wait();
  EXPECT_TRUE(success);

  return result;
}

std::string NotificationsTest::CreateSimpleNotification(
    Browser* browser,
    bool wait_for_new_balloon) {
  return CreateNotification(
      browser, wait_for_new_balloon,
      "no_such_file.png", "My Title", "My Body", "");
}

bool NotificationsTest::RequestPermissionAndWait(Browser* browser) {
  InfoBarService* infobar_service = InfoBarService::FromWebContents(
      browser->tab_strip_model()->GetActiveWebContents());
  content::WindowedNotificationObserver observer(
      chrome::NOTIFICATION_TAB_CONTENTS_INFOBAR_ADDED,
      content::Source<InfoBarService>(infobar_service));
  std::string result;
  bool success = content::ExecuteScriptAndExtractString(
      browser->tab_strip_model()->GetActiveWebContents(),
      "requestPermission();",
      &result);
  if (!success || result != "1")
    return false;
  observer.Wait();
  return true;
}

bool NotificationsTest::CancelNotification(
    const char* notification_id,
    Browser* browser) {
  std::string script = base::StringPrintf(
      "cancelNotification('%s');",
      notification_id);

  MessageCenterChangeObserver observer;
  std::string result;
  bool success = content::ExecuteScriptAndExtractString(
      browser->tab_strip_model()->GetActiveWebContents(),
      script,
      &result);
  if (!success || result != "1")
    return false;
  return observer.Wait();
}

bool NotificationsTest::PerformActionOnInfoBar(
    Browser* browser,
    InfobarAction action,
    size_t infobar_index,
    int tab_index) {
  InfoBarService* infobar_service = InfoBarService::FromWebContents(
      browser->tab_strip_model()->GetWebContentsAt(tab_index));
  if (infobar_index >= infobar_service->infobar_count()) {
    ADD_FAILURE();
    return false;
  }

  infobars::InfoBar* infobar = infobar_service->infobar_at(infobar_index);
  infobars::InfoBarDelegate* infobar_delegate = infobar->delegate();
  switch (action) {
    case DISMISS:
      infobar_delegate->InfoBarDismissed();
      infobar_service->RemoveInfoBar(infobar);
      return true;

    case ALLOW: {
      ConfirmInfoBarDelegate* confirm_infobar_delegate =
          infobar_delegate->AsConfirmInfoBarDelegate();
      if (!confirm_infobar_delegate) {
        ADD_FAILURE();
      } else if (confirm_infobar_delegate->Accept()) {
        infobar_service->RemoveInfoBar(infobar);
        return true;
      }
    }

    case DENY: {
      ConfirmInfoBarDelegate* confirm_infobar_delegate =
          infobar_delegate->AsConfirmInfoBarDelegate();
      if (!confirm_infobar_delegate) {
        ADD_FAILURE();
      } else if (confirm_infobar_delegate->Cancel()) {
        infobar_service->RemoveInfoBar(infobar);
        return true;
      }
    }
  }

  return false;
}

void NotificationsTest::GetPrefsByContentSetting(
    ContentSetting setting,
    ContentSettingsForOneType* settings) {
  DesktopNotificationProfileUtil::GetNotificationsSettings(
      browser()->profile(), settings);
  for (ContentSettingsForOneType::iterator it = settings->begin();
       it != settings->end(); ) {
    if (it->setting != setting || it->source.compare("preference") != 0)
      it = settings->erase(it);
    else
      ++it;
  }
}

bool NotificationsTest::CheckOriginInSetting(
    const ContentSettingsForOneType& settings,
    const GURL& origin) {
  ContentSettingsPattern pattern =
      ContentSettingsPattern::FromURLNoWildcard(origin);
  for (ContentSettingsForOneType::const_iterator it = settings.begin();
       it != settings.end(); ++it) {
    if (it->primary_pattern == pattern)
      return true;
  }
  return false;
}

void NotificationsTest::DropOriginPreference(const GURL& origin) {
  DesktopNotificationProfileUtil::ClearSetting(browser()->profile(),
      ContentSettingsPattern::FromURLNoWildcard(origin));
}

// Flaky on Windows, Mac, Linux: http://crbug.com/437414.
IN_PROC_BROWSER_TEST_F(NotificationsTest, DISABLED_TestUserGestureInfobar) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  ui_test_utils::NavigateToURL(
      browser(),
      embedded_test_server()->GetURL(
          "/notifications/notifications_request_function.html"));

  // Request permission by calling request() while eval'ing an inline script;
  // That's considered a user gesture to webkit, and should produce an infobar.
  bool result;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      browser()->tab_strip_model()->GetActiveWebContents(),
      "window.domAutomationController.send(request());",
      &result));
  EXPECT_TRUE(result);

  InfoBarService* infobar_service = InfoBarService::FromWebContents(
      browser()->tab_strip_model()->GetWebContentsAt(0));
  EXPECT_EQ(1U, infobar_service->infobar_count());
}

IN_PROC_BROWSER_TEST_F(NotificationsTest, TestCreateSimpleNotification) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // Creates a simple notification.
  AllowAllOrigins();
  ui_test_utils::NavigateToURL(browser(), GetTestPageURL());

  std::string result = CreateSimpleNotification(browser(), true);
  EXPECT_NE("-1", result);

  GURL EXPECTED_ICON_URL = embedded_test_server()->GetURL(kExpectedIconUrl);
  ASSERT_EQ(1, GetNotificationCount());
  message_center::NotificationList::Notifications notifications =
      message_center::MessageCenter::Get()->GetVisibleNotifications();
  EXPECT_EQ(base::ASCIIToUTF16("My Title"),
            (*notifications.rbegin())->title());
  EXPECT_EQ(base::ASCIIToUTF16("My Body"),
            (*notifications.rbegin())->message());
}

IN_PROC_BROWSER_TEST_F(NotificationsTest, TestCloseNotification) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // Creates a notification and closes it.
  AllowAllOrigins();
  ui_test_utils::NavigateToURL(browser(), GetTestPageURL());

  std::string result = CreateSimpleNotification(browser(), true);
  EXPECT_NE("-1", result);
  ASSERT_EQ(1, GetNotificationCount());

  message_center::NotificationList::Notifications notifications =
      message_center::MessageCenter::Get()->GetVisibleNotifications();
  message_center::MessageCenter::Get()->RemoveNotification(
      (*notifications.rbegin())->id(),
      true);  // by_user

  ASSERT_EQ(0, GetNotificationCount());
}

IN_PROC_BROWSER_TEST_F(NotificationsTest, TestCancelNotification) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // Creates a notification and cancels it in the origin page.
  AllowAllOrigins();
  ui_test_utils::NavigateToURL(browser(), GetTestPageURL());

  std::string note_id = CreateSimpleNotification(browser(), true);
  EXPECT_NE(note_id, "-1");

  ASSERT_EQ(1, GetNotificationCount());
  ASSERT_TRUE(CancelNotification(note_id.c_str(), browser()));
  ASSERT_EQ(0, GetNotificationCount());
}

IN_PROC_BROWSER_TEST_F(NotificationsTest, TestPermissionInfobarAppears) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // Requests notification privileges and verifies the infobar appears.
  ui_test_utils::NavigateToURL(browser(), GetTestPageURL());
  ASSERT_TRUE(RequestPermissionAndWait(browser()));

  ASSERT_EQ(0, GetNotificationCount());
  ASSERT_NO_FATAL_FAILURE(VerifyInfoBar(browser(), 0));
}

IN_PROC_BROWSER_TEST_F(NotificationsTest, TestAllowOnPermissionInfobar) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // Tries to create a notification and clicks allow on the infobar.
  ui_test_utils::NavigateToURL(browser(), GetTestPageURL());
  // This notification should not be shown because we do not have permission.
  CreateSimpleNotification(browser(), false);
  ASSERT_EQ(0, GetNotificationCount());

  ASSERT_TRUE(RequestPermissionAndWait(browser()));
  ASSERT_TRUE(PerformActionOnInfoBar(browser(), ALLOW, 0, 0));

  CreateSimpleNotification(browser(), true);
  EXPECT_EQ(1, GetNotificationCount());
}

IN_PROC_BROWSER_TEST_F(NotificationsTest, TestDenyOnPermissionInfobar) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // Test that no notification is created
  // when Deny is chosen from permission infobar.
  ui_test_utils::NavigateToURL(browser(), GetTestPageURL());
  ASSERT_TRUE(RequestPermissionAndWait(browser()));
  PerformActionOnInfoBar(browser(), DENY, 0, 0);
  CreateSimpleNotification(browser(), false);
  ASSERT_EQ(0, GetNotificationCount());
  ContentSettingsForOneType settings;
  GetPrefsByContentSetting(CONTENT_SETTING_BLOCK, &settings);
  EXPECT_TRUE(CheckOriginInSetting(settings, GetTestPageURL()));
}

IN_PROC_BROWSER_TEST_F(NotificationsTest, TestClosePermissionInfobar) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // Test that no notification is created when permission infobar is dismissed.
  ui_test_utils::NavigateToURL(browser(), GetTestPageURL());
  ASSERT_TRUE(RequestPermissionAndWait(browser()));
  PerformActionOnInfoBar(browser(), DISMISS, 0, 0);
  CreateSimpleNotification(browser(), false);
  ASSERT_EQ(0, GetNotificationCount());
  ContentSettingsForOneType settings;
  GetPrefsByContentSetting(CONTENT_SETTING_BLOCK, &settings);
  EXPECT_EQ(0U, settings.size());
}

IN_PROC_BROWSER_TEST_F(NotificationsTest, TestAllowNotificationsFromAllSites) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // Verify that all domains can be allowed to show notifications.
  SetDefaultContentSetting(CONTENT_SETTING_ALLOW);
  ui_test_utils::NavigateToURL(browser(), GetTestPageURL());

  std::string result = CreateSimpleNotification(browser(), true);
  EXPECT_NE("-1", result);

  ASSERT_EQ(1, GetNotificationCount());
  InfoBarService* infobar_service = InfoBarService::FromWebContents(
      browser()->tab_strip_model()->GetWebContentsAt(0));
  EXPECT_EQ(0U, infobar_service->infobar_count());
}

IN_PROC_BROWSER_TEST_F(NotificationsTest, TestDenyNotificationsFromAllSites) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // Verify that no domain can show notifications.
  SetDefaultContentSetting(CONTENT_SETTING_BLOCK);
  ui_test_utils::NavigateToURL(browser(), GetTestPageURL());

  std::string result = CreateSimpleNotification(browser(), false);
  EXPECT_EQ("-1", result);

  ASSERT_EQ(0, GetNotificationCount());
}

IN_PROC_BROWSER_TEST_F(NotificationsTest, TestDenyDomainAndAllowAll) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // Verify that denying a domain and allowing all shouldn't show
  // notifications from the denied domain.
  DenyOrigin(GetTestPageURL().GetOrigin());
  SetDefaultContentSetting(CONTENT_SETTING_ALLOW);

  ui_test_utils::NavigateToURL(browser(), GetTestPageURL());

  std::string result = CreateSimpleNotification(browser(), false);
  EXPECT_EQ("-1", result);

  ASSERT_EQ(0, GetNotificationCount());
}

IN_PROC_BROWSER_TEST_F(NotificationsTest, TestAllowDomainAndDenyAll) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // Verify that allowing a domain and denying all others should show
  // notifications from the allowed domain.
  AllowOrigin(GetTestPageURL().GetOrigin());
  SetDefaultContentSetting(CONTENT_SETTING_BLOCK);

  ui_test_utils::NavigateToURL(browser(), GetTestPageURL());

  std::string result = CreateSimpleNotification(browser(), true);
  EXPECT_NE("-1", result);

  ASSERT_EQ(1, GetNotificationCount());
}

IN_PROC_BROWSER_TEST_F(NotificationsTest, TestDenyAndThenAllowDomain) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // Verify that denying and again allowing should show notifications.
  DenyOrigin(GetTestPageURL().GetOrigin());

  ui_test_utils::NavigateToURL(browser(), GetTestPageURL());

  std::string result = CreateSimpleNotification(browser(), false);
  EXPECT_EQ("-1", result);

  ASSERT_EQ(0, GetNotificationCount());

  AllowOrigin(GetTestPageURL().GetOrigin());
  result = CreateSimpleNotification(browser(), true);
  EXPECT_NE("-1", result);

  ASSERT_EQ(1, GetNotificationCount());
  InfoBarService* infobar_service = InfoBarService::FromWebContents(
      browser()->tab_strip_model()->GetWebContentsAt(0));
  EXPECT_EQ(0U, infobar_service->infobar_count());
}

IN_PROC_BROWSER_TEST_F(NotificationsTest, TestCreateDenyCloseNotifications) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // Verify able to create, deny, and close the notification.
  AllowAllOrigins();
  ui_test_utils::NavigateToURL(browser(), GetTestPageURL());
  CreateSimpleNotification(browser(), true);
  ASSERT_EQ(1, GetNotificationCount());

  DenyOrigin(GetTestPageURL().GetOrigin());
  ContentSettingsForOneType settings;
  GetPrefsByContentSetting(CONTENT_SETTING_BLOCK, &settings);
  ASSERT_TRUE(CheckOriginInSetting(settings, GetTestPageURL().GetOrigin()));

  EXPECT_EQ(1, GetNotificationCount());
  message_center::NotificationList::Notifications notifications =
      message_center::MessageCenter::Get()->GetVisibleNotifications();
  message_center::MessageCenter::Get()->RemoveNotification(
      (*notifications.rbegin())->id(),
      true);  // by_user
  ASSERT_EQ(0, GetNotificationCount());
}

// Crashes on Linux/Win. See http://crbug.com/160657.
IN_PROC_BROWSER_TEST_F(
    NotificationsTest,
    DISABLED_TestOriginPrefsNotSavedInIncognito) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // Verify that allow/deny origin preferences are not saved in incognito.
  Browser* incognito = CreateIncognitoBrowser();
  ui_test_utils::NavigateToURL(incognito, GetTestPageURL());
  ASSERT_TRUE(RequestPermissionAndWait(incognito));
  PerformActionOnInfoBar(incognito, DENY, 0, 0);
  CloseBrowserWindow(incognito);

  incognito = CreateIncognitoBrowser();
  ui_test_utils::NavigateToURL(incognito, GetTestPageURL());
  ASSERT_TRUE(RequestPermissionAndWait(incognito));
  PerformActionOnInfoBar(incognito, ALLOW, 0, 0);
  CreateSimpleNotification(incognito, true);
  ASSERT_EQ(1, GetNotificationCount());
  CloseBrowserWindow(incognito);

  incognito = CreateIncognitoBrowser();
  ui_test_utils::NavigateToURL(incognito, GetTestPageURL());
  ASSERT_TRUE(RequestPermissionAndWait(incognito));

  ContentSettingsForOneType settings;
  GetPrefsByContentSetting(CONTENT_SETTING_BLOCK, &settings);
  EXPECT_EQ(0U, settings.size());
  GetPrefsByContentSetting(CONTENT_SETTING_ALLOW, &settings);
  EXPECT_EQ(0U, settings.size());
}

IN_PROC_BROWSER_TEST_F(NotificationsTest, TestIncognitoNotification) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // Test notifications in incognito window.
  Browser* browser = CreateIncognitoBrowser();
  ui_test_utils::NavigateToURL(browser, GetTestPageURL());
  browser->tab_strip_model()->ActivateTabAt(0, true);
  ASSERT_TRUE(RequestPermissionAndWait(browser));
  PerformActionOnInfoBar(browser, ALLOW, 0, 0);
  CreateSimpleNotification(browser, true);
  ASSERT_EQ(1, GetNotificationCount());
}

IN_PROC_BROWSER_TEST_F(NotificationsTest, TestCloseTabWithPermissionInfobar) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // Test that user can close tab when infobar present.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(),
      GURL("about:blank"),
      NEW_BACKGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_TAB);
  browser()->tab_strip_model()->ActivateTabAt(0, true);
  ui_test_utils::NavigateToURL(browser(), GetTestPageURL());
  ASSERT_TRUE(RequestPermissionAndWait(browser()));
  content::WebContentsDestroyedWatcher destroyed_watcher(
      browser()->tab_strip_model()->GetWebContentsAt(0));
  browser()->tab_strip_model()->CloseWebContentsAt(0,
                                                   TabStripModel::CLOSE_NONE);
  destroyed_watcher.Wait();
}

IN_PROC_BROWSER_TEST_F(
    NotificationsTest,
    TestNavigateAwayWithPermissionInfobar) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // Test navigating away when an infobar is present,
  // then trying to create a notification from the same page.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(),
      GURL("about:blank"),
      NEW_BACKGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_TAB);
  browser()->tab_strip_model()->ActivateTabAt(0, true);
  ui_test_utils::NavigateToURL(browser(), GetTestPageURL());
  ASSERT_TRUE(RequestPermissionAndWait(browser()));
  ui_test_utils::NavigateToURL(browser(), GetTestPageURL());
  ASSERT_TRUE(RequestPermissionAndWait(browser()));
  PerformActionOnInfoBar(browser(), ALLOW, 0, 0);
  CreateSimpleNotification(browser(), true);
  ASSERT_EQ(1, GetNotificationCount());
}

// See crbug.com/248470
#if defined(OS_LINUX)
#define MAYBE_TestCrashRendererNotificationRemain \
    DISABLED_TestCrashRendererNotificationRemain
#else
#define MAYBE_TestCrashRendererNotificationRemain \
    TestCrashRendererNotificationRemain
#endif

IN_PROC_BROWSER_TEST_F(NotificationsTest,
                       MAYBE_TestCrashRendererNotificationRemain) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // Test crashing renderer does not close or crash notification.
  AllowAllOrigins();
  ui_test_utils::NavigateToURLWithDisposition(
      browser(),
      GURL("about:blank"),
      NEW_BACKGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_TAB);
  browser()->tab_strip_model()->ActivateTabAt(0, true);
  ui_test_utils::NavigateToURL(browser(), GetTestPageURL());
  CreateSimpleNotification(browser(), true);
  ASSERT_EQ(1, GetNotificationCount());
  CrashTab(browser(), 0);
  ASSERT_EQ(1, GetNotificationCount());
}

IN_PROC_BROWSER_TEST_F(NotificationsTest, TestNotificationReplacement) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // Test that we can replace a notification using the replaceId.
  AllowAllOrigins();

  ui_test_utils::NavigateToURL(browser(), GetTestPageURL());

  std::string result = CreateNotification(
      browser(), true, "abc.png", "Title1", "Body1", "chat");
  EXPECT_NE("-1", result);

  ASSERT_EQ(1, GetNotificationCount());

  result = CreateNotification(
      browser(), false, "no_such_file.png", "Title2", "Body2", "chat");
  EXPECT_NE("-1", result);

  ASSERT_EQ(1, GetNotificationCount());
  message_center::NotificationList::Notifications notifications =
      message_center::MessageCenter::Get()->GetVisibleNotifications();
  EXPECT_EQ(base::ASCIIToUTF16("Title2"), (*notifications.rbegin())->title());
  EXPECT_EQ(base::ASCIIToUTF16("Body2"),
            (*notifications.rbegin())->message());
}

IN_PROC_BROWSER_TEST_F(NotificationsTest, TestLastUsage) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  HostContentSettingsMap* settings_map =
      browser()->profile()->GetHostContentSettingsMap();
  base::SimpleTestClock* clock = new base::SimpleTestClock();
  settings_map->SetPrefClockForTesting(scoped_ptr<base::Clock>(clock));
  clock->SetNow(base::Time::UnixEpoch() + base::TimeDelta::FromSeconds(10));

  // Creates a simple notification.
  AllowAllOrigins();
  ui_test_utils::NavigateToURL(browser(), GetTestPageURL());

  std::string result = CreateSimpleNotification(browser(), true);
  EXPECT_NE("-1", result);

  EXPECT_EQ(settings_map->GetLastUsage(GetTestPageURL().GetOrigin(),
                                       GetTestPageURL().GetOrigin(),
                                       CONTENT_SETTINGS_TYPE_NOTIFICATIONS)
                .ToDoubleT(),
            10);

  clock->Advance(base::TimeDelta::FromSeconds(3));

  result = CreateSimpleNotification(browser(), true);
  EXPECT_NE("-1", result);

  EXPECT_EQ(settings_map->GetLastUsage(GetTestPageURL().GetOrigin(),
                                       GetTestPageURL().GetOrigin(),
                                       CONTENT_SETTINGS_TYPE_NOTIFICATIONS)
                .ToDoubleT(),
            13);
}

IN_PROC_BROWSER_TEST_F(NotificationsTest,
                       TestNotificationReplacementReappearance) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // Test that we can replace a notification using the tag, and that it will
  // cause the notification to reappear as a popup again.
  AllowAllOrigins();

  ui_test_utils::NavigateToURL(browser(), GetTestPageURL());

  ASSERT_EQ(0, GetNotificationPopupCount());

  std::string result = CreateNotification(
      browser(), true, "abc.png", "Title1", "Body1", "chat");
  EXPECT_NE("-1", result);

  ASSERT_EQ(1, GetNotificationPopupCount());

  message_center::NotificationList::Notifications notifications =
      message_center::MessageCenter::Get()->GetVisibleNotifications();
  message_center::MessageCenter::Get()->ClickOnNotification(
      (*notifications.rbegin())->id());

  ASSERT_EQ(0, GetNotificationPopupCount());

  result = CreateNotification(
      browser(), true, "abc.png", "Title2", "Body2", "chat");
  EXPECT_NE("-1", result);

  ASSERT_EQ(1, GetNotificationPopupCount());
}

IN_PROC_BROWSER_TEST_F(NotificationsTest, TestNotificationValidIcon) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());
  AllowAllOrigins();

  ui_test_utils::NavigateToURL(browser(), GetTestPageURL());
  ASSERT_EQ(0, GetNotificationPopupCount());

  std::string result = CreateNotification(
      browser(), true, "icon.png", "Title1", "Body1", "chat");
  EXPECT_NE("-1", result);

  message_center::NotificationList::PopupNotifications notifications =
      message_center::MessageCenter::Get()->GetPopupNotifications();
  ASSERT_EQ(1u, notifications.size());

  auto* notification = *notifications.rbegin();

  EXPECT_EQ(100, notification->icon().Width());
  EXPECT_EQ(100, notification->icon().Height());
}

IN_PROC_BROWSER_TEST_F(NotificationsTest, TestNotificationInvalidIcon) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());
  AllowAllOrigins();

  ui_test_utils::NavigateToURL(browser(), GetTestPageURL());
  ASSERT_EQ(0, GetNotificationPopupCount());

  // Not supplying an icon URL.
  std::string result = CreateNotification(
      browser(), true, "", "Title1", "Body1", "chat");
  EXPECT_NE("-1", result);

  message_center::NotificationList::PopupNotifications notifications =
      message_center::MessageCenter::Get()->GetPopupNotifications();
  ASSERT_EQ(1u, notifications.size());

  auto* notification = *notifications.rbegin();
  EXPECT_TRUE(notification->icon().IsEmpty());

  // Supplying an invalid icon URL.
  result = CreateNotification(
      browser(), true, "invalid.png", "Title1", "Body1", "chat");
  EXPECT_NE("-1", result);

  notifications = message_center::MessageCenter::Get()->GetPopupNotifications();
  ASSERT_EQ(1u, notifications.size());

  notification = *notifications.rbegin();
  EXPECT_TRUE(notification->icon().IsEmpty());
}

IN_PROC_BROWSER_TEST_F(NotificationsTest, TestNotificationDoubleClose) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());
  AllowAllOrigins();

  ui_test_utils::NavigateToURL(
      browser(), GetTestPageURLForFile("notification-double-close.html"));
  ASSERT_EQ(0, GetNotificationPopupCount());

  std::string result = CreateNotification(
      browser(), true, "", "Title1", "Body1", "chat");
  EXPECT_NE("-1", result);

  ASSERT_EQ(1, GetNotificationCount());
  message_center::NotificationList::Notifications notifications =
      message_center::MessageCenter::Get()->GetVisibleNotifications();
  message_center::MessageCenter::Get()->RemoveNotification(
      (*notifications.rbegin())->id(),
      true);  // by_user

  ASSERT_EQ(0, GetNotificationCount());

  // Calling WebContents::IsCrashed() will return FALSE here, even if the WC did
  // crash. Work around this timing issue by creating another notification,
  // which requires interaction with the renderer process.
  result = CreateNotification(browser(), true, "", "Title1", "Body1", "chat");
  EXPECT_NE("-1", result);
}
