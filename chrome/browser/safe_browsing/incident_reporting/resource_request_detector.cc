// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/incident_reporting/resource_request_detector.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/safe_browsing/incident_reporting/incident_receiver.h"
#include "chrome/browser/safe_browsing/incident_reporting/resource_request_incident.h"
#include "chrome/common/safe_browsing/csd.pb.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/site_instance.h"
#include "crypto/sha2.h"
#include "net/url_request/url_request.h"
#include "url/gurl.h"

namespace {

const char* const kScriptHashes[] = {
    "\x2b\x45\xc0\xda\x79\x4c\x65\x19\x4d\x78\x98\x85\x6c\xe8\xbd\x95"
    "\xf5\x9a\x5a\xf4\x4e\xf9\x9f\x4f\x93\x35\x3b\xa8\x52\xc0\x02\xfb",
    "\x05\xce\x5b\xda\xff\x28\x17\xf9\xc9\x38\x62\x6a\x39\x1b\x76\x56"
    "\xe3\xef\xed\x48\x1f\xe3\xae\x93\x4f\xd3\xd2\x96\x87\x53\x45\xf0",
    "\x3a\x65\x3d\x71\x2d\x3a\xc4\x35\x10\xd7\x01\xb6\xbb\xfb\x49\xda"
    "\x12\xce\x09\xfd\x48\x45\x76\x64\x12\xff\xd4\x7c\x61\x47\x3c\x0b",
    "\x95\x8e\x4d\x65\xac\xea\x96\xe5\x11\xd9\xfa\xcc\xcc\xb7\xcd\xb3"
    "\xcb\x8f\x4d\xf0\xf8\x72\xa0\xc5\x87\x02\xad\xe4\x1f\x3d\xfa\xf2",
    "\x2b\x8f\x58\x38\xeb\x87\x5d\xa0\x14\x90\x95\x89\x04\xd3\xe5\x89"
    "\xc5\xd7\x7a\xb6\x48\x53\x18\xfe\x71\x3a\x6a\xfd\xf0\xb3\x6e\xa8",
    "\xa9\x18\x65\x4d\xd3\xf5\xdf\x09\xf6\xe7\xfe\x21\x0f\x11\x35\x9a"
    "\x53\xbf\xb4\xa8\x5e\x23\xb1\x0c\x3c\x64\x94\xf5\x08\x9b\x29\x15",
    "\x5a\x2b\x9b\x45\x81\x5c\x4b\xa5\xf5\x9b\x54\x78\x21\x73\x79\x87"
    "\x37\xdb\x88\x97\xd9\x76\xd9\x21\x80\xfc\x54\x83\x77\xdb\x17\x7f",
    "\xd5\xab\x93\xdc\x3a\xd2\x40\xee\x77\x82\x12\x5c\xf7\x7f\x91\x5c"
    "\x56\x62\x17\xbb\x4e\x6a\xb8\x38\x62\x9d\x0a\xbe\xd3\x8f\x50\xdb",
    "\xdb\x73\x50\xd3\x58\x50\x2e\xfc\x00\xca\xef\x9d\x68\xf4\xb5\x77"
    "\x2b\x00\xf9\x7d\xf8\x89\x96\x6e\x35\x22\x17\x35\x4d\xb2\x89\xb3",
    "\xdc\xf1\x0b\xb3\x29\x98\xac\x40\x24\x16\x09\x4b\x50\x3c\xe2\xa7"
    "\x7f\xde\x5f\xdf\x76\x4a\x29\x54\xbc\x49\xd6\x67\x11\x92\x16\xdf",
    "\x98\x28\x26\x7a\xa9\xc9\x8b\xab\xd6\x64\xe4\xd6\x89\x70\x67\x97"
    "\x84\x37\x92\x8b\x1b\xa4\xdf\x4f\x49\xc9\x0a\x12\x15\xff\x6e\x91",
    "\x30\xa5\x65\x41\xaf\x60\x9d\x2a\x84\x38\x98\xf0\x41\xa9\x4f\x97"
    "\xbd\x39\x20\xad\x94\x3a\x0b\x3e\x43\xa4\xe1\x91\x90\x9f\xdf\x25",
    "\x7b\x48\x72\x6d\x40\xc1\x2f\xac\xf7\x9f\x73\x84\xc5\x2a\x7a\x98"
    "\x6e\x98\x87\xb7\xe0\x65\xbd\x12\xc6\x27\x89\x56\x87\x3d\x36\x47",
    "\x7b\x57\x48\xde\x08\x7e\x8e\xba\xe9\x61\xa8\xec\xa9\x14\x70\xeb"
    "\x6f\x70\x3d\xd7\xb7\x73\x4b\x9e\x1c\x01\x80\x39\x64\x6a\x1e\xee",
    "\x1d\x86\xb8\x5a\x0e\x22\x41\xac\xbf\x7b\x35\x26\x89\x98\x46\x1e"
    "\x9d\xc2\x59\x6c\x33\xe3\xb7\x63\xed\x29\xf9\x49\x2c\xec\x93\xb5",
    "\x2e\xf3\x04\xd3\x5d\x4b\x58\xc7\x2f\x8b\xb8\xe9\x77\x01\xa8\x78"
    "\x1b\x4e\xea\x16\xca\x86\xdb\x76\x04\x8e\xc6\x84\x10\x15\x3c\xe6",
    "\xec\x06\x16\xaa\xdc\x96\xe4\xbb\xf9\x76\xb4\x4c\x6e\x1c\x7a\x55"
    "\xc6\x6f\x15\x00\x2e\xc7\x5d\xbe\x81\x6b\x74\x00\xe6\x29\x8e\x4e",
    "\xba\x4b\xce\xb5\x52\x2b\x0a\xc6\x13\x87\x56\xd2\x2d\x80\x6f\x77"
    "\x5a\x9d\x7d\x24\x04\xfd\x41\xe4\x3a\x1a\xd3\xcf\x76\xf5\x21\x4b",
    "\xaa\xab\xfd\x8d\x8a\x43\x9b\x99\x98\xad\x01\xec\xc5\xbb\x40\x80"
    "\x78\x44\xe4\xec\x44\x94\x5f\xe2\xb2\xc2\xd3\x87\xe1\x21\xd0\x1f",
    "\x78\x64\x83\x81\xca\x8f\x08\x92\xd2\x95\x36\xab\x77\xff\xcb\xf4"
    "\xb9\x5c\xc0\xa1\xd7\xfa\xf2\x6e\x6c\xa0\xc5\xfb\xe1\x49\x4a\x7e",
    "\x91\x1e\x2b\xb9\x6b\x12\x32\xc3\x74\xab\xf1\x6b\xaf\xfa\x40\x1c"
    "\x25\x50\x3f\x2f\x6e\x25\x95\x09\x5f\x7e\xc4\x91\x56\x56\xbd\x34",
    "\xa1\xaf\x68\xf8\xdc\x2d\x52\x6a\xe8\xd2\x13\xcd\x73\x05\xf7\x3e"
    "\xb1\x8b\x52\xb1\x69\xea\x64\x24\x2c\x79\x76\x81\x11\x9d\xa0\x71",
    "\x8e\x3c\xe6\x2f\xcb\xea\x7a\x1a\x31\x11\xa7\x52\xfd\x3f\x68\xca"
    "\x7b\xf0\x22\xd9\x6f\xd7\x21\x62\xe4\xb9\x05\x85\x93\xd0\xea\xfb",
    "\xab\x13\xfc\x28\x67\x26\xb0\x35\x93\x82\xba\x70\xda\x2d\xcc\xa9"
    "\x8e\x0b\xee\xd8\xd1\x93\x89\x9b\x53\x9f\xf8\x12\x83\x13\x95\x7d",
    "\xe9\x7a\x20\xc8\x98\x04\x34\xe9\x36\x9b\x9b\x3c\x19\x2b\xe0\xf5"
    "\xdf\xc7\x7f\x4e\x94\x1b\x8a\x0a\xf6\x35\xba\xef\xbc\x18\x79\x26",
    "\x24\x15\x42\x76\x4d\x29\xae\x4e\x1b\x2b\xd5\x8a\xdb\x85\x77\xea"
    "\xe6\xc4\x21\x26\x83\x17\x3e\x7f\xe1\xf4\xdc\xe8\xd1\xee\x38\xac",
    "\xbb\x44\xfe\x76\xeb\x37\x4f\x4e\xd2\x99\x70\x9e\x20\x7f\x08\x30"
    "\xec\x7b\xe9\x3a\x59\x81\x82\x3e\x45\x01\x41\x8d\xe5\x32\x74\x68",
    "\x5a\x18\x08\xb9\xb8\xc3\x16\x5f\x4b\x96\x6a\x81\x4f\xeb\xc1\xe0"
    "\x44\x05\xf5\xea\xa9\x34\xeb\xaa\x7e\x97\xd1\xf1\xd4\xd3\x9c\x30",
    "\xac\x93\xea\x0d\xd5\xdb\xa4\xe9\x2f\xa2\xdd\x1a\x49\x4b\xdb\x54"
    "\x8a\xb0\x93\x2f\x6d\x48\x54\x39\x30\xf1\x8c\x89\x87\xf2\x4b\x97",
    "\x90\x55\x4d\xe7\xcc\x8f\x6f\x3a\xa5\xf9\x90\xb7\x22\xf8\xe6\xf9"
    "\x33\x9e\xb6\x2d\x47\x97\x42\x3c\xd7\x5f\x89\x1e\x32\xb9\xcc\x59",
    "\xdf\xb0\xe0\x83\xfd\xd1\x3f\x0b\xad\xd6\x08\x9d\x47\x91\x10\xba"
    "\x59\xdc\x87\xd3\x68\xf1\x5c\xdc\x64\xf9\xdd\xf0\xe8\xd5\xdd\x02",
    "\x3a\xa0\x93\x8c\x7c\x7f\x9b\x9a\x2a\x87\x60\x6d\xd5\x73\x6d\xa4"
    "\xc6\xac\x84\x07\x68\xba\x43\x94\x24\x1f\x9c\x5f\x1b\x87\x54\x82",
    "\x76\x5f\xad\xc9\xb6\x00\xf0\x28\x37\x3e\xbe\xfb\x35\x2b\x95\xac"
    "\xc3\x54\x09\x2b\x04\x72\x92\xbb\x3a\x6e\x5c\x78\xb4\xa8\x87\x58",
    "\xec\x33\xf1\x38\x85\xf0\x1c\x1e\xee\xca\x05\x2d\x9b\xd3\x4f\x8a"
    "\x54\x6b\x91\x36\x10\x64\xf6\x64\xbe\x1d\xf4\xa5\xa1\x22\x8e\x97",
    "\x75\xa3\xd3\x53\xb0\x57\xbe\x92\x9c\xf5\xf9\xc1\x30\x95\x10\xee"
    "\x93\xc0\x4e\x48\x9d\x4a\xa1\x8d\x40\xe5\xa2\x42\xd7\xf2\xc2\x77",
    "\x21\x66\x33\xff\xc3\xfa\xe1\x7a\xa1\x06\xf2\x9e\x2f\xc6\xcc\x93"
    "\x1e\x62\x17\xf1\xcc\x02\x2f\x39\x80\xee\x34\x4a\x85\xc8\x99\xed",
    "\x9b\x62\xc6\x2b\xc9\xb0\xf9\xbd\x93\x1a\xfd\xed\xfb\x68\xa0\xc2"
    "\x15\xfe\x34\xea\xc4\x89\x73\x9e\x70\x93\xe1\x1f\x4a\x75\xbe\x09",
    "\xb9\xe4\x66\x44\xea\x77\xe1\x74\x3d\x92\xcf\x6c\x20\x7e\xbf\x46"
    "\xfd\x4f\x4e\x82\x17\xa8\x7d\x3d\x19\xd4\xda\xde\x75\x74\xf1\x13",
    "\x8b\x2e\x30\xfa\x2e\xe1\xa1\x8e\xb6\x00\xb9\xe3\xc2\xc9\xa4\xad"
    "\x70\x03\x72\xea\xa8\x68\xdc\x95\x43\x6d\xdf\x40\x26\x58\xde\xe6",
};

const char* const kDomainHashes[] = {
    "\x1e\x11\x37\x30\xc2\x8a\xf5\xde\xac\x4c\xf3\x6b\x45\xbf\xc2\x64"
    "\x86\x73\x44\xad\xb5\x81\xb0\xc8\x54\x58\x6e\x6b\x6f\x92\x50\xc9",
    "\xac\xc0\x51\x88\x40\xfe\xdd\x9b\x02\x5b\x58\x8a\xe7\x19\x58\xaa"
    "\x45\xb9\x19\x7e\x8a\xf0\xd0\xa8\x2a\x53\x6e\xc4\x38\x31\xc9\x96",
    "\x2b\xbe\xdf\x89\x33\x2c\xe4\xc7\xcf\xca\x65\xfb\x91\x1c\x9d\x3a"
    "\x4e\x51\xbe\x56\xe3\xfa\x2c\x32\x78\x6b\x90\x03\x68\xf4\x3f\xc5",
    "\x5b\x81\x16\xa0\xce\xa4\x6d\x57\xbd\x38\x7f\xd0\x85\x25\x59\x53"
    "\xaf\x46\xf8\x24\x44\xde\x6e\x3e\x24\x96\x97\x9a\x7c\x53\xbc\xdf",
    "\x07\x9e\x8d\xe6\x1e\x5e\xb8\x35\x24\x84\x0f\xd9\x08\x2a\x99\xf3"
    "\x28\x73\xac\x7b\x67\x01\x33\xa3\x49\xf8\xad\xb7\xef\xc6\xb4\xb8",
    "\x9e\xb5\x08\x1e\x63\x1a\x76\xb1\x32\x6f\xf1\xf7\xad\x31\xbf\xf8"
    "\xa1\x65\x4a\x90\x6d\x08\xc5\xb4\xca\xb5\x7a\x83\xc9\xbf\x2f\xcc",
    "\x8e\xc5\xf8\x8f\x1e\x16\x5a\x6c\x32\x89\x03\xca\x57\xd2\x5b\xda"
    "\x90\xac\x27\x87\x8d\x31\x0d\x3e\xae\x23\xa9\xfd\x90\x3a\xca\x44",
    "\xae\xad\x0e\x56\xa8\x15\x77\xfd\x7e\x57\x31\x73\x09\xd0\x64\x17"
    "\x39\xdb\x81\x5f\x21\x9a\x68\x7c\x93\x31\xd6\x08\x44\x9e\xe0\x8c",
    "\xe9\x50\x69\xc7\xfe\xd2\x6b\xc6\x07\xd5\x0e\x4d\x66\x0f\xf7\x7e"
    "\xc8\xdd\xb8\xba\xdd\x77\x24\x50\x22\x4a\xfe\xb0\x17\x6c\x97\x70",
    "\x2a\xa2\xd3\xaa\x45\x98\xf7\x02\x21\x25\xc0\xe2\x8d\x56\x57\xe5"
    "\xc5\x50\x63\x86\x1a\x31\xfd\xae\x68\x63\x68\x60\x97\xaf\x70\xb9",
    "\xb3\xc9\x4e\x79\x0b\x34\xec\x92\xba\x62\x6d\x0a\x1a\xe8\xb8\xed"
    "\xf6\x32\xb6\x46\xeb\x48\x12\xa2\x7c\x97\x8c\x01\x5f\xab\x00\xf1",
    "\xb1\x46\x39\xdc\x41\x12\xdf\x27\x41\x20\x0c\x29\x34\xc0\x76\x3f"
    "\xdc\xfa\x19\x4d\x76\xfe\x7b\xce\x0e\x22\x00\x36\x0d\xc8\xaa\x61",
    "\xfb\x3a\xc8\xdc\x0e\x89\xa0\x6a\xf5\xe4\x6d\x8b\x47\x05\xdb\x0b"
    "\x27\xeb\x15\x41\x14\xdc\xbc\xa1\x3a\x63\x10\xc2\xb6\x28\xcd\xc9",
    "\x98\xa0\x19\x03\x97\x3b\xee\x5b\x7d\x11\xde\xa4\xd2\x07\x58\xa0"
    "\x5d\x4a\x45\x85\x95\x5d\xd5\x82\x74\x12\x64\xbf\x7a\x3d\x84\x84",
    "\xc9\x05\x29\x1e\x3f\x37\x68\x4a\xac\x50\x36\x0b\xc8\x31\x4d\x5c"
    "\xa7\x3b\x3d\x5c\x1b\xeb\xd3\xcc\xbb\x9e\x74\x64\x69\x42\x23\x6c",
    "\xe9\x68\xe5\x82\xc8\xb6\x78\xc4\xb2\xcc\xfa\xa2\xd2\x6c\x58\x89"
    "\x59\x41\xee\x98\x25\x64\xd4\x12\x59\x81\x2c\xea\xa6\xd3\x23\xd8",
    "\x7f\xd8\x3f\x84\x70\xfd\x08\x9b\xe6\x66\x65\x77\x4a\x0e\x20\x25"
    "\xc9\x9a\xc0\x6c\x12\x82\x00\x08\x4a\x62\xe8\x1c\xa7\xb3\x90\x07",
    "\xaa\x45\x3b\x66\xab\x46\x95\x21\x92\x5f\x7c\xc3\xab\xa3\x3e\x5e"
    "\x23\x14\x4a\x50\xfa\x5d\xb8\xf5\x25\x29\x42\x23\x6c\x23\x95\xeb",
    "\xf9\xcf\x8a\x1c\xc0\x7f\x38\x8d\x20\x5d\xe9\x88\x00\xdf\x6b\xb3"
    "\xc4\x39\xa4\x4f\x61\x65\x6e\x43\x35\x54\x2c\x15\x50\xc3\xa3\x21",
    "\xc4\x1b\x1a\x9d\xdd\x18\xd3\xb7\xdd\x2c\x02\x07\xfd\x63\x3b\x53"
    "\x7b\xe0\x1d\x17\xcf\x15\xc9\x25\xa8\x76\xd1\x41\x9e\x62\x34\x0a",
    "\xc3\xeb\x5e\x05\x55\x1e\x63\xe9\x6e\xa7\x98\x92\xd7\x3b\x45\xe1"
    "\x5f\xbc\xc4\xf0\x2f\xb1\x9f\xbf\x4b\x1f\xe5\xdd\xde\x76\x2a\x77",
    "\xfc\xd4\xa8\x97\x50\x0d\xba\x15\xac\x3c\x2b\x6e\x2b\x79\x93\xcd"
    "\x18\x1a\xb1\xad\x32\x04\x27\x01\x39\xf7\x6d\x7a\x39\xb5\x92\x35",
    "\x97\x94\xec\x59\x45\xd8\xfe\xa3\x73\x1f\x03\xe6\xb2\xfc\x2e\xe8"
    "\xf7\x95\xe3\xaf\x8f\x97\x01\x6f\xef\x6b\x7b\xee\x41\x5e\x27\x7e",
    "\x75\xc1\x70\x94\x68\xf6\xcc\x07\xb7\xbe\x0b\x84\x0c\x64\xa8\x47"
    "\x4e\xea\x7f\x75\x3b\xcb\x28\x39\xab\xe5\x14\x8a\xb4\x5a\x38\xb2",
    "\x94\x48\xfd\x84\x30\xba\x7d\x81\x04\xdc\xbb\x16\xa1\x06\xa9\xe4"
    "\xb1\xa7\xff\xc5\x13\x22\xed\x4e\x05\xfe\xf9\xb8\x69\xfe\x23\xd4",
    "\xb5\x32\x33\x46\x6c\x29\xe2\x74\xa6\x63\x60\x70\xdb\x20\x15\x12"
    "\x0a\x67\xf0\x3a\xad\xf9\x0c\x33\x91\x4c\x90\x5c\x55\x92\x1f\xf8",
    "\x16\xe6\x9c\xdf\xa2\x18\x13\x60\xe4\x2b\xb3\x07\x29\xa8\xd8\x1b"
    "\xc5\xa8\xd1\x85\x42\x67\x57\x81\x55\x34\x97\x1d\x8c\xe9\xee\xb7",
    "\x28\x3f\x74\x64\xb2\x15\xfc\x1b\x75\xcd\x69\x88\x04\x1b\x27\x62"
    "\xd0\xc2\xdc\xbe\x31\xbe\xb5\x30\xa3\x6e\x01\xdd\x0f\x4e\x31\x2b",
    "\x75\xc2\x30\x5b\xa3\x9b\xff\x0d\xdc\x75\xdf\x20\x8e\xa1\xe6\x5c"
    "\x17\xab\xf0\x58\x06\xf3\xda\x9f\xa5\xaa\x98\xfe\x1a\x7e\x74\x2b",
    "\x3c\xc1\x60\xc5\xd0\x56\x0d\x08\xd5\x19\xbf\x08\x51\x18\x9b\xc8"
    "\xdd\x8d\x58\x5f\x1d\x75\x88\x14\x73\x8c\xda\x66\x12\x94\x8a\xeb",
    "\x54\xba\x7d\x21\x4e\x4e\xc2\xf3\x37\x37\x86\xcd\xbe\x7b\x89\x42"
    "\xa9\x7b\x3b\xec\x69\x49\x6c\x1c\x58\xb8\x4d\xe8\x06\x1c\x88\x37",
    "\x62\xef\x4d\x5f\xa4\x64\x80\xd6\x97\xd2\xd0\xbd\x31\x30\x03\x5f"
    "\x22\x37\x8d\x48\xdd\x8a\xb2\xf0\xe3\x57\x35\x98\x37\x70\x32\x25",
    "\x23\x93\xc0\xa1\xd4\x27\xbd\x64\x65\x86\xe1\xa4\x86\x99\x99\x47"
    "\x89\xf9\x69\xe2\xba\xce\x7c\x42\xc7\x5d\xbc\xe9\x14\x73\x1c\x8d",
    "\x10\xe5\x75\x6d\x09\x43\xb3\xca\x0d\x1b\x78\xd1\xc2\x1a\xe5\xc0"
    "\xd8\x29\x57\x86\x87\xe3\x43\x95\x87\xf6\x92\x83\x5e\x08\x4f\x7a",
    "\x1c\xf9\xec\x01\x62\xbe\x78\x9b\x0e\x42\x3b\x7e\x70\x47\x27\x46"
    "\x34\x52\x6e\x45\x1b\x60\x6e\xaf\xcb\x74\x8e\xdd\xbd\xe3\x4f\x5a",
    "\x62\x02\x40\x4d\x50\xd8\x2a\xd0\x67\xdc\xb5\xc7\xfc\x13\xe9\x66"
    "\x6a\x14\x33\x7e\xef\xf7\x20\x83\x4c\xf6\x32\xf4\x7a\x75\x32\xa1",
    "\x35\x89\xab\x5d\xeb\xd5\x4c\x3a\x0f\x34\xeb\x35\x39\x9d\x51\xda"
    "\x7c\x98\x40\xb7\xd4\xca\x5b\x5e\x3f\x82\x22\xbb\xd6\x56\x42\x78",
    "\x30\x91\xf8\x24\xa3\xb6\x66\xb0\xc5\xe6\xe0\xfc\xa8\xfc\x2c\x9f"
    "\x53\x09\x3f\xe5\x4f\x19\xab\xae\x09\xbc\x40\xa9\xd1\x37\x8e\x84",
    "\xfa\x5a\x2f\xf0\xb0\x3e\x81\xbb\x7b\x4b\xc0\xf0\x67\xf1\xbe\x9d"
    "\x86\x87\x51\xe6\x72\x34\x70\x02\xc2\xec\xb5\x66\xe7\xd1\x4d\x55",
    "\x10\x24\x54\x8f\xe4\x06\x49\x6b\x0f\xcf\x95\x5c\xf9\xa6\xdc\xa9"
    "\xc0\x7d\xda\xda\x78\x21\x57\x40\xdb\xb3\x54\x5f\x3b\x53\x48\xee",
    "\xf7\xf2\x47\x19\x6e\x7d\x14\x08\x4b\xf3\x6f\x5c\x40\x19\x11\x54"
    "\x68\xa5\x0d\xde\x6e\xba\x5e\x1b\x34\x04\x72\x41\x55\x31\xb1\x18",
    "\xb6\xfa\x48\xa8\xd7\x20\xde\x56\x8c\x90\x81\xac\xaf\xd8\xf2\xe6"
    "\xab\x56\xbb\x64\x1e\xbc\x93\x56\x3f\xce\xac\xd9\xa7\x4d\xa8\x40",
    "\xfb\x8b\x14\x2e\xa8\x6a\x77\xaf\x7c\x13\x8a\x38\x6b\xd9\xf1\xc8"
    "\x87\x63\x8d\x00\xe4\xac\xf2\x11\x4a\x1f\x39\x57\x1f\xa6\xca\xdf",
    "\xba\xad\xe8\xdb\x70\x80\x8d\xbd\x3c\xc7\x6b\xd6\x02\x6a\x41\x40"
    "\x62\x45\x7b\x18\x65\x94\xf3\x56\xc5\x24\x1e\xcb\x81\x8d\x45\x09",
    "\x8f\xd5\xf8\xd3\x29\x82\x94\x51\xa8\xe6\x3a\x9d\x3a\xc7\x51\xe1"
    "\xd3\x54\x32\xcb\x2c\x20\x98\x5a\x70\x04\x18\xfd\x49\x75\x85\x6e",
    "\x90\x73\x6e\x8e\xe9\x75\xdf\xc6\x7e\xe7\x00\xe4\x4d\xc7\x0f\x04"
    "\xe6\x58\x78\xa3\xbc\x98\x22\xb9\x38\xe0\xf0\x67\xe2\xa9\x8e\x1f",
    "\x99\x3b\x39\x8e\x69\x7f\x28\xfd\x09\x8d\xc9\xed\xf9\x57\x0e\x41"
    "\x1b\x41\x48\x40\x37\xf4\x77\xd3\x07\xbd\x82\xc6\xda\x16\xa8\xec",
    "\x6d\x57\xf2\xd8\xf9\x6a\x82\x76\x1d\xb6\x8a\xe8\xb6\x3a\xcc\xd4"
    "\x30\x59\xdd\xa6\x18\x64\xac\xd9\x83\x80\x7c\x75\x7a\xdf\x20\xfe",
    "\x33\x5a\x23\xb0\xde\xd3\x7f\xc2\x96\xb7\x2e\xd4\x8a\xdc\x65\x0e"
    "\xe6\x95\x6b\x41\xf0\xfe\xa0\xdf\xdf\x28\x73\xce\x6e\x1d\x79\x2d",
    "\x13\xe3\xbc\x23\xb3\xf2\x10\x76\x10\xe8\x83\x8b\x83\xf0\x5e\x8d"
    "\x4a\x8e\xf3\x98\x4d\x05\x03\x53\x69\xe0\xc0\x21\x9f\x69\x3f\x77",
    "\xba\x88\x57\x60\x31\x4c\xd9\x6b\x21\x3e\xa3\x88\xe7\x45\x6c\x41"
    "\x91\x66\xf2\x08\xd0\x89\xe6\x39\x68\x6c\xb8\x7a\xd7\x7d\x9f\x76",
    "\xcd\xd5\x93\x5a\xe2\xdb\xf3\x63\xeb\xfd\xd0\x88\x49\x7d\xf6\x29"
    "\xbf\x1f\xee\x3a\xda\xa1\x95\x38\x4d\xc3\x91\x21\xce\x01\xd1\x8d",
};

Profile* GetProfileForRenderProcessId(int render_process_id) {
  // How to get a profile from a RenderProcess id:
  // 1) Get the RenderProcessHost
  // 2) From 1) Get the BrowserContext
  // 3) From 2) Get the Profile.
  Profile* profile = nullptr;
  content::RenderProcessHost* render_process_host =
      content::RenderProcessHost::FromID(render_process_id);
  if (render_process_host) {
    content::BrowserContext* browser_context =
        render_process_host->GetBrowserContext();
    if (browser_context)
      profile = Profile::FromBrowserContext(browser_context);
  }
  return profile;
}

GURL GetUrlForRenderFrameId(int render_process_id, int render_frame_id) {
  content::RenderFrameHost* render_frame_host =
      content::RenderFrameHost::FromID(render_process_id, render_frame_id);
  if (render_frame_host)
    return render_frame_host->GetLastCommittedURL();
  return GURL();
}

}  // namespace

