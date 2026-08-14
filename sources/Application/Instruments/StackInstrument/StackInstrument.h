/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2018 Discodirt
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _STACK_INSTRUMENT_H_
#define _STACK_INSTRUMENT_H_

#include "Application/Instruments/I_Instrument.h"
#include "StackEngine.h"

class StackInstrument : public I_Instrument {

public:
  StackInstrument();
  virtual ~StackInstrument();

  // Start & stop the instument
  virtual bool Start(int channel, unsigned char note, uint8_t volume, bool retrigger = true);
  virtual void Stop(int channel);

  virtual void SetStepVolume(int channel, uint8_t volume);

  // size refers to the number of samples
  // should always fill interleaved stereo / 16bit
  virtual bool Render(int channel, fixed *buffer, int size, bool updateTick);
  virtual void ProcessCommand(int channel, Token token, uint16_t value);

  virtual InstrumentType GetType() {
    return IT_STACK;
  }

  etl::ilist<Variable *> *Variables() {
    return &variables_;
  }

  virtual bool Init() {
    return true;
  }

  virtual bool IsInitialized() {
    return true;
  }

  virtual bool IsEmpty() {
    return false;
  }

  void SetChannel(int i);
  void SendProgramChange(int channel, int program);
  void SendProgramChangeWithNote(int channel, int program);

  bool SupportsCommand(Token token);

  virtual void OnStart() {
  }
  virtual void Purge() {
  }

  virtual void GetTableState(TableSaveState &state) {
  }
  virtual void SetTableState(TableSaveState &state) {
  }

  virtual int GetTable() {
    return table_.GetInt();
  }

  virtual bool GetTableAutomation() {
    return tableAuto_.GetBool();
  }

private:
  static stack_voice_t voices_[SONG_CHANNEL_COUNT];

  stack_parameters_t getInstrumentParameters();

  etl::list<Variable *, 12> variables_;

  Variable spread_;
  Variable wave_;
  Variable transpose_;
  Variable table_;
  Variable tableAuto_;
  Variable attack_;
  Variable decay_;
  Variable sustain_;
  Variable release_;
  Variable volume_;
  Variable brightness_;
  Variable glide_;
};

#endif
