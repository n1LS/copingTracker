/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2018 Discodirt
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#ifndef _MIXER_SERVICE_H_
#define _MIXER_SERVICE_H_

#include "Application/Commands/ApplicationCommandDispatcher.h" // Would be better done externally and call an API here
#include "DelayEffect.h"
#include "Foundation/Observable.h"
#include "Foundation/T_Singleton.h"
#include "MixBus.h"
#include "Metronome.h"
#include "Services/Audio/AudioMixer.h"
#include "Services/Audio/AudioOut.h"
#include "System/Process/SysMutex.h"

enum MixerServiceMode {
  MSM_AUDIO,
  MSM_FILE,
  MSM_FILESPLIT,
};

#define MAX_BUS_COUNT 10
// bus 0-7 are the song channels, 8 the file streamer
#define DELAY_MIX_BUS 9

// Render load: the time core1 needs to produce one slice of audio, as per mille
// of the time that slice takes to play. At 1000 the render is as slow as
// playback, so the next buffer arrives late and the DMA plays silence.
constexpr uint32_t RENDER_LOAD_WARNING = 80;  // amber from here
constexpr uint32_t RENDER_LOAD_CRITICAL = 95; // red from here
constexpr uint32_t RENDER_LOAD_HOLD_US = 2000000;

static_assert(SAMPLE_RATE_HZ % 100 == 0, "RenderLoadPercent needs a sample rate that is a multiple of 100");

// elapsed * 1000 / (samples / SAMPLE_RATE_HZ * 1000000), in integers. The
// elapsed time is limited so the product fits 32 bits.
constexpr uint32_t RenderLoadPercent(uint32_t elapsedUs, uint32_t samples) {
  if (samples == 0) {
    return 0;
  }
  if (elapsedUs > 90000) {
    elapsedUs = 90000;
  }
  return (elapsedUs * (SAMPLE_RATE_HZ / 100)) / (samples * 100);
}
// some test cases
static_assert(RenderLoadPercent(10000, 918) == 48, "10ms of a 20.8ms slice is 48%");
static_assert(RenderLoadPercent(20816, 918) == 99, "a render as long as the slice is ~100%");

class MixerService : public T_Singleton<MixerService>, public Observable, public I_Observer, public CommandExecuter {

public:
  MixerService();
  virtual ~MixerService();

  bool Init();
  void Close();

  bool Start();
  void Stop();

  MixBus *GetMixBus(int i);

  MasterBus *GetMasterBus() {
    return &master_;
  };

  DelayEffect *GetDelay() {
    return &delay_;
  };

  Metronome *GetMetronome() {
    return &metronome_;
  };

  virtual void Update(Observable &o, I_ObservableData *d);

  void OnPlayerStart(MixerServiceMode mode);
  void OnPlayerStop();

  void SetMasterVolume(int);
  int GetPlayedBufferPercentage();

  virtual void Execute(Token id, float value);

  AudioOut *GetAudioOut();
  MixerServiceMode GetMode() const {
    return mode_;
  }

  void Lock();
  void Unlock();

  // Highest render load of the last RENDER_LOAD_HOLD_US, in per mille (see
  // RenderLoadPermille). Written by core1, a plain aligned read is safe.
  uint32_t GetRenderLoad() {
    return loadPeak_;
  }

protected:
  void setRenderingMode(MixerServiceMode mode);

  // Helper function to convert linear volume (0-100) to non-linear (0.0-1.0) in
  // fixed point
  fixed ToLogVolume(int vol);

private:
  bool configureRenderPaths();
  void updateRenderLoad(uint32_t startUs);

  volatile uint32_t loadPeak_ = 0;
  uint32_t loadPeakTime_ = 0;

  AudioOut *out_;
  MasterBus master_;
  MixBus bus_[MAX_BUS_COUNT];
  DelayEffect delay_;
  Metronome metronome_;
  SysMutex *sync_;
  Project *project_; // Reference to the current project
  MixerServiceMode mode_ = MSM_AUDIO;
};
#endif
