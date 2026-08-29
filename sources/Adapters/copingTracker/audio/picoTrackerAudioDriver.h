/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#ifndef _PICOTRACKERAUDIO_DRIVER_H_
#define _PICOTRACKERAUDIO_DRIVER_H_

#include "Foundation/T_Singleton.h"
#include "Services/Audio/AudioDriver.h"

#define MINI_BLANK_SIZE 128 // Samples

class picoTrackerAudioDriver : public AudioDriver {
public:
  picoTrackerAudioDriver(AudioSettings &settings);
  virtual ~picoTrackerAudioDriver();

  // Sound implementation
  virtual bool InitDriver();
  virtual void CloseDriver();
  virtual bool StartDriver();
  virtual void StopDriver();
  virtual int GetPlayedBufferPercentage();
  virtual int GetSampleRate() {
    return 44100;
  };
  virtual bool Interlaced() {
    return true;
  };

  // Additional
  void OnChunkDone();
  void SetVolume(int v);
  int GetVolume();
  // Patch the I2S PIO program's SET Y immediates to change the output level
  // without requiring a reboot. level matches the LINEOUT config values:
  //   0 = default (quietest, sensitive headphones)
  //   1 = HP high volume
  //   2 = line level
  void SetAudioLevel(int level);
  virtual double GetStreamTime();
  static void IRQHandler();
  static void BufferNeeded();

  // Core1 hand-off for project loading. Safe to call only when playback is
  // stopped (player->Stop() already guarantees this before load begins).
  static void SuspendAudioThreadForLoad();
  static void ResumeAudioThread();

private:
  static picoTrackerAudioDriver *instance_;

  AudioSettings settings_;
  static const char miniBlank_[MINI_BLANK_SIZE * 2 * sizeof(int16_t)];
  int volume_;
  uint32_t startTime_;
  unsigned int pioOffset_;
};
#endif
