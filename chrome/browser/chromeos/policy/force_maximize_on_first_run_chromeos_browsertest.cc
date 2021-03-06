// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "ash/display/display_manager.h"
#include "ash/shell.h"
#include "ash/test/display_manager_test_api.h"
#include "ash/wm/window_positioner.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/policy/login_policy_test_base.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/host_desktop.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/test_utils.h"
#include "policy/policy_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace policy {

// Boolean parameter is used to run this test for webview (true) and for
// iframe (false) GAIA sign in.
class ForceMaximizeOnFirstRunTest : public LoginPolicyTestBase,
                                    public testing::WithParamInterface<bool> {
 protected:
  ForceMaximizeOnFirstRunTest() : LoginPolicyTestBase() {
    set_use_webview(GetParam());
  }

  scoped_ptr<base::DictionaryValue> GetMandatoryPoliciesValue() const override {
    scoped_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
    dict->SetBoolean(key::kForceMaximizeOnFirstRun, true);
    return dict;
  }

  void SetUpResolution() {
    // Set a screen resolution for which the first browser window will not be
    // maximized by default.
    const int width =
        ash::WindowPositioner::GetForceMaximizedWidthLimit() + 100;
    // Set resolution to 1466x300.
    const std::string resolution = base::IntToString(width) + "x300";
    ash::DisplayManager* const display_manager =
        ash::Shell::GetInstance()->display_manager();
    ash::test::DisplayManagerTestApi display_manager_test_api(display_manager);
    display_manager_test_api.UpdateDisplay(resolution);
  }

  const Browser* OpenNewBrowserWindow() {
    const user_manager::User* const user =
        user_manager::UserManager::Get()->GetActiveUser();
    Profile* const profile =
        chromeos::ProfileHelper::Get()->GetProfileByUser(user);
    return CreateBrowser(profile);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ForceMaximizeOnFirstRunTest);
};

IN_PROC_BROWSER_TEST_P(ForceMaximizeOnFirstRunTest, PRE_TwoRuns) {
  SetUpResolution();
  SkipToLoginScreen();
  LogIn(kAccountId, kAccountPassword);

  // Check that the first browser window is maximized.
  const BrowserList* const browser_list =
      BrowserList::GetInstance(chrome::HOST_DESKTOP_TYPE_ASH);
  EXPECT_EQ(1U, browser_list->size());
  const Browser* const browser = browser_list->get(0);
  ASSERT_TRUE(browser);
  EXPECT_TRUE(browser->window()->IsMaximized());

  // Un-maximize the window as its state will be carried forward to the next
  // opened window.
  browser->window()->Restore();
  EXPECT_FALSE(browser->window()->IsMaximized());

  // Create a second window and check that it is not affected by the policy.
  const Browser* const browser1 = OpenNewBrowserWindow();
  ASSERT_TRUE(browser1);
  EXPECT_FALSE(browser1->window()->IsMaximized());
}

IN_PROC_BROWSER_TEST_P(ForceMaximizeOnFirstRunTest, TwoRuns) {
  SetUpResolution();
  content::WindowedNotificationObserver(
      chrome::NOTIFICATION_LOGIN_OR_LOCK_WEBUI_VISIBLE,
      content::NotificationService::AllSources()).Wait();
  LogIn(kAccountId, kAccountPassword);

  const Browser* const browser = OpenNewBrowserWindow();
  ASSERT_TRUE(browser);
  EXPECT_FALSE(browser->window()->IsMaximized());
}

class ForceMaximizePolicyFalseTest : public ForceMaximizeOnFirstRunTest {
 protected:
  ForceMaximizePolicyFalseTest() : ForceMaximizeOnFirstRunTest() {}

  scoped_ptr<base::DictionaryValue> GetMandatoryPoliciesValue() const override {
    scoped_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
    dict->SetBoolean(key::kForceMaximizeOnFirstRun, false);
    return dict;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ForceMaximizePolicyFalseTest);
};

IN_PROC_BROWSER_TEST_P(ForceMaximizePolicyFalseTest, GeneralFirstRun) {
  SetUpResolution();
  SkipToLoginScreen();
  LogIn(kAccountId, kAccountPassword);

  const BrowserList* const browser_list =
      BrowserList::GetInstance(chrome::HOST_DESKTOP_TYPE_ASH);
  EXPECT_EQ(1U, browser_list->size());
  const Browser* const browser = browser_list->get(0);
  ASSERT_TRUE(browser);
  EXPECT_FALSE(browser->window()->IsMaximized());
}

INSTANTIATE_TEST_CASE_P(ForceMaximizeOnFirstRunTestSuite,
                        ForceMaximizeOnFirstRunTest,
                        testing::Bool());

INSTANTIATE_TEST_CASE_P(ForceMaximizePolicyFalseTestSuite,
                        ForceMaximizePolicyFalseTest,
                        testing::Bool());
}  // namespace policy
