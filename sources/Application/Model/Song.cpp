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
#include "Application/Persistency/PersistenceConstants.h"
#include "Application/Views/ToastView.h"
#include "Phrase.h"
#include "System/System/System.h"
#include "System/io/Status.h"
#include "Table.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

Song::Song() : Persistent("Song"), chain_(), phrase_() {
  Reset();
};

Song::~Song() {};

ChainContent Song::GetChainContent(uint8_t chain) const {
  bool hasPhrases = false;

  for (int i = 0; i < PHRASES_PER_CHAIN; i++) {
    const uint8_t phrase = chain_.steps_[chain][i].phrase;
    if (phrase == EMPTY_CHAIN_VALUE) {
      continue;
    }

    hasPhrases = true;
    // a phrase number that does not exist has nothing in it
    if (phrase < PHRASE_COUNT && !phrase_.IsEmpty(phrase)) {
      return ChainContent::HasContent;
    }
  }

  return hasPhrases ? ChainContent::OnlyEmptyPhrases : ChainContent::NoPhrases;
}

void Song::Reset() {
  for (int i = 0; i < SONG_ROW_COUNT; i++) {
    for (int j = 0; j < SONG_CHANNEL_COUNT; j++) {
      rows_[i].chains[j] = EMPTY_SONG_VALUE;
    }
  }
  chain_.Reset();
  phrase_.Reset();
}

// only what is there is saved, empty chains and phrases are left out and so are the steps of the others that are
// as a reset leaves them:
//   "rows": [[chain of each channel] per row, up to the last row that is not empty],
//   "chains": [{"id": 4, "steps": [[step, phrase, transpose], ...]}],
//   "phrases": [{"id": 3, "steps": [[step, note, instrument, volume, command 1, value 1, command 2, value 2], ...]}]
// the instrument is 255 for none and the volume 0 for none, the commands are the numbers of the Tokens
static bool isDefaultStep(const ChainStep &s) {
  return s.phrase == EMPTY_CHAIN_VALUE && s.transpose == 0;
}

static bool isDefaultStep(const PhraseStep &s) {
  return s.note == NO_NOTE && s.getInstrument() == NO_INSTRUMENT && s.getVolume() == NO_VOLUME &&
         s.getCmd1() == Token::InstrumentCommandNone && s.param1 == 0 && s.getCmd2() == Token::InstrumentCommandNone &&
         s.param2 == 0;
}

void Song::SaveContent(JsonWriter &writer) {
  int rowCount = 0;
  for (int i = 0; i < SONG_ROW_COUNT; i++) {
    for (int j = 0; j < SONG_CHANNEL_COUNT; j++) {
      if (rows_[i].chains[j] != EMPTY_SONG_VALUE) {
        rowCount = i + 1;
      }
    }
  }
  writer.BeginArray("rows");
  for (int i = 0; i < rowCount; i++) {
    writer.IntArray(nullptr, rows_[i].chains, SONG_CHANNEL_COUNT);
  }
  writer.EndArray();

  writer.BeginArray("chains");
  for (int i = 0; i < CHAIN_COUNT; i++) {
    bool empty = true;
    for (int j = 0; j < PHRASES_PER_CHAIN; j++) {
      empty = empty && isDefaultStep(chain_.steps_[i][j]);
    }
    if (empty) {
      continue;
    }
    writer.BeginObject();
    writer.Int("id", i);
    writer.BeginArray("steps");
    for (int j = 0; j < PHRASES_PER_CHAIN; j++) {
      if (!isDefaultStep(chain_.steps_[i][j])) {
        const int step[3] = {j, chain_.steps_[i][j].phrase, chain_.steps_[i][j].transpose};
        writer.IntArray(nullptr, step, 3);
      }
    }
    writer.EndArray();
    writer.EndObject();
  }
  writer.EndArray();

  writer.BeginArray("phrases");
  for (int i = 0; i < PHRASE_COUNT; i++) {
    bool empty = true;
    for (int j = 0; j < STEPS_PER_PHRASE; j++) {
      empty = empty && isDefaultStep(phrase_.steps_[i][j]);
    }
    if (empty) {
      continue;
    }
    writer.BeginObject();
    writer.Int("id", i);
    writer.BeginArray("steps");
    for (int j = 0; j < STEPS_PER_PHRASE; j++) {
      const PhraseStep &s = phrase_.steps_[i][j];
      if (isDefaultStep(s)) {
        continue;
      }
      const int step[8] = {
          j, static_cast<int>(s.note), s.getInstrument(), s.getVolume(), s.getCmd1(), s.param1, s.getCmd2(), s.param2};
      writer.IntArray(nullptr, step, 8);
    }
    writer.EndArray();
    writer.EndObject();
  }
  writer.EndArray();
}

