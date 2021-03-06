// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_DECLARATIVE_USER_SCRIPT_MANAGER_H_
#define EXTENSIONS_BROWSER_DECLARATIVE_USER_SCRIPT_MANAGER_H_

#include <map>

#include "base/macros.h"
#include "base/memory/linked_ptr.h"
#include "base/scoped_observer.h"
#include "extensions/browser/extension_registry_observer.h"
#include "extensions/common/host_id.h"

namespace content {
class BrowserContext;
}

namespace extensions {
class DeclarativeUserScriptMaster;

// Manages a set of DeclarativeUserScriptMaster objects for script injections.
class DeclarativeUserScriptManager : public ExtensionRegistryObserver {
 public:
  explicit DeclarativeUserScriptManager(
      content::BrowserContext* browser_context);
  ~DeclarativeUserScriptManager() override;

  // Gets the user script master for declarative scripts by the given
  // HostID; if one does not exist, a new object will be created.
  DeclarativeUserScriptMaster* GetDeclarativeUserScriptMasterByID(
      const HostID& host_id);

 private:
  using UserScriptMasterMap =
      std::map<HostID, linked_ptr<DeclarativeUserScriptMaster>>;

  // ExtensionRegistryObserver:
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const Extension* extension,
                           UnloadedExtensionInfo::Reason reason) override;

  // Creates a DeclarativeUserScriptMaster object.
  DeclarativeUserScriptMaster* CreateDeclarativeUserScriptMaster(
      const HostID& host_id);

  // A map of DeclarativeUserScriptMasters for each host; each master
  // is lazily initialized.
  UserScriptMasterMap declarative_user_script_masters_;

  content::BrowserContext* browser_context_;

  ScopedObserver<ExtensionRegistry, ExtensionRegistryObserver>
      extension_registry_observer_;

  DISALLOW_COPY_AND_ASSIGN(DeclarativeUserScriptManager);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_DECLARATIVE_USER_SCRIPT_MANAGER_H_
