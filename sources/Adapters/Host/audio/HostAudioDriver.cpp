/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "HostAudioDriver.h"
#include <algorithm>
#include <cstring>

HostAudioDriver *HostAudioDriver::instance_ = nullptr;
AudioBufferData HostAudioDriver::staticPool_[HOST_POOL_SIZE];

HostAudioDriver::HostAudioDriver(AudioSettings &settings)
    : AudioDriver(settings, staticPool_, HOST_POOL_SIZE), device_id_(0), samples_played_(0) {
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

  start_time_ = std::chrono::system_clock::now();
  samples_played_ = 0;
  return true;
}

void HostAudioDriver::CloseDriver() {
  // Always stop any previously-running producer thread before touching the
  // SDL device: InitDriver()/StartDriver() can be invoked more than once on
  // the same instance (e.g. once at boot, then again when a project is
  // (re)loaded), and reassigning producerThread_ while the old thread is
  // still joinable would call std::terminate().
  running_ = false;
  slotCv_.notify_all();
  if (producerThread_.joinable()) {
    producerThread_.join();
  }

  if (device_id_ != 0) {
    SDL_CloseAudioDevice(device_id_);
    device_id_ = 0;
  }
}

bool HostAudioDriver::StartDriver() {
  if (device_id_ == 0) {
    return false;
  }

  // Guard against StartDriver() being called again while a producer thread
  // from a previous Start() is still alive (see CloseDriver() comment).
  running_ = false;
  slotCv_.notify_all();
  if (producerThread_.joinable()) {
    producerThread_.join();
  }

  // Kick off the render thread with HOST_POOL_SIZE - 1 permits, matching
  // picoTrackerAudioDriver's sem_init/sem_release sequence: this lets the
  // producer render that many slices ahead of the very first callback before
  // it has to wait for playback to free a slot.
  {
    std::lock_guard<std::mutex> lock(slotMutex_);
    freeSlots_ = HOST_POOL_SIZE - 1;
  }
  running_ = true;
  producerThread_ = std::thread(&HostAudioDriver::ProducerLoop, this);

  SDL_PauseAudioDevice(device_id_, 0);
  return true;
}

void HostAudioDriver::StopDriver() {
  if (device_id_ != 0) {
    SDL_PauseAudioDevice(device_id_, 1);
  }

  running_ = false;
  slotCv_.notify_all();
  if (producerThread_.joinable()) {
    producerThread_.join();
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

// Runs on a dedicated thread and renders one playback slice at a time, ahead
// of when the SDL callback needs it - analogous to AudioThread() on core1 for
// the Pico. freeSlots_ is purely a render-ahead throttle (don't get more than
// HOST_POOL_SIZE-1 slices ahead of playback); it does not determine how
// much audio is produced per tick. Each iteration below fires exactly one
// onAudioBufferTick()/OnNewBufferNeeded() pair, which together flush MIDI and
// render exactly getPlaySampleCount() samples (the tempo-dependent slice size
// from SyncMaster) via AudioOutDriver::Trigger()/AudioMixer::Render(). This
// tempo-slice quantum is the actual scheduling clock, and it is completely
// independent of the SDL fragment size (desired.samples) and of
// HOST_POOL_SIZE/pool depth - those only affect how far ahead we render
// and how many samples FillAudioBuffer() copies out per callback, never how
// much is rendered per tick.
void HostAudioDriver::ProducerLoop() {
  while (running_) {
    std::unique_lock<std::mutex> lock(slotMutex_);
    slotCv_.wait(lock, [this] { return !running_ || freeSlots_ > 0; });
    if (!running_) {
      break;
    }
    freeSlots_--;
    lock.unlock();

    // onAudioBufferTick()/OnNewBufferNeeded() drive the actual DSP render
    // (AudioOutDriver::Trigger() -> AudioMixer::Render()) and push the
    // result into AudioDriver::pool_ via AddBuffer(). This is the
    // potentially expensive part we want off the real-time audio thread.
    // mutex_ guards the shared pool_/poolQueuePosition_/poolPlayPosition_
    // state against the concurrent SDL callback thread (FillAudioBuffer).
    std::lock_guard<std::mutex> poolLock(mutex_);
    onAudioBufferTick();
    OnNewBufferNeeded();
  }
}

void HostAudioDriver::FillAudioBuffer(uint8_t *stream, int len) {
  std::lock_guard<std::mutex> lock(mutex_);

  memset(stream, 0, len);

  // Copy pre-rendered audio data from the buffer pool. Rendering itself
  // happens on ProducerLoop(), so this real-time callback only ever does
  // memcpy's and index bookkeeping - no DSP work - keeping it fast and
  // avoiding the audible dropouts ("hollow" sound) that resulted from
  // rendering synchronously inside the callback with no lookahead.
  int remaining = len;
  uint8_t *dest = stream;

  while (remaining > 0 && hasData()) {
    AudioBufferData *buf = &pool_[poolPlayPosition_];
    if (buf->empty_) {
      poolPlayPosition_ = (poolPlayPosition_ + 1) % HOST_POOL_SIZE;
      continue;
    }

    int to_copy = (buf->size_ < remaining) ? buf->size_ : remaining;
    memcpy(dest, buf->buffer_, to_copy);
    dest += to_copy;
    remaining -= to_copy;
    buf->size_ -= to_copy;

    if (buf->size_ <= 0) {
      buf->empty_ = true;
      poolPlayPosition_ = (poolPlayPosition_ + 1) % HOST_POOL_SIZE;
      if (poolPlayPosition_ == poolQueuePosition_) {
        hasData_ = false;
      }

      // Free slot consumed: let the producer render the next slice.
      {
        std::lock_guard<std::mutex> slotLock(slotMutex_);
        if (freeSlots_ < HOST_POOL_SIZE - 1) {
          freeSlots_++;
        }
      }
      slotCv_.notify_one();
    }
  }

  samples_played_ += (len - remaining) / (obtained_spec_.channels * sizeof(int16_t));
}