namespace safe_browsing {

ResourceRequestDetector::ResourceRequestDetector(
    scoped_ptr<IncidentReceiver> incident_receiver)
    : incident_receiver_(incident_receiver.Pass()),
      allow_null_profile_for_testing_(false),
      weak_ptr_factory_(this) {
  InitializeHashSets();
}

ResourceRequestDetector::~ResourceRequestDetector() {
}

void ResourceRequestDetector::OnResourceRequest(
    const net::URLRequest* request) {
  // Only look at actual net requests (e.g., not chrome-extensions://id/foo.js).
  if (!request->url().SchemeIsHTTPOrHTTPS())
    return;

  DetectDomainRequests(request);
  DetectScriptRequests(request);
}

void ResourceRequestDetector::DetectDomainRequests(
    const net::URLRequest* request) {
  const content::ResourceRequestInfo* request_info =
      content::ResourceRequestInfo::ForRequest(request);

  // Only detect non top-level requests.
  if (request_info->GetResourceType() == content::RESOURCE_TYPE_MAIN_FRAME)
    return;

  std::string domain_digest(crypto::kSHA256Length, '\0');
  crypto::SHA256HashString(request->url().host(), &domain_digest[0],
                           crypto::kSHA256Length);

  if (domain_set_.count(domain_digest)) {
    DVLOG(1) << "Domain detector match found.";

    scoped_ptr<ClientIncidentReport_IncidentData_ResourceRequestIncident>
        incident_data(
            new ClientIncidentReport_IncidentData_ResourceRequestIncident());
    incident_data->set_type(
        ClientIncidentReport_IncidentData_ResourceRequestIncident::TYPE_DOMAIN);
    incident_data->set_digest(domain_digest);

    // This next bit of work needs a profile, so has to happen on the UI
    // thread.
    int render_process_id = 0;
    int render_frame_id = 0;
    content::ResourceRequestInfo::GetRenderFrameForRequest(
        request, &render_process_id, &render_frame_id);

    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::Bind(&ResourceRequestDetector::ReportIncidentOnUIThread,
                   weak_ptr_factory_.GetWeakPtr(), render_process_id,
                   render_frame_id, base::Passed(&incident_data)));
  }
}

