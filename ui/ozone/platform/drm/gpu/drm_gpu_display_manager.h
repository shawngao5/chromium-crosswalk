// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_GPU_DRM_GPU_DISPLAY_MANAGER_H_
#define UI_OZONE_PLATFORM_DRM_GPU_DRM_GPU_DISPLAY_MANAGER_H_

#include "base/memory/scoped_vector.h"
#include "ui/ozone/common/gpu/ozone_gpu_message_params.h"

namespace ui {

class DrmDeviceManager;
class DrmDisplay;
class ScreenManager;

struct GammaRampRGBEntry;

class DrmGpuDisplayManager {
 public:
  DrmGpuDisplayManager(ScreenManager* screen_manager,
                       DrmDeviceManager* drm_device_manager);
  ~DrmGpuDisplayManager();

  // Returns a list of the connected displays. When this is called the list of
  // displays is refreshed.
  std::vector<DisplaySnapshot_Params> GetDisplays();

  // Takes/releases the control of the DRM devices.
  bool TakeDisplayControl();
  bool RelinquishDisplayControl();

  bool ConfigureDisplay(int64_t id,
                        const DisplayMode_Params& mode,
                        const gfx::Point& origin);
  bool DisableDisplay(int64_t id);
  bool GetHDCPState(int64_t display_id, HDCPState* state);
  bool SetHDCPState(int64_t display_id, HDCPState state);
  void SetGammaRamp(int64_t id, const std::vector<GammaRampRGBEntry>& lut);

 private:
  DrmDisplay* FindDisplay(int64_t display_id);

  // Notify ScreenManager of all the displays that were present before the
  // update but are gone after the update.
  void NotifyScreenManager(const std::vector<DrmDisplay*>& new_displays,
                           const std::vector<DrmDisplay*>& old_displays) const;

  ScreenManager* screen_manager_;  // Not owned.
  DrmDeviceManager* drm_device_manager_;  // Not owned.

  ScopedVector<DrmDisplay> displays_;

  DISALLOW_COPY_AND_ASSIGN(DrmGpuDisplayManager);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_GPU_DRM_GPU_DISPLAY_MANAGER_H_
