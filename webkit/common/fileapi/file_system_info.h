// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_COMMON_FILEAPI_FILE_SYSTEM_INFO_H_
#define WEBKIT_COMMON_FILEAPI_FILE_SYSTEM_INFO_H_

#include <string>

#include "url/gurl.h"
#include "webkit/common/fileapi/file_system_types.h"
#include "webkit/common/storage_common_export.h"

namespace storage {

// This struct is used to send the necessary information for Blink to create a
// DOMFileSystem.  Since Blink side only uses mount_type (rather than
// detailed/cracked filesystem type) this only contains mount_type but not type.
struct WEBKIT_STORAGE_COMMON_EXPORT FileSystemInfo {
  FileSystemInfo();
  FileSystemInfo(const std::string& filesystem_name,
                 const GURL& root_url,
                 storage::FileSystemType mount_type);
  ~FileSystemInfo();

  std::string name;
  GURL root_url;
  storage::FileSystemType mount_type;
};

}  // namespace storage

#endif  // WEBKIT_COMMON_FILEAPI_FILE_SYSTEM_INFO_H_