void ResourceRequestDetector::DetectScriptRequests(
    const net::URLRequest* request) {
  const content::ResourceRequestInfo* request_info =
      content::ResourceRequestInfo::ForRequest(request);

  if (request_info->GetResourceType() != content::RESOURCE_TYPE_SCRIPT)
    return;

  DVLOG(1) << "Script request: " << request->url().spec();

  std::string url(request->url().host() + request->url().path());
  std::string script_digest(crypto::kSHA256Length, '\0');
  crypto::SHA256HashString(url, &script_digest[0],
                           crypto::kSHA256Length);

  if (script_set_.count(script_digest)) {
    DVLOG(1) << "Script detector match found.";

    scoped_ptr<ClientIncidentReport_IncidentData_ResourceRequestIncident>
        incident_data(
            new ClientIncidentReport_IncidentData_ResourceRequestIncident());
    incident_data->set_type(
        ClientIncidentReport_IncidentData_ResourceRequestIncident::TYPE_SCRIPT);
    incident_data->set_digest(script_digest);

    // This next bit of work needs a profile, so has to happen on the UI
    // thread.
    int render_process_id = 0;
    int render_frame_id = 0;
    content::ResourceRequestInfo::GetRenderFrameForRequest(
        request, &render_process_id, &render_frame_id);

    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::Bind(&ResourceRequestDetector::ReportIncidentOnUIThread,
                   weak_ptr_factory_.GetWeakPtr(), render_process_id,
                   render_frame_id, base::Passed(&incident_data)));
  }
}

