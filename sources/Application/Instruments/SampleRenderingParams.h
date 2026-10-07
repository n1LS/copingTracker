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

#ifndef _SAMPLE_RENDER_PARAMS_H_
#define _SAMPLE_RENDER_PARAMS_H_

#include "Application/Instruments/EnvelopeGenerators.h"
#include "Externals/etl/include/etl/vector.h"
#include "Foundation/Types/Types.h"
#include "SRPUpdaters.h"

#pragma pack(push, 1)
struct sample_voice_t {
  int16_t *sampleBuffer_; // wavdata
  int channelCount_;

  int krateCount_;    // K-rate counter
  float position_;    // Position in the sample stream
  int rendFirst_;     // position of the first sample (can be either start or loop
                      // depending on the mode)
  int rendLoopStart_; // Loop start position
  int rendLoopEnd_;   // Loop end position

  fixed baseSpeed_;  // The base speed with respect to current note
  fixed speed_;      // speed at which we currently travel the stream
  fixed baseVolume_; // Base volume the instrument was triggered with
  fixed volume_;     // Current volume
  int retrigLoop_;   // number of ticks before retrig
  int retrigCount_;  // current tick countdown before retrig
  int retrigOffset_; // offset in ticks after retrig

  fixed baseFCut_;
  fixed baseFRes_;

  fixed cutoff_; // filter cutoff
  fixed reso_;   // filter reso

  fixed baseFbTun_;
  fixed baseFbMix_;

  fixed fbTun_;
  fixed fbMix_;

  fixed basePan_; // panning
  fixed pan_;

  bool reverse_;  // true if we we go backwards in stream
  bool retrig_;   // true if we're retriggering
  bool finished_; // the instrument has cut off

  unsigned char crush_; // crush
  unsigned char drive_; // crush drive

  unsigned char downsample_; // downsampling
  bool couldClick_;
  char midiNote_;            // Current midi note
  signed char lastMidiNote_; // last midi note started on this channel, -1 = none
  // (legato pitch base and first-trigger detection)

  bool sliceActive_;
  uint8_t activeSliceIndex_;
  int loopModeValue_;

  // Last samples rendered on this channel (left/right), used for click
  // suppression when a note is (re)triggered.
  fixed lastSample_[2];

  // Active Sample Rendering Parameter updaters, stored as tags (UpdaterKind
  // values as uint8_t) instead of polymorphic pointers.
  etl::vector<uint8_t, 10> activeUpdaters_;

  VolumeRamp volumeRamp_;
  Panner panner_;
  FCRamp cutRamp_;
  FRRamp resRamp_;
  LinSpeedRamp speedRamp_;
  LogSpeedRamp legato_;
  LogSpeedRamp pfin_;
  Arp arp_;
  Vibrato vibrato_;

  adsr_envelope_t envelope_;

  uint8_t _padding[3];
};
#pragma pack(pop)

// --- de-virtualized SRP updater helpers --------------------------------------
// sample_voice_t tracks active updaters by a small tag (UpdaterKind stored as
// a uint8_t in activeUpdaters_) rather than by polymorphic pointers. These
// helpers keep that indirection in one place and are used by SampleInstrument
// and by any code that drives per-voice rendering parameters.

inline UpdaterKind UpdaterKindFromByte(uint8_t b) {
  return static_cast<UpdaterKind>(b);
}

inline bool IsUpdaterActive(const sample_voice_t &v, UpdaterKind kind) {
  const uint8_t tag = static_cast<uint8_t>(kind);
  for (auto b : v.activeUpdaters_) {
    if (b == tag) {
      return true;
    }
  }
  return false;
}

inline void AddUpdater(sample_voice_t &v, UpdaterKind kind) {
  if (!IsUpdaterActive(v, kind)) {
    v.activeUpdaters_.push_back(static_cast<uint8_t>(kind));
  }
}

inline void RemoveUpdater(sample_voice_t &v, UpdaterKind kind) {
  const uint8_t tag = static_cast<uint8_t>(kind);
  for (auto it = v.activeUpdaters_.begin(); it != v.activeUpdaters_.end(); ++it) {
    if (*it == tag) {
      v.activeUpdaters_.erase(it);
      break;
    }
  }
}

inline void TriggerUpdater(sample_voice_t &v, UpdaterKind kind, bool tableTick) {
  switch (kind) {
    case UpdaterKind::Volume:
      v.volumeRamp_.Trigger(tableTick);
      break;
    case UpdaterKind::Pan:
      v.panner_.Trigger(tableTick);
      break;
    case UpdaterKind::Cut:
      v.cutRamp_.Trigger(tableTick);
      break;
    case UpdaterKind::Res:
      v.resRamp_.Trigger(tableTick);
      break;
    case UpdaterKind::Speed:
      v.speedRamp_.Trigger(tableTick);
      break;
    case UpdaterKind::Legato:
      v.legato_.Trigger(tableTick);
      break;
    case UpdaterKind::Pfin:
      v.pfin_.Trigger(tableTick);
      break;
    case UpdaterKind::Arp:
      v.arp_.Trigger(tableTick);
      break;
    case UpdaterKind::Vibrato:
      v.vibrato_.Trigger(tableTick);
      break;
    default:
      break;
  }
}

inline void UpdateUpdater(sample_voice_t &v, UpdaterKind kind, RUParams &rup) {
  switch (kind) {
    case UpdaterKind::Volume:
      v.volumeRamp_.UpdateSRP(rup);
      break;
    case UpdaterKind::Pan:
      v.panner_.UpdateSRP(rup);
      break;
    case UpdaterKind::Cut:
      v.cutRamp_.UpdateSRP(rup);
      break;
    case UpdaterKind::Res:
      v.resRamp_.UpdateSRP(rup);
      break;
    case UpdaterKind::Speed:
      v.speedRamp_.UpdateSRP(rup);
      break;
    case UpdaterKind::Legato:
      v.legato_.UpdateSRP(rup);
      break;
    case UpdaterKind::Pfin:
      v.pfin_.UpdateSRP(rup);
      break;
    case UpdaterKind::Arp:
      v.arp_.UpdateSRP(rup);
      break;
    case UpdaterKind::Vibrato:
      v.vibrato_.UpdateSRP(rup);
      break;
    default:
      break;
  }
}

static_assert(sizeof(sample_voice_t) % 4 == 0, "Check sizeof(chiptune_voice_t) in error message - it must be a multiple of 4");

#endif
