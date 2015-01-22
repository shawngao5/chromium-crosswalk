// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/run_loop.h"
#include "extensions/browser/api/printer_provider/printer_provider_api.h"
#include "extensions/common/extension.h"
#include "extensions/shell/test/shell_apitest.h"
#include "extensions/test/extension_test_message_listener.h"
#include "extensions/test/result_catcher.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using extensions::PrinterProviderAPI;

// Callback for PrinterProviderAPI::DispatchPrintRequested calls.
// It copies |value| to |*result| and runs |callback|.
void RecordPrintErrorAndRunCallback(PrinterProviderAPI::PrintError* result,
                                    const base::Closure& callback,
                                    PrinterProviderAPI::PrintError value) {
  *result = value;
  if (!callback.is_null())
    callback.Run();
}

// Tests for chrome.printerProvider API.
class PrinterProviderApiTest : public extensions::ShellApiTest {
 public:
  PrinterProviderApiTest() {}
  ~PrinterProviderApiTest() override {}

  void StartPrintRequest(const std::string& extension_id,
                         const PrinterProviderAPI::PrintCallback& callback) {
    PrinterProviderAPI::PrintJob job;
    job.printer_id = "printer_id";
    job.ticket_json = "{}";
    job.content_type = "content_type";
    job.document_bytes = "bytes";

    PrinterProviderAPI::GetFactoryInstance()
        ->Get(browser_context())
        ->DispatchPrintRequested(extension_id, job, callback);
  }

  // Loads chrome.printerProvider test app and initializes is for test
  // |test_param|.
  // When the app's background page is loaded, the app will send 'loaded'
  // message. As a response to the message it will expect string message
  // specifying the test that should be run. When the app initializes its state
  // (e.g. registers listener for a chrome.printerProvider event) it will send
  // message 'ready', at which point the test may be started.
  // If the app is successfully initialized, |*extension_id_out| will be set to
  // the loaded extension's id, otherwise it will remain unchanged.
  void InitializePrinterProviderTestApp(const std::string& app_path,
                                        const std::string& test_param,
                                        std::string* extension_id_out) {
    ExtensionTestMessageListener loaded_listener("loaded", true);
    ExtensionTestMessageListener ready_listener("ready", false);

    const extensions::Extension* extension = LoadApp(app_path);
    ASSERT_TRUE(extension);
    const std::string extension_id = extension->id();

    loaded_listener.set_extension_id(extension_id);
    ready_listener.set_extension_id(extension_id);

    ASSERT_TRUE(loaded_listener.WaitUntilSatisfied());

    loaded_listener.Reply(test_param);

    ASSERT_TRUE(ready_listener.WaitUntilSatisfied());

    *extension_id_out = extension_id;
  }

  // Runs a test for chrome.printerProvider.onPrintRequested event.
  // |test_param|: The test that should be run.
  // |expected_result|: The print result the app is expected to report.
  void RunPrintRequestTestApp(const std::string& test_param,
                              PrinterProviderAPI::PrintError expected_result) {
    extensions::ResultCatcher catcher;

    std::string extension_id;
    InitializePrinterProviderTestApp("api_test/printer_provider/request_print",
                                     test_param, &extension_id);
    if (extension_id.empty())
      return;

    base::RunLoop run_loop;
    PrinterProviderAPI::PrintError print_result;
    StartPrintRequest(extension_id,
                      base::Bind(&RecordPrintErrorAndRunCallback, &print_result,
                                 run_loop.QuitClosure()));

    ASSERT_TRUE(catcher.GetNextResult()) << catcher.message();

    run_loop.Run();
    EXPECT_EQ(expected_result, print_result);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(PrinterProviderApiTest);
};

IN_PROC_BROWSER_TEST_F(PrinterProviderApiTest, PrintJobSuccess) {
  RunPrintRequestTestApp("OK", PrinterProviderAPI::PRINT_ERROR_NONE);
}

IN_PROC_BROWSER_TEST_F(PrinterProviderApiTest, PrintJobAsyncSuccess) {
  RunPrintRequestTestApp("ASYNC_RESPONSE",
                         PrinterProviderAPI::PRINT_ERROR_NONE);
}

IN_PROC_BROWSER_TEST_F(PrinterProviderApiTest, PrintJobFailed) {
  RunPrintRequestTestApp("INVALID_TICKET",
                         PrinterProviderAPI::PRINT_ERROR_INVALID_TICKET);
}

IN_PROC_BROWSER_TEST_F(PrinterProviderApiTest, NoPrintEventListener) {
  RunPrintRequestTestApp("NO_LISTENER", PrinterProviderAPI::PRINT_ERROR_FAILED);
}

IN_PROC_BROWSER_TEST_F(PrinterProviderApiTest,
                       PrintRequestInvalidCallbackParam) {
  RunPrintRequestTestApp("INVALID_VALUE",
                         PrinterProviderAPI::PRINT_ERROR_FAILED);
}

}  // namespace
