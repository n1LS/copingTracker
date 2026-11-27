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

#ifndef _SONG_H_
#define _SONG_H_

#include <stdint.h>

#include "Application/Persistency/Persistent.h"
#include "Chain.h"
#include "Phrase.h"
#include "config/AudioConstants.h"

#define SONG_ROW_COUNT 128

#define NOTE_C3 60
#define LOWEST_NOTE 0
#define HIGHEST_NOTE 119
#define HIGHEST_PLAYABLE_NOTE 127
#define NOTE_OFF 0xFE
#define NO_NOTE 0xFF

#define EMPTY_SONG_VALUE 0xFF
#define EMPTY_CHAIN_VALUE 0xFF

// What a chain holds, see Song::GetChainContent
enum class ChainContent : uint8_t {
  NoPhrases,        // it does not reference any phrase
  OnlyEmptyPhrases, // it references phrases but all of them are empty
  HasContent        // at least one of the phrases it references is not empty
};

struct SongRow {
  uint8_t chains[SONG_CHANNEL_COUNT];
};

class Song : Persistent {
public:
  Song();
  ~Song();
  void Reset();
  // chain, phrase and table allocation from the data, see the definition
  void RebuildAllocation(bool afterEdit);

  // Whether a chain is empty, the phrases it references count (it stops at the first one that has something)
  ChainContent GetChainContent(uint8_t chain) const;

  virtual void SaveContent(JsonWriter &writer);
  virtual void RestoreContent(JsonReader &reader);

  // The chain of each channel in each row. Everything that changes one goes through EditCell, it records the byte in
  // the undo history before it hands out the memory (call it right before the change, not to look).
  const SongRow &Row(int row) const {
    return rows_[row];
  }
  uint8_t Cell(int row, int channel) const {
    return rows_[row].chains[channel];
  }
  uint8_t &EditCell(int row, int channel) {
    UndoJournal::RecordChange(&rows_[row].chains[channel], sizeof(uint8_t));
    return rows_[row].chains[channel];
  }

  // The rows firstRow..lastRow are recorded in the undo history, the array of all rows is handed out (for the
  // operations that move whole blocks of the song around).
  SongRow *EditRows(int firstRow, int lastRow) {
    UndoJournal::RecordChange(&rows_[firstRow], sizeof(SongRow) * (lastRow - firstRow + 1));
    return rows_;
  }

  // The loop of the song view, the first and last song row, it is part of the project but not of the undo history.
  // No loop is -1.
  bool HasLoop() const {
    return loopStart_ >= 0;
  }
  int LoopStart() const {
    return loopStart_;
  }
  int LoopEnd() const {
    return loopEnd_;
  }
  void SetLoop(int start, int end) {
    loopStart_ = start < end ? start : end;
    loopEnd_ = start < end ? end : start;
  }
  void ClearLoop() {
    loopStart_ = -1;
    loopEnd_ = -1;
  }

  Chain chain_;
  Phrase phrase_;

private:
  int loopStart_ = -1;
  int loopEnd_ = -1;
  SongRow rows_[SONG_ROW_COUNT];
};

#endif
