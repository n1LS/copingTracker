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

#ifndef _AUDIO_DRIVER_H_
#define _AUDIO_DRIVER_H_

#include "AudioSettings.h"
#include "Foundation/Observable.h"

#define SOUND_BUFFER_COUNT 2
#define SOUND_BUFFER_MAX 7500
#define MAX_SAMPLE_COUNT 1875

struct AudioBufferData {
  int size_;
  void *driverData_;
  char buffer_[MAX_SAMPLE_COUNT * 2 * sizeof(uint16_t)];
  size_t readOffset_;
  bool empty_;
};

class AudioDriver : public Observable {

public:
  class Event : public I_ObservableData {
  public:
    enum Type { ADET_DRIVERTICK, ADET_BUFFERNEEDED };

    Event(Type type) {
      type_ = type;
    };
    Type type_;
  };

public:
  // poolSize/pool let a subclass use a deeper render-ahead pool than the
  // Pico's DMA-double-buffering-driven default of SOUND_BUFFER_COUNT (2).
  // SOUND_BUFFER_COUNT is a Pico DMA constraint (it only ever needs to be one
  // buffer ahead of the one currently playing), not a general scheduling
  // limit - drivers whose physical callback/fragment size can exceed one
  // tempo slice (e.g. HostAudioDriver with large SDL fragments) need more
  // slots so the tick/render cadence (see onAudioBufferTick()) never has to
  // wait on pool depth.
  AudioDriver(AudioSettings &settings, AudioBufferData *pool, int poolSize);
  virtual ~AudioDriver();

  virtual bool Init();
  virtual void Close();
  virtual bool Start();
  virtual void Stop();

  virtual bool InitDriver() = 0;
  virtual void CloseDriver() = 0;
  virtual bool StartDriver() = 0;
  virtual void StopDriver() = 0;

  virtual bool Interlaced() = 0;
  virtual int GetPlayedBufferPercentage() = 0;
  virtual void OnAudioActive(bool active) {
  }

  virtual double GetStreamTime() = 0; // in secs

  virtual void AddBuffer(short *buffer, int size); // size in samples

  AudioSettings GetAudioSettings();

  void OnNewBufferNeeded();

  int GetPoolSize() {
    return poolSize_;
  }

protected:
  void eatBuffer(void *buffer, int size); // size in bytes
  void onAudioBufferTick();
  bool hasData();
  AudioSettings settings_;

protected:
  bool isPlaying_;
  AudioBufferData *pool_;
  int poolSize_;
  int poolQueuePosition_;
  int poolPlayPosition_;
  int bufferPos_;
  int bufferSize_;
  bool hasData_;
};
#endif
