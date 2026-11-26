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

#include "Groove.h"
#include "Application/Persistency/PersistenceConstants.h"

unsigned char Groove::data_[MAX_GROOVES][STEPS_PER_GROOVE];

Groove::Groove() : Persistent(JSON_NODE_GROOVES) {
  Clear();
};

Groove::~Groove() {};

void Groove::Clear() {
  // Init all grooves with basic datas
  memset(data_, NO_GROOVE_DATA, sizeof(data_));
  for (int i = 0; i < MAX_GROOVES; i++) {
    data_[i][0] = 6;
    data_[i][1] = 6;
  };
  // init grooves selectah
  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
    // Channel
    channelGroove_[i].groove_ = 0;
    channelGroove_[i].position_ = 0;
    channelGroove_[i].ticks_ = data_[0][0];
  };
}
// Resest groove data at song startup

void Groove::Reset() {
  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
    // Channel
    ChannelGroove &c = channelGroove_[i];
    c.position_ = 0;
    c.ticks_ = data_[c.groove_][c.position_];
  }
}

void Groove::GetChannelData(int channel, int *groove, int *position) {
  ChannelGroove &c = channelGroove_[channel];
  *groove = c.groove_;
  *position = c.position_;
}

// Grooves that are as Clear() leaves them are not saved, of the others the steps up to the last one that is set:
//   "grooves": [{"id": 3, "steps": [7, 5]}]
// a step of 0 would be a division by zero, it is the end of the groove like 255
void Groove::SaveContent(JsonWriter &writer) {
  writer.BeginArray(JSON_KEY_GROOVES);
  for (int i = 0; i < MAX_GROOVES; i++) {
    int count = 0;
    for (int j = 0; j < STEPS_PER_GROOVE; j++) {
      if (data_[i][j] != NO_GROOVE_DATA) {
        count = j + 1;
      }
    }
    if (count == 2 && data_[i][0] == 6 && data_[i][1] == 6) {
      continue;
    }
    writer.BeginObject();
    writer.Int(JSON_KEY_ID, i);
    writer.IntArray(JSON_KEY_STEPS, data_[i], count);
    writer.EndObject();
  }
  writer.EndArray();
}

void Groove::RestoreContent(JsonReader &reader) {
  // grooves that are not in the file are the default ones
  Clear();

  while (reader.NextKey()) {
    if (!reader.KeyIs(JSON_KEY_GROOVES)) {
      reader.Skip();
      continue;
    }
    if (!reader.BeginArray()) {
      return;
    }
    while (reader.NextElement()) {
      if (!reader.BeginObject()) {
        return;
      }
      int id = -1;
      while (reader.NextKey()) {
        if (reader.KeyIs(JSON_KEY_ID)) {
          id = reader.ReadInt();
        } else if (reader.KeyIs(JSON_KEY_STEPS) && id >= 0 && id < MAX_GROOVES) {
          // the id comes first, the steps that are not in the file are cleared
          memset(data_[id], NO_GROOVE_DATA, STEPS_PER_GROOVE);
          reader.ReadIntArray(data_[id], STEPS_PER_GROOVE);
          for (int j = 0; j < STEPS_PER_GROOVE; j++) {
            if (data_[id][j] == 0) {
              data_[id][j] = NO_GROOVE_DATA;
            }
          }
        } else {
          reader.Skip();
        }
      }
    }
  }
}

// Trigger grooves so we go to the next step
void Groove::Trigger() {
  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
    ChannelGroove &c = channelGroove_[i];
    UpdateGroove(c, false);
  }
}

bool Groove::UpdateGroove(ChannelGroove &c, bool reverse) {

  bool stepped = false;

  if (reverse) { // Table
    c.ticks_++;
    if (c.groove_ == 255) { // Default table groove
      if (c.ticks_ == 1) {
        stepped = true;
        c.ticks_ = 0;
      }
    } else {
      if (c.ticks_ == data_[c.groove_][c.position_]) {
        c.position_ = (c.position_ + 1) % STEPS_PER_GROOVE;
        if (data_[c.groove_][c.position_] == 0xFF || data_[c.groove_][c.position_] == 0) {
          c.position_ = 0;
        };
        c.ticks_ = 0;
        stepped = true;
      }
    }
  } else { // Note
    if (c.ticks_ == 0) {
      c.position_ = (c.position_ + 1) % STEPS_PER_GROOVE;
      if (data_[c.groove_][c.position_] == 0xFF || data_[c.groove_][c.position_] == 0) {
        c.position_ = 0;
      };
      c.ticks_ = data_[c.groove_][c.position_];
      stepped = true;
    };
    c.ticks_--;
  }
  return stepped;
}

void Groove::SetGroove(int channel, int groove) {
  if (groove >= MAX_GROOVES)
    return;
  channelGroove_[channel].groove_ = groove;
  channelGroove_[channel].position_ = 0;
  channelGroove_[channel].ticks_ = data_[channelGroove_[channel].groove_][channelGroove_[channel].position_];
}

// Returns true if, according to current groove setting it is time to go
// to the next sequencing step

bool Groove::TriggerChannel(int i) {
  ChannelGroove &c = channelGroove_[i];
  const unsigned char length = data_[c.groove_][c.position_];
  // a step of 0 ticks cannot be, it plays on every tick instead of dividing by zero
  return length == 0 || (c.ticks_ % length) == 0;
}

const unsigned char *Groove::GetGrooveData(int groove) const {
  return data_[groove];
}

unsigned char *Groove::EditGrooveData(int groove) {
  UndoJournal::RecordChange(data_[groove], STEPS_PER_GROOVE);
  return data_[groove];
}
