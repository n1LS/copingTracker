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
#include "LSDJKitEngine.h"
#include "System/Console/Trace.h"
#include <cstdint>

// two kits of 15 samples each, encoded as note = kit1Sample * 15 + kit2Sample
#define LSDJKIT_HIGHEST_NOTE 225

class LSDJKitInstrument : public I_Instrument {

public:
  LSDJKitInstrument();
  virtual ~LSDJKitInstrument() {};

  virtual bool Init() {
    return true;
  }
  virtual bool IsInitialized() {
    return true;
  };
  virtual bool IsEmpty() {
    return false;
  };

  bool SupportsCommand(Token token);

  virtual InstrumentType GetType() {
    return IT_LSDJKIT;
  }

  virtual bool SupportsScales() override {
    return false;
  }

  virtual uint8_t GetHighestNote() override {
    return LSDJKIT_HIGHEST_NOTE;
  }

  // Start & stop the instument
  virtual bool Start(int channel, unsigned char note, uint8_t volume, bool retrigger = true);
  virtual void Stop(int channel);

  virtual void OnStart() {};
  virtual void Purge() {};

  virtual void SetStepVolume(int channel, uint8_t volume);

  // size refers to the number of samples
  // should always fill interleaved stereo / 16bit
  virtual bool Render(int channel, fixed *buffer, int size, bool updateTick);
  virtual void ProcessCommand(int channel, Token token, uint16_t value);

  virtual int GetTable() {
    return 0;
  }
  virtual bool GetTableAutomation() {
    return false;
  }
  virtual void GetTableState(TableSaveState &state) {};
  virtual void SetTableState(TableSaveState &state) {};
  etl::ilist<Variable *> *Variables() {
    return &variables_;
  }

  void setChannel(uint8_t channel);

  void noteDisplay(uint8_t note, char (&out)[4]) override;
  void noteDisplayCondensed(uint8_t note, char (&line1)[3], char (&line2)[3]) override;
  void focusedNoteDisplay(uint8_t note, char (&line)[12]);

  virtual int GetNoteIncrement(bool small) override {
    return small ? 1 : 15;
  }

private:
  static lsdjkit_voice_t voices_[SONG_CHANNEL_COUNT];

  etl::list<Variable *, 7> variables_;

  Variable vKit1_;
  Variable vKit2_;
  Variable vBitDepth_;

  void RunCommand(int channel);
  void CommandInitArp(int channel, uint16_t value);

  lsdjkit_parameters_t getInstrumentParameters(uint8_t note);
};
