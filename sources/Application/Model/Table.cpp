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

#include "Table.h"
#include "Application/Instruments/CommandList.h"
#include "Application/Persistency/PersistenceConstants.h"
#include "Application/Utils/char.h"
#include "Song.h"
#include "System/System/System.h"

Table::Table() {
  Reset();
};

void Table::Reset() {
  for (int i = 0; i < TABLE_STEPS; i++) {
    steps_[i].cmd1 = Token(Token::InstrumentCommandNone).raw8();
    steps_[i].cmd2 = Token(Token::InstrumentCommandNone).raw8();
    steps_[i].cmd3 = Token(Token::InstrumentCommandNone).raw8();
    steps_[i].param1 = 0;
    steps_[i].param2 = 0;
    steps_[i].param3 = 0;
  }
}

void Table::Copy(const Table &other) {
  for (int i = 0; i < TABLE_STEPS; i++) {
    steps_[i] = other.steps_[i];
  }
}

bool Table::IsEmpty() {
  const uint8_t none = static_cast<uint8_t>(Token::InstrumentCommandNone);
  for (int i = 0; i < TABLE_STEPS; i++) {
    if (steps_[i].cmd1 != none || steps_[i].cmd2 != none || steps_[i].cmd3 != none)
      return false;
    if (steps_[i].param1 != 0 || steps_[i].param2 != 0 || steps_[i].param3 != 0)
      return false;
  }
  return true;
}
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

TableHolder::TableHolder() : Persistent("Tables") {
  Reset();
}

void TableHolder::Reset() {
  for (int i = 0; i < TABLE_COUNT; i++) {
    table_[i].Reset();
  }
  for (int i = 0; i < TABLE_COUNT; i++) {
    allocation_[i] = false;
  };
}

Table &TableHolder::GetTable(int table) {
  NAssert((table >= 0) && (table < TABLE_COUNT));
  return table_[table];
}

// only the tables that are not empty, and of those only the steps that are not empty:
//   "tables": [{"id": 3, "steps": [[step, command 1, value 1, command 2, value 2, command 3, value 3], ...]}]
void TableHolder::SaveContent(JsonWriter &writer) {
  writer.BeginArray("tables");
  for (int i = 0; i < TABLE_COUNT; i++) {
    Table &table = table_[i];
    if (table.IsEmpty()) {
      continue;
    }
    const uint8_t none = static_cast<uint8_t>(Token::InstrumentCommandNone);
    writer.BeginObject();
    writer.Int("id", i);
    writer.BeginArray("steps");
    for (int j = 0; j < TABLE_STEPS; j++) {
      const TableStep &s = table.steps_[j];
      if (s.cmd1 == none && s.cmd2 == none && s.cmd3 == none && s.param1 == 0 && s.param2 == 0 && s.param3 == 0) {
        continue;
      }
      const int step[7] = {j, s.cmd1, s.param1, s.cmd2, s.param2, s.cmd3, s.param3};
      writer.IntArray(nullptr, step, 7);
    }
    writer.EndArray();
    writer.EndObject();
  }
  writer.EndArray();
}

void TableHolder::RestoreContent(JsonReader &reader) {
  // what is not in the file is empty (the allocation is left alone: the song has marked the tables it uses, when it
  // was restored before)
  for (int i = 0; i < TABLE_COUNT; i++) {
    table_[i].Reset();
  }
  while (reader.NextKey()) {
    if (!reader.KeyIs("tables")) {
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
        if (reader.KeyIs("id")) {
          id = reader.ReadInt();
        } else if (reader.KeyIs("steps") && id >= 0 && id < TABLE_COUNT) {
          // the id comes first
          Table &table = table_[id];
          if (!reader.BeginArray()) {
            return;
          }
          while (reader.NextElement()) {
            int v[7] = {0};
            const size_t count = reader.ReadIntArray(v, 7);
            if (count >= 7 && v[0] >= 0 && v[0] < TABLE_STEPS) {
              TableStep &s = table.steps_[v[0]];
              s.cmd1 = static_cast<uint8_t>(v[1]);
              s.param1 = static_cast<uint16_t>(v[2]);
              s.cmd2 = static_cast<uint8_t>(v[3]);
              s.param2 = static_cast<uint16_t>(v[4]);
              s.cmd3 = static_cast<uint8_t>(v[5]);
              s.param3 = static_cast<uint16_t>(v[6]);
            }
          }
        } else {
          reader.Skip();
        }
      }
      if (id >= 0 && id < TABLE_COUNT) {
        allocation_[id] = !table_[id].IsEmpty();
      }
    }
  }
}

void TableHolder::SetUsed(int i) {
  if (i >= TABLE_COUNT) {
    NAssert(i < 128);
  }
  allocation_[i] = true;
}

int TableHolder::GetNext() {
  for (int i = 0; i < TABLE_COUNT; i++) {
    if (!allocation_[i]) {
      if (table_[i].IsEmpty()) {
        allocation_[i] = true;
        return i;
      }
    };
  };
  return NO_MORE_TABLE;
}

int TableHolder::Clone(int table) {
  int target = GetNext();
  if (target != NO_MORE_TABLE) {
    table_[target].Copy(table_[table]);
  };
  return target;
}
