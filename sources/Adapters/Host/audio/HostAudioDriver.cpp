/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "HostAudioDriver.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>

static int g_tickCount = 0;
static std::chrono::steady_clock::time_point g_tickStart;

HostAudioDriver *HostAudioDriver::instance_ = nullptr;

HostAudioDriver::HostAudioDriver(AudioSettings &settings) : AudioDriver(settings), device_id_(0), samples_played_(0) {
  instance_ = this;
}

HostAudioDriver::~HostAudioDriver() {
  CloseDriver();
}

bool HostAudioDriver::InitDriver() {
  // InitDriver() can be called more than once on the same instance (e.g. once
  // at boot via HostAudio::Init(), and again whenever MixerService::Init()
  // re-inits the shared AudioOutDriver for a newly loaded project). Without
  // this guard each call opened a brand new SDL audio device without closing
  // the previous one, leaving multiple SDL callback threads concurrently
  // racing on the same shared static buffer pool (AudioDriver::pool_) - this
  // is what made playback race far ahead of the correct tempo.
  CloseDriver();

  SDL_AudioSpec desired;
  SDL_zero(desired);
  desired.freq = 44100;
  desired.format = AUDIO_S16;
  desired.channels = 2;
  desired.samples = 1024;
  desired.callback = SDLAudioCallback;
  desired.userdata = this;

  device_id_ = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained_spec_, 0);
  if (device_id_ == 0) {
    return false;
  }
  fprintf(stderr, "AUDIODEBUG new device_id=%u\n", device_id_);

  start_time_ = std::chrono::system_clock::now();
  samples_played_ = 0;
  return true;
}

void HostAudioDriver::CloseDriver() {
  if (device_id_ != 0) {
    SDL_CloseAudioDevice(device_id_);
    device_id_ = 0;
  }
}

bool HostAudioDriver::StartDriver() {
  if (device_id_ == 0) {
    return false;
  }
  SDL_PauseAudioDevice(device_id_, 0);
  return true;
}

void HostAudioDriver::StopDriver() {
  if (device_id_ != 0) {
    SDL_PauseAudioDevice(device_id_, 1);
  }
}

int HostAudioDriver::GetPlayedBufferPercentage() {
  if (device_id_ == 0)
    return 0;
  int queued = SDL_GetQueuedAudioSize(device_id_);
  int max = obtained_spec_.samples * obtained_spec_.channels * 2;
  if (max <= 0)
    return 0;
  return std::max(0, 100 - (queued * 100 / max));
}

double HostAudioDriver::GetStreamTime() {
  auto now = std::chrono::system_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
  return duration.count() / 1000.0;
}

void HostAudioDriver::SDLAudioCallback(void *userdata, uint8_t *stream, int len) {
  HostAudioDriver *driver = (HostAudioDriver *)userdata;
  if (driver) {
    driver->FillAudioBuffer(stream, len);
  } else {
    memset(stream, 0, len);
  }
}

void HostAudioDriver::FillAudioBuffer(uint8_t *stream, int len) {
  std::lock_guard<std::mutex> lock(mutex_);

  memset(stream, 0, len);

  // Copy audio data from the buffer pool, generating exactly one more
  // playback slice (playSampleCount_ frames) at a time, on demand, instead of
  // firing a fixed tick once per SDL callback. This mirrors the Pico's
  // DMA-IRQ-driven cadence: a tick/new-buffer is only produced when the pool
  // has run dry, so the sequencer advances one slice per playSampleCount_
  // frames of real audio time rather than once per fixed-size SDL fragment.
  int remaining = len;
  uint8_t *dest = stream;

  while (remaining > 0) {
    if (!hasData()) {
      if (g_tickCount == 0) {
        g_tickStart = std::chrono::steady_clock::now();
      }
      g_tickCount++;
      if (g_tickCount % 48 == 0) {
        double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - g_tickStart).count();
        fprintf(stderr, "TICKDEBUG count=%d elapsed=%.3f rate=%.3f\n", g_tickCount, secs, g_tickCount / secs);
      }
      onAudioBufferTick();
      OnNewBufferNeeded();
      if (!hasData()) {
        // Not playing (or nothing produced): leave the rest of the stream
        // silent instead of spinning forever.
        break;
      }
    }

    AudioBufferData *buf = &pool_[poolPlayPosition_];
    if (buf->empty_) {
      poolPlayPosition_ = (poolPlayPosition_ + 1) % SOUND_BUFFER_COUNT;
      continue;
    }

    int to_copy = (buf->size_ < remaining) ? buf->size_ : remaining;
    memcpy(dest, buf->buffer_, to_copy);
    dest += to_copy;
    remaining -= to_copy;
    buf->size_ -= to_copy;

    if (buf->size_ <= 0) {
      buf->empty_ = true;
      poolPlayPosition_ = (poolPlayPosition_ + 1) % SOUND_BUFFER_COUNT;
      if (poolPlayPosition_ == poolQueuePosition_) {
        hasData_ = false;
      }
    }
  }

  samples_played_ += (len - remaining) / (obtained_spec_.channels * sizeof(int16_t));
}
