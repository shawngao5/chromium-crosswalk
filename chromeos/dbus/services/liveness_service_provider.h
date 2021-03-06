// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_SERVICES_LIVENESS_SERVICE_PROVIDER_H_
#define CHROMEOS_DBUS_SERVICES_LIVENESS_SERVICE_PROVIDER_H_

#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/services/cros_dbus_service.h"
#include "dbus/exported_object.h"

namespace dbus {
class MethodCall;
class Response;
}

namespace chromeos {

// This class exports a "CheckLiveness" D-Bus method that the session manager
// calls periodically to confirm that Chrome's UI thread is responsive to D-Bus
// messages.  It can be tested with the following command:
//
// % dbus-send --system --type=method_call --print-reply
//     --dest=org.chromium.LibCrosService
//     /org/chromium/LibCrosService
//     org.chromium.LibCrosServiceInterface.CheckLiveness
//
// -> method return sender=:1.9 -> dest=:1.27 reply_serial=2
//
// (An empty response should be returned.)
class CHROMEOS_EXPORT LivenessServiceProvider
    : public CrosDBusService::ServiceProviderInterface {
 public:
  LivenessServiceProvider();
  ~LivenessServiceProvider() override;

  // CrosDBusService::ServiceProviderInterface overrides:
  void Start(scoped_refptr<dbus::ExportedObject> exported_object) override;

 private:
  // Called from ExportedObject when CheckLiveness() is exported as a D-Bus
  // method or failed to be exported.
  void OnExported(const std::string& interface_name,
                  const std::string& method_name,
                  bool success);

  // Called on UI thread in response to a D-Bus request.
  void CheckLiveness(dbus::MethodCall* method_call,
                     dbus::ExportedObject::ResponseSender response_sender);

  // Keep this last so that all weak pointers will be invalidated at the
  // beginning of destruction.
  base::WeakPtrFactory<LivenessServiceProvider> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(LivenessServiceProvider);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_SERVICES_LIVENESS_SERVICE_PROVIDER_H_