// the steps of one chain / phrase, each starts with its index: reads up to the end of the array, a step that is not
// in range is skipped
template <typename Store>
static void readStepList(JsonReader &reader, int id, int idLimit, int stepLimit, size_t fields, Store store) {
  if (!reader.BeginArray()) {
    return;
  }
  while (reader.NextElement()) {
    int values[8] = {0};
    const size_t count = reader.ReadIntArray(values, fields + 1);
    if (id >= 0 && id < idLimit && count >= fields + 1 && values[0] >= 0 && values[0] < stepLimit) {
      store(id, values[0], values + 1);
    }
  }
}

void Song::RestoreContent(JsonReader &reader) {
  // sparse files: everything that is not in the file is empty
  Reset();

  while (reader.NextKey()) {
    if (reader.KeyIs("rows")) {
      if (!reader.BeginArray()) {
        return;
      }
      int row = 0;
      while (reader.NextElement()) {
        uint8_t chains[SONG_CHANNEL_COUNT];
        const size_t count = reader.ReadIntArray(chains, SONG_CHANNEL_COUNT);
        if (row < SONG_ROW_COUNT && count >= SONG_CHANNEL_COUNT) {
          for (int j = 0; j < SONG_CHANNEL_COUNT; j++) {
            rows_[row].chains[j] = chains[j];
          }
        }
        row++;
      }
    } else if (reader.KeyIs("chains") || reader.KeyIs("phrases")) {
      const bool isChain = reader.KeyIs("chains");
      if (!reader.BeginArray()) {
        return;
      }
      while (reader.NextElement()) {
        if (!reader.BeginObject()) {
          return;
        }
        int id = -1;
        while (reader.NextKey()) {
          if (reader.KeyIs("id")) {
            id = reader.ReadInt();
          } else if (reader.KeyIs("steps")) {
            // the id comes first
            if (isChain) {
              readStepList(reader, id, CHAIN_COUNT, PHRASES_PER_CHAIN, 2, [this](int chain, int step, const int *v) {
                chain_.steps_[chain][step].phrase = static_cast<uint8_t>(v[0]);
                chain_.steps_[chain][step].transpose = static_cast<uint8_t>(v[1]);
              });
            } else {
              readStepList(reader, id, PHRASE_COUNT, STEPS_PER_PHRASE, 7, [this](int phrase, int step, const int *v) {
                PhraseStep &s = phrase_.steps_[phrase][step];
                s.note = static_cast<uint8_t>(v[0]);
                s.setInstrument(static_cast<uint8_t>(v[1]));
                s.setVolume(static_cast<uint8_t>(v[2]));
                s.setCmd1(static_cast<uint8_t>(v[3]));
                s.param1 = static_cast<uint16_t>(v[4]);
                s.setCmd2(static_cast<uint8_t>(v[5]));
                s.param2 = static_cast<uint16_t>(v[6]);
              });
            }
          } else {
            reader.Skip();
          }
        }
      }
    } else {
      reader.Skip();
    }
  }
  if (reader.HadError()) {
    return;
  }

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
    }
  }

  TableHolder *th = TableHolder::GetInstance();

  for (int i = 0; i < PHRASE_COUNT; i++) {
    for (int j = 0; j < STEPS_PER_PHRASE; j++) {
      PhraseStep &step = phrase_.steps_[i][j];
      if (step.note != 0xFF) {
        phrase_.SetUsed(i);
      }
      if (Token::enum_type(step.getCmd1()) == Token::InstrumentCommandTable) {
        step.param1 &= 0x7F;
        th->SetUsed(step.param1);
      }
      if (Token::enum_type(step.getCmd2()) == Token::InstrumentCommandTable) {
        step.param2 &= 0x7F;
        th->SetUsed(step.param2);
      }
    }
  }
}
