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

#ifndef _APPLICATION_MIXER_H_
#define _APPLICATION_MIXER_H_

#include "Application/Audio/AudioFileStreamer.h"
#include "Application/Mixer/MixerService.h"
#include "Application/Model/Project.h"
#include "Application/Utils/fixed.h"
#include "Application/Views/ViewData.h"
#include "Foundation/Observable.h"
#include "Foundation/T_Singleton.h"
#include "PlayerChannel.h"
#include "Services/Audio/AudioOut.h"

#define STREAM_MIX_BUS 8

class PlayerMixer : public T_Singleton<PlayerMixer>, public Observable, public I_Observer {
public:
  PlayerMixer();
  virtual ~PlayerMixer() {};

  bool Start();
  void Stop();
  bool IsPlaying();
  bool Init(Project *project);
  void BindProject(Project *project);
  void Close();

  void OnPlayerStart(MixerServiceMode msmMode);
  void OnPlayerStop();

  void StartInstrument(int channel, I_Instrument *instrument, unsigned char note, uint8_t volume, bool newInstrument);
  void StopInstrument(int channel, bool force = false);

  int GetChannelNote(int Channel);
  int GetChannelVolume(int Channel);

  I_Instrument *GetInstrument(int channel);

  I_Instrument *GetLastInstrument(int channel);

  // True while any channel still holds this instrument, ie. the audio thread
  // may render it. Note this is not the same as Player::GetPlayedInstrument(),
  // which reports NO_INSTRUMENT for muted channels even though those channels
  // still hold (and render) the instrument.
  bool IsInstrumentInUse(I_Instrument *instrument);

  // Force every channel holding this instrument to drop it. Must be called
  // before an instrument is destroyed or returned to the pool, otherwise the
  // channels are left rendering freed memory.
  void ReleaseInstrument(I_Instrument *instrument);

  void StartChannel(int channel);
  void StopChannel(int channel);

  bool IsChannelPlaying(int channel);

  void StartStreaming(const char *name, int startSample = 0);
  void StartLoopingStreaming(const char *name);
  void StopStreaming();

  stereosample GetMasterOutLevel();

  void Update(Observable &o, I_ObservableData *d);
  int GetPlayedBufferPercentage();

  void SetChannelMute(int channel, bool mute);
  bool IsChannelMuted(int channel);

  bool GetPlayedSliceIndex(int channel, uint8_t &sliceIndex);

  AudioOut *GetAudioOut();

  void Lock();
  void Unlock();

  // Get the current project
  Project *GetProject() {
    return project_;
  }

  etl::array<stereosample, SONG_CHANNEL_COUNT> *GetMixerLevels();

private:
  void updateDelayParameters();

  Project *project_;
  etl::array<stereosample, SONG_CHANNEL_COUNT> mixerLevels_;

  I_Instrument *lastInstrument_[SONG_CHANNEL_COUNT];
  bool isChannelPlaying_[SONG_CHANNEL_COUNT];

  AudioFileStreamer fileStreamer_;
  PlayerChannel *channel_[SONG_CHANNEL_COUNT];

  // store trigger notes, 0xFF = none

  uint8_t notes_[SONG_CHANNEL_COUNT];
  unsigned char volume_[SONG_CHANNEL_COUNT];
};

#endif
