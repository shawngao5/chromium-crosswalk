// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Set up string assets.
loadTimeData.data = {
  DRIVE_DIRECTORY_LABEL: 'My Drive',
  DOWNLOADS_DIRECTORY_LABEL: 'Downloads'
};

// A providing extension which has mounted a file system, and doesn't support
// multiple mounts.
var MOUNTED_SINGLE_PROVIDING_EXTENSION = {
  extensionId: 'mounted-single-extension-id',
  name: 'mounted-single-extension-name',
  configurable: false,
  multipleMounts: false,
  source: 'device'
};

// A providing extension which has not mounted a file system, and doesn't
// support  multiple mounts.
var NOT_MOUNTED_SINGLE_PROVIDING_EXTENSION = {
  extensionId: 'not-mounted-single-extension-id',
  name: 'not-mounted-single-extension-name',
  configurable: false,
  multipleMounts: false,
  source: 'device'
};

// A providing extension which has mounted a file system and supports mounting
// more.
var MOUNTED_MULTIPLE_PROVIDING_EXTENSION = {
  extensionId: 'mounted-multiple-extension-id',
  name: 'mounted-multiple-extension-name',
  configurable: true,
  multipleMounts: true,
  source: 'network'
};

// A providing extension which has not mounted a file system but it's of "file"
// source. Such extensions do mounting via file handlers.
var NOT_MOUNTED_FILE_PROVIDING_EXTENSION = {
  extensionId: 'file-extension-id',
  name: 'file-extension-name',
  configurable: false,
  multipleMounts: true,
  source: 'file'
};

var volumeManager = null;

function addProvidedVolume(volumeManager, extensionId, volumeId) {
  var fileSystem = new MockFileSystem(volumeId, 'filesystem:' + volumeId);
  fileSystem.entries['/'] = new MockDirectoryEntry(fileSystem, '');

  var volumeInfo = new VolumeInfo(
      VolumeManagerCommon.VolumeType.PROVIDED,
      volumeId,
      fileSystem,
      '',           // error
      '',           // deviceType
      '',           // devicePath
      false,        // isReadonly
      {isCurrentProfile: true, displayName: ''},  // profile
      '',           // label
      extensionId,  // extensionId
      false);       // hasMedia

  volumeManager.volumeInfoList.push(volumeInfo);
}

function setUp() {
  // Create a dummy API for fetching a list of providing extensions.
  chrome.fileManagerPrivate = {
    getProvidingExtensions: function(callback) {
      callback([MOUNTED_SINGLE_PROVIDING_EXTENSION,
                NOT_MOUNTED_SINGLE_PROVIDING_EXTENSION,
                MOUNTED_MULTIPLE_PROVIDING_EXTENSION,
                NOT_MOUNTED_FILE_PROVIDING_EXTENSION]);
    }
  };
  chrome.runtime = {};

  // Create a dummy volume manager.
  volumeManager = new MockVolumeManagerWrapper();
  addProvidedVolume(volumeManager,
      MOUNTED_SINGLE_PROVIDING_EXTENSION.extensionId, 'volume-1');
  addProvidedVolume(volumeManager,
      MOUNTED_MULTIPLE_PROVIDING_EXTENSION.extensionId, 'volume-2');
}

function testGetInstalledProviders(callback) {
  var model = new ProvidersModel(volumeManager);
  reportPromise(model.getInstalledProviders().then(
      function(extensions) {
        assertEquals(4, extensions.length);
        assertEquals(MOUNTED_SINGLE_PROVIDING_EXTENSION.extensionId,
            extensions[0].extensionId);
        assertEquals(MOUNTED_SINGLE_PROVIDING_EXTENSION.name,
            extensions[0].extensionName);
        assertEquals(MOUNTED_SINGLE_PROVIDING_EXTENSION.configurable,
            extensions[0].configurable);
        assertEquals(MOUNTED_SINGLE_PROVIDING_EXTENSION.multipleMounts,
            extensions[0].multipleMounts);
        assertEquals(MOUNTED_SINGLE_PROVIDING_EXTENSION.source,
            extensions[0].source);

        assertEquals(NOT_MOUNTED_SINGLE_PROVIDING_EXTENSION.extensionId,
            extensions[1].extensionId);
        assertEquals(MOUNTED_MULTIPLE_PROVIDING_EXTENSION.extensionId,
            extensions[2].extensionId);
        assertEquals(NOT_MOUNTED_FILE_PROVIDING_EXTENSION.extensionId,
            extensions[3].extensionId);
  }), callback);
}

function testGetMountableProviders(callback) {
  var model = new ProvidersModel(volumeManager);
  reportPromise(model.getMountableProviders().then(
      function(extensions) {
        assertEquals(2, extensions.length);
        assertEquals(NOT_MOUNTED_SINGLE_PROVIDING_EXTENSION.extensionId,
            extensions[0].extensionId);
        assertEquals(MOUNTED_MULTIPLE_PROVIDING_EXTENSION.extensionId,
            extensions[1].extensionId);
  }), callback);
}