void ResourceRequestDetector::set_allow_null_profile_for_testing(
    bool allow_null_profile_for_testing) {
  allow_null_profile_for_testing_ = allow_null_profile_for_testing;
}

void ResourceRequestDetector::InitializeHashSets() {
  // Store a hashed set of decoded string hashes. Probably slower than a linear
  // search for this size list, but this is only temporary.
  for (const char* encoded_hash : kScriptHashes)
    script_set_.insert(std::string(encoded_hash, crypto::kSHA256Length));

  for (const char* encoded_hash : kDomainHashes)
    domain_set_.insert(std::string(encoded_hash, crypto::kSHA256Length));
}

void ResourceRequestDetector::ReportIncidentOnUIThread(
    int render_process_id,
    int render_frame_id,
    scoped_ptr<ClientIncidentReport_IncidentData_ResourceRequestIncident>
        incident_data) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  Profile* profile = GetProfileForRenderProcessId(render_process_id);
  if (profile || allow_null_profile_for_testing_) {
    // Add the URL obtained from the RenderFrameHost, if available.
    GURL host_url = GetUrlForRenderFrameId(render_process_id, render_frame_id);
    if (host_url.is_valid())
      incident_data->set_origin(host_url.GetOrigin().spec());

    incident_receiver_->AddIncidentForProfile(
        profile,
        make_scoped_ptr(new ResourceRequestIncident(incident_data.Pass())));
  }
}

}  // namespace safe_browsing
