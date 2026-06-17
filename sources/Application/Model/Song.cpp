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

#include "Song.h"
#include "Application/Instruments/CommandList.h"
#include "Application/Utils/HexBuffers.h"
#include "Phrase.h"
#include "System/System/System.h"
#include "System/io/Status.h"
#include "Table.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

Song::Song() : Persistent("Song"), chain_(), phrase_() { Reset(); };

Song::~Song() {};

void Song::Reset() {
  for (int i = 0; i < SONG_ROW_COUNT; i++) {
    for (int j = 0; j < SONG_CHANNEL_COUNT; j++) {
      rows_[i].chains[j] = EMPTY_SONG_VALUE;
    }
  }
  chain_.Reset();
  phrase_.Reset();
}

void Song::SaveContent(tinyxml2::XMLPrinter *printer) {
  saveHexBuffer(printer, "Song", (uint8_t *)rows_, SONG_ROW_COUNT * SONG_CHANNEL_COUNT);
  saveHexBuffer(printer, "ChainSteps", (uint8_t *)chain_.steps_, CHAIN_COUNT * PHRASES_PER_CHAIN * sizeof(ChainStep));
  saveHexBuffer(printer, "PhraseSteps", (uint8_t *)phrase_.steps_, PHRASE_COUNT * STEPS_PER_PHRASE * sizeof(PhraseStep));
}

void Song::RestoreContent(PersistencyDocument *doc) {
  bool elem = doc->FirstChild();

  while (elem) {
    if (!strcmp("Song", doc->ElemName())) {
      restoreHexBuffer(doc, (uint8_t *)rows_);
    };
    if (!strcmp("ChainSteps", doc->ElemName())) {
      restoreHexBuffer(doc, (uint8_t *)chain_.steps_);
    };
    if (!strcmp("PhraseSteps", doc->ElemName())) {
      restoreHexBuffer(doc, (uint8_t *)phrase_.steps_);
    };
    elem = doc->NextSibling();
  }

  Status::Set("Restoring allocation");

  // Restore chain & phrase allocation table

  for (int i = 0; i < SONG_ROW_COUNT; i++) {
    for (int j = 0; j < SONG_CHANNEL_COUNT; j++) {
      uint8_t v = rows_[i].chains[j];
      if (v != 0xFF && v < 0x80) {
        chain_.SetUsed(v);
      }
    }
  }

  for (int i = 0; i < CHAIN_COUNT; i++) {
    for (int j = 0; j < PHRASES_PER_CHAIN; j++) {
      uint8_t p = chain_.steps_[i][j].phrase;
      if (p != 0xFF) {
        chain_.SetUsed(i);
        phrase_.SetUsed(p);
      }
    };
  }

  TableHolder *th = TableHolder::GetInstance();

  for (int i = 0; i < PHRASE_COUNT; i++) {
    for (int j = 0; j < STEPS_PER_PHRASE; j++) {
      PhraseStep &step = phrase_.steps_[i][j];
      if (step.note != 0xFF) {
        phrase_.SetUsed(i);
      }
      if (FourCC::enum_type(step.cmd1) == FourCC::InstrumentCommandTable) {
        step.param1 &= 0x7F;
        th->SetUsed(step.param1);
      }
      if (FourCC::enum_type(step.cmd2) == FourCC::InstrumentCommandTable) {
        step.param2 &= 0x7F;
        th->SetUsed(step.param2);
      }
    };
  }
}
