/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#pragma once

#include "Application/Instruments/I_Instrument.h"
#include "Application/Model/Song.h"
#include "Application/Persistency/PersistenceConstants.h"
#include "DrumEngine.h"
#include "System/Console/Trace.h"
#include <cstdint>

class DrumInstrument : public I_Instrument {

public:
  DrumInstrument();
  virtual ~DrumInstrument() {};

  virtual bool Init() {
    return true;
  }
  virtual bool IsInitialized() {
    return true;
  };
  virtual bool IsEmpty() {
    return false;
  };

  virtual bool SupportsCommand(Token cc);

  virtual InstrumentType GetType() {
    return IT_DRUM;
  };

  // Start & stop the instument
  virtual bool Start(int channel, unsigned char note, uint8_t volume, bool retrigger = true);
  virtual void Stop(int channel);

  virtual void OnStart() {};
  virtual void Purge() {};

  virtual void SetStepVolume(int channel, uint8_t volume);

  // size refers to the number of samples
  // should always fill interleaved stereo / 16bit
  virtual bool Render(int channel, fixed *buffer, int size, bool updateTick);
  virtual void ProcessCommand(int channel, Token cc, uint16_t value);

  virtual int GetTable() {
    return 0;
  };
  virtual bool GetTableAutomation() {
    return false;
  };
  virtual void GetTableState(TableSaveState &state) {};
  virtual void SetTableState(TableSaveState &state) {};
  etl::ilist<Variable *> *Variables() {
    return &variables_;
  };

  void setChannel(uint8_t channel);

  void noteDisplay(uint8_t note, char (&out)[4]) override;

private:
  static drum_voice_t voices_[SONG_CHANNEL_COUNT];

  etl::list<Variable *, 13> variables_;

  Variable vVoice0_;
  Variable vVoice1_;
  Variable vVoice2_;
  Variable vVoice3_;
  Variable vVoice4_;
  Variable vVoice5_;
  Variable vVoice6_;
  Variable vVoice7_;
  Variable vVoice8_;
  Variable vVoice9_;
  Variable vVoice10_;
  Variable vVoice11_;

  Variable vCharacter_;

  void RunCommand(int channel);
  void CommandInitArp(int channel, uint16_t value);
  drum_parameters_t getInstrumentParameters(uint8_t note);
};
