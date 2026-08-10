/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _GM_BANK_SOURCE_H_
#define _GM_BANK_SOURCE_H_

#include "GMBank.h"
#include "SoundSource.h"

// Adapts the flash-resident General MIDI sample bank (GMBank /
// GMBank_data.generated.h) to the SoundSource interface so a SampleInstrument
// can play GM instruments through the normal rendering pipeline.
//
// The GM PCM data lives in a separate flash blob (pcmData[] in
// GMBank_data.generated.h) from the SD-card-loaded samples SamplePool
// manages, so it cannot be exposed through a regular pool-backed source.
// Instead this class resolves per-note SampleEntry data straight from
// GMBank::sampleForNote() and reports itself as IsMulti() == true, which
// makes SampleInstrument::Start() resolve loop points/root note/size per note
// exactly like it already does for other multi-sample sources.
//
// One instance is owned per SampleInstrument; SetInstrument() selects which
// GM patch (0..kGMInstrumentCount-1) subsequent per-note lookups resolve
// against.
class GMBankSource : public SoundSource {
public:
  void SetInstrument(uint16_t instrument) {
    instrument_ = instrument;
  };
  uint16_t GetInstrument() const {
    return instrument_;
  };

  virtual int GetLoopStart(int note);
  virtual int GetLoopEnd(int note);
  virtual int GetSize(int note);
  virtual int GetSampleRate(int note);
  virtual int GetChannelCount(int note);
  virtual void *GetSampleBuffer(int note);
  virtual bool IsMulti() {
    return true;
  };
  virtual int GetRootNote(int note);
  virtual float GetLengthInSec();

  uint8_t GetAttack(int note);
  uint8_t GetDecay(int note);
  uint8_t GetSustain(int note);
  uint8_t GetRelease(int note);

private:
  bool lookup(int note, const SampleEntry **entry) const;
  uint16_t instrument_ = 0;
};

#endif
