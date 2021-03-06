// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Audio test utilities.

// GetStats reports audio output energy in the [0, 32768] range.
var MAX_AUDIO_OUTPUT_ENERGY = 32768;

// Queries WebRTC stats on |peerConnection| to find out whether audio is playing
// on the connection. Note this does not necessarily mean the audio is actually
// playing out (for instance if there's a bug in the WebRTC web media player).
// If |beLenient| is true, we assume we're on a slow and unreliable bot and that
// we should do a minimum of checking.
function ensureAudioPlaying(peerConnection, beLenient) {
  addExpectedEvent();

  gatherAudioLevelSamples(peerConnection, 3 * 1000, function(samples) {
    identifyFakeDeviceSignal_(samples, beLenient);
    eventOccured();
  });
}

// Queries WebRTC stats on |peerConnection| to find out whether audio is muted
// on the connection.
function ensureSilence(peerConnection) {
  addExpectedEvent();
  setTimeout(function() {
    gatherAudioLevelSamples(peerConnection, 1 * 1000, function(samples) {
      identifySilence_(samples);
      eventOccured();
    });
  }, 500);
}

// Not sure if this is a bug, but sometimes we get several audio ssrc's where
// just reports audio level zero. Think of the nonzero level as the more
// credible one here. http://crbug.com/479147.
function workAroundSeveralReportsIssue(audioOutputLevels) {
  if (audioOutputLevels.length == 1) {
    return audioOutputLevels[0];
  }

  console.log("Hit issue where one report batch returns two or more reports " +
              "with audioReportLevel; got " + audioOutputLevels);

  return Math.max(audioOutputLevels[0], audioOutputLevels[1]);
}

// Gathers samples from WebRTC stats as fast as possible for |durationMs|
// milliseconds and calls back |callback| with an array with numbers in the
// [0, 32768] range. There are no guarantees for how often we will be able to
// collect values, but this function deliberately avoids setTimeout calls in
// order be as insensitive as possible to starvation (particularly when this
// code runs in parallel with other tests on a heavily loaded bot).
function gatherAudioLevelSamples(peerConnection, durationMs, callback) {
  console.log('Gathering audio samples for ' + durationMs + ' milliseconds...');
  var audioLevelSamples = []

  // If this times out and never found any audio output levels, the call
  // probably doesn't have an audio stream.
  var startTime = new Date();
  var gotStats = function(response) {
    audioOutputLevels = getAudioLevelFromStats_(response);
    if (audioOutputLevels.length == 0) {
      // The call probably isn't up yet.
      peerConnection.getStats(gotStats);
      return;
    }
    var outputLevel = workAroundSeveralReportsIssue(audioOutputLevels);
    audioLevelSamples.push(outputLevel);

    var elapsed = new Date() - startTime;
    if (elapsed > durationMs) {
      console.log('Gathered all samples.');
      callback(audioLevelSamples);
      return;
    }
    peerConnection.getStats(gotStats);
  }
  peerConnection.getStats(gotStats);
}

/**
* Tries to identify the beep-every-half-second signal generated by the fake
* audio device in media/video/capture/fake_video_capture_device.cc. Fails the
* test if we can't see a signal. The samples should have been gathered over at
* least two seconds since we expect to see at least three "peaks" in there
* (we should see either 3 or 4 depending on how things line up).
*
* If |beLenient| is specified, we assume we're running on a slow device or
* or under TSAN, and relax the checks quite a bit.
*
* @private
*/
function identifyFakeDeviceSignal_(samples, beLenient) {
  var numPeaks = 0;
  var threshold = MAX_AUDIO_OUTPUT_ENERGY * 0.7;
  if (beLenient)
    threshold = MAX_AUDIO_OUTPUT_ENERGY * 0.6;
  var currentlyOverThreshold = false;

  // Detect when we have been been over the threshold and is going back again
  // (i.e. count peaks). We should see about two peaks per second.
  for (var i = 0; i < samples.length; ++i) {
    if (currentlyOverThreshold && samples[i] < threshold)
      numPeaks++;
    currentlyOverThreshold = samples[i] >= threshold;
  }

  console.log('Number of peaks identified: ' + numPeaks);

  var expectedPeaks = 2;
  if (beLenient)
    expectedPeaks = 1;

  if (numPeaks < expectedPeaks)
    failTest('Expected to see at least ' + expectedPeaks + ' peak(s) in ' +
        'audio signal, got ' + numPeaks + '. Dumping samples for analysis: "' +
        samples + '"');
}

/**
 * @private
 */
function identifySilence_(samples) {
  var average = 0;
  for (var i = 0; i < samples.length; ++i)
    average += samples[i] / samples.length;

  // If silent (like when muted), we should get very near zero audio level.
  console.log('Average audio level: ' + average);
  if (average > 500)
    failTest('Expected silence, but avg audio level was ' + average);
}

/**
 * @private
 */
function getAudioLevelFromStats_(response) {
  var reports = response.result();
  var audioOutputLevels = [];
  for (var i = 0; i < reports.length; ++i) {
    var report = reports[i];
    if (report.names().indexOf('audioOutputLevel') != -1) {
      audioOutputLevels.push(report.stat('audioOutputLevel'));
    }
  }
  return audioOutputLevels;
}
