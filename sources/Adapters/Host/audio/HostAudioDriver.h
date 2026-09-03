/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#pragma once

#include "Services/Audio/AudioDriver.h"
#include <SDL2/SDL.h>
#include <chrono>
#include <mutex>
#include <queue>

class HostAudioDriver : public AudioDriver {
public:
  HostAudioDriver(AudioSettings &settings);
  virtual ~HostAudioDriver();

  virtual bool InitDriver() override;
  virtual void CloseDriver() override;
  virtual bool StartDriver() override;
  virtual void StopDriver() override;

  virtual bool Interlaced() override {
    return true;
  }
  virtual int GetPlayedBufferPercentage() override;
  virtual double GetStreamTime() override;

  static void SDLAudioCallback(void *userdata, uint8_t *stream, int len);
  void FillAudioBuffer(uint8_t *stream, int len);

private:
  SDL_AudioDeviceID device_id_;
  SDL_AudioSpec obtained_spec_;
  std::chrono::system_clock::time_point start_time_;
  int samples_played_;
  std::mutex mutex_;
  static HostAudioDriver *instance_;
};
