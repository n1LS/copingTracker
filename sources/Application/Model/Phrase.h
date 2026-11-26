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

#ifndef _PHRASE_H_
#define _PHRASE_H_

#include "Application/Model/UndoJournal.h"
#include "Foundation/Types/Types.h"
#include <bitset>

#define PHRASE_COUNT 0x80
#define NO_MORE_PHRASE (PHRASE_COUNT + 1)
#define STEPS_PER_PHRASE 16

// the instruments there are, and what a step has for no instrument and no volume (the values the code outside of
// the step uses, the volume is stored as it is, the instrument in fewer bits, see PhraseStep)
#define MAX_INSTRUMENT_COUNT 32
#define NO_INSTRUMENT 0xFF
#define NO_VOLUME 0

// One step of a phrase in 8 bytes: the two command parameters and 32 bits for the rest. The fields that are
// smaller than a byte are read and written through the functions, which convert from and to the values the rest
// of the code uses:
//  - the volume is 1..15 (0xF is the loudest) and 0 is no volume (NO_VOLUME), there is no volume 0 to play
//  - the instrument is 0..MAX_INSTRUMENT_COUNT - 1, it is stored plus one, 0 in the step is no instrument
//    (NO_INSTRUMENT)
//  - a command is the value of its Token (they are all below 64), 0 is no command (InstrumentCommandNone)
// A step that is all 0 has no volume, no instrument and no commands, only its note is 0 (not NO_NOTE).
struct PhraseStep {
  uint16_t param1;
  uint16_t param2;
  uint32_t note : 8;
  uint32_t volume_ : 4;
  uint32_t instrument_ : 6;
  uint32_t cmd1_ : 7;
  uint32_t cmd2_ : 7;

  // NO_VOLUME (0) or 1..15
  uint8_t getVolume() const {
    return static_cast<uint8_t>(volume_);
  }
  // NO_VOLUME (0) is no volume, more than 15 is the loudest
  void setVolume(uint8_t volume) {
    volume_ = volume > 15 ? 15 : volume;
  }

  // NO_INSTRUMENT or 0..MAX_INSTRUMENT_COUNT - 1
  uint8_t getInstrument() const {
    return (instrument_ == 0 || instrument_ > MAX_INSTRUMENT_COUNT) ? NO_INSTRUMENT
                                                                    : static_cast<uint8_t>(instrument_ - 1);
  }
  // anything that is not an instrument is no instrument
  void setInstrument(uint8_t instrument) {
    instrument_ = instrument >= MAX_INSTRUMENT_COUNT ? 0 : instrument + 1;
  }

  // the Token value of the command
  uint8_t getCmd1() const {
    return static_cast<uint8_t>(cmd1_);
  }
  uint8_t getCmd2() const {
    return static_cast<uint8_t>(cmd2_);
  }
  void setCmd1(uint8_t command) {
    cmd1_ = command;
  }
  void setCmd2(uint8_t command) {
    cmd2_ = command;
  }
};
static_assert(sizeof(PhraseStep) == 8, "a PhraseStep is 8 bytes");
static_assert(MAX_INSTRUMENT_COUNT < 64, "the instrument has 6 bits and is stored plus one");
static_assert(NO_VOLUME == 0, "the volume is stored as it is, 0 is none");

class Phrase {
public:
  Phrase();
  ~Phrase();
  void Reset();
  uint16_t GetNext();
  // A phrase is empty if none of its steps has a note, an instrument, a volume or a command (the parameter of
  // a command that is none does not count)
  bool IsEmpty(uint8_t phrase) const;
  bool IsUsed(uint8_t i) {
    return isUsed_[i];
  };
  void SetUsed(uint8_t c);
  void ClearAllocation();

  inline Token getCmd1(int phrase, int step) const {
    return Token::enum_type(steps_[phrase][step].getCmd1());
  }
  inline Token getCmd2(int phrase, int step) const {
    return Token::enum_type(steps_[phrase][step].getCmd2());
  }
  inline void setCmd1(int phrase, int step, Token f) {
    EditStep(phrase, step).setCmd1(static_cast<uint8_t>(f.raw()));
  }
  inline void setCmd2(int phrase, int step, Token f) {
    EditStep(phrase, step).setCmd2(static_cast<uint8_t>(f.raw()));
  }

  // The steps. Everything that changes one goes through EditStep / EditSteps, they record the bytes in the undo
  // history before they hand out the memory (call them right before the change, not to look).
  const PhraseStep &Step(int phrase, int step) const {
    return steps_[phrase][step];
  }
  const PhraseStep *Steps(int phrase) const {
    return steps_[phrase];
  }
  PhraseStep &EditStep(int phrase, int step) {
    UndoJournal::RecordChange(&steps_[phrase][step], sizeof(PhraseStep));
    return steps_[phrase][step];
  }
  // all STEPS_PER_PHRASE steps of the phrase
  PhraseStep *EditSteps(int phrase) {
    UndoJournal::RecordChange(steps_[phrase], sizeof(steps_[phrase]));
    return steps_[phrase];
  }

private:
  friend class Song; // saves and loads the steps as they are

  PhraseStep steps_[PHRASE_COUNT][STEPS_PER_PHRASE];
  std::bitset<PHRASE_COUNT> isUsed_;
};

#endif
