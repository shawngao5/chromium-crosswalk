// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_AUDIO_AUDIO_OBSERVER_H_
#define ASH_SYSTEM_AUDIO_AUDIO_OBSERVER_H_

#include <stdint.h>

namespace ash {

class AudioObserver {
 public:
  virtual ~AudioObserver() {}

  // Called when an active output device's volume changed.
  // |node_id|: id of the active node.
  // |volume|: volume as a percentage, 0.0 -- 100.0.
  virtual void OnOutputNodeVolumeChanged(uint64_t node_id, double volume) = 0;

  // Called when output mute state changed.
  virtual void OnOutputMuteChanged(bool mute_on) = 0;

  // Called when audio nodes changed.
  virtual void OnAudioNodesChanged() = 0;

  // Called when active audio output node changed.
  virtual void OnActiveOutputNodeChanged() = 0;

  // Called when active audio input node changed.
  virtual void OnActiveInputNodeChanged() = 0;
};

}  // namespace ash

#endif  // ASH_SYSTEM_AUDIO_AUDIO_OBSERVER_H_
