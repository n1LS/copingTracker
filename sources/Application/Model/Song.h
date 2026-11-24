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

  // Whether a chain is empty, the phrases it references count (it stops at the first one that has something)
  ChainContent GetChainContent(uint8_t chain) const;

  virtual void SaveContent(tinyxml2::XMLPrinter *printer);
  virtual void RestoreContent(PersistencyDocument *doc);

  SongRow rows_[SONG_ROW_COUNT];
  Chain chain_;
  Phrase phrase_;
};

#endif
