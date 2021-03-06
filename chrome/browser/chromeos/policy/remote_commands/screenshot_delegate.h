// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_REMOTE_COMMANDS_SCREENSHOT_DELEGATE_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_REMOTE_COMMANDS_SCREENSHOT_DELEGATE_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/task_runner.h"
#include "chrome/browser/chromeos/policy/remote_commands/device_command_screenshot_job.h"
#include "chrome/browser/chromeos/policy/upload_job.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/snapshot/snapshot.h"
#include "url/gurl.h"

namespace policy {

// An implementation of the |DeviceCommandScreenshotJob::Delegate| that uses
// aura's GrabWindowSnapshotAsync() to acquire the window snapshot.
class ScreenshotDelegate : public DeviceCommandScreenshotJob::Delegate {
 public:
  explicit ScreenshotDelegate(
      scoped_refptr<base::TaskRunner> blocking_task_runner);
  ~ScreenshotDelegate() override;

  // DeviceCommandScreenshotJob::Delegate:
  bool IsScreenshotAllowed() override;
  void TakeSnapshot(
      gfx::NativeWindow window,
      const gfx::Rect& source_rect,
      const ui::GrabWindowSnapshotAsyncPNGCallback& callback) override;
  scoped_ptr<UploadJob> CreateUploadJob(const GURL& upload_url,
                                        UploadJob::Delegate* delegate) override;

 private:
  void StoreScreenshot(const ui::GrabWindowSnapshotAsyncPNGCallback& callback,
                       scoped_refptr<base::RefCountedBytes> png_data);

  scoped_refptr<base::TaskRunner> blocking_task_runner_;

  base::WeakPtrFactory<ScreenshotDelegate> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ScreenshotDelegate);
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_REMOTE_COMMANDS_SCREENSHOT_DELEGATE_H_
