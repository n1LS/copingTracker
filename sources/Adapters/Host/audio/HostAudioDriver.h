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
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

// The Host driver's SDL callback can request fragments far larger than one
// tempo-slice (e.g. 4096 samples vs. a ~918-sample slice at 120 BPM). Unlike
// the Pico (which only ever needs to stay one DMA buffer ahead of playback),
// the producer thread here must be able to render several slices ahead so a
// single large SDL callback can be filled without starving - i.e. without the
// tick/render cadence (see ProducerLoop()) ever having to wait on pool depth.
// This is deliberately decoupled from the Pico's SOUND_BUFFER_COUNT (2).
#define HOST_POOL_SIZE 16

class HostAudioDriver : public AudioDriver {
public:
  HostAudioDriver(AudioSettings &settings);
  virtual ~HostAudioDriver();

  virtual bool InitDriver() override;
  virtual void CloseDriver() override;
  virtual bool StartDriver() override;
  virtual void StopDriver() override;

  void AddBuffer(short *buffer, int samplecount) override;

  virtual bool Interlaced() override {
    return true;
  }
  virtual int GetPlayedBufferPercentage() override;
  virtual double GetStreamTime() override;
  static void SDLAudioCallback(void *userdata, uint8_t *stream, int len);
  void FillAudioBuffer(uint8_t *stream, int len);

private:
  void ProducerLoop();

  SDL_AudioDeviceID device_id_;
  SDL_AudioSpec obtained_spec_;
  std::chrono::system_clock::time_point start_time_;
  int samples_played_;
  double avg_samples_played = 0;
  int avg_sample_counter = 0;
  std::mutex mutex_;

  static AudioBufferData staticPool_[HOST_POOL_SIZE];

  // Renders playback slices ahead of time on a dedicated thread, mirroring
  // the Pico's core1 render thread that fills buffers ahead of the DMA IRQ
  // via a semaphore. Without this, rendering happened synchronously inside
  // the real-time SDL callback with zero lookahead, so any DSP render taking
  // longer than the callback's tight deadline caused audible
  // dropouts/underruns ("hollow" sound).
  std::thread producerThread_;
  std::atomic<bool> running_{false};
  std::mutex slotMutex_;
  std::condition_variable slotCv_;
  int freeSlots_ = 0;
  int queuedSamples_ = 0;

  static HostAudioDriver *instance_;
};
