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
#include "Application/Utils/HexBuffers.h"
#include "Application/Utils/char.h"
#include "Song.h"
#include "System/System/System.h"

Table::Table() { Reset(); };

void Table::Reset() {
  for (int i = 0; i < TABLE_STEPS; i++) {
    steps_[i].cmd1 = static_cast<uint8_t>(FourCC::InstrumentCommandNone);
    steps_[i].cmd2 = static_cast<uint8_t>(FourCC::InstrumentCommandNone);
    steps_[i].cmd3 = static_cast<uint8_t>(FourCC::InstrumentCommandNone);
    steps_[i]._pad = 0;
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
  const uint8_t none = static_cast<uint8_t>(FourCC::InstrumentCommandNone);
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

TableHolder::TableHolder() : Persistent("Tables") { Reset(); }

void TableHolder::Reset() {
  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
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

void TableHolder::SaveContent(tinyxml2::XMLPrinter *printer) {

  char hex[3];
  for (int i = 0; i < TABLE_COUNT; i++) {
    printer->OpenElement("Table");
    hex2char(i, hex);
    printer->PushAttribute("id", hex);

    Table &table = table_[i];
    if (!table.IsEmpty()) {
      uint8_t cmd1[TABLE_STEPS], cmd2[TABLE_STEPS], cmd3[TABLE_STEPS];
      uint16_t param1[TABLE_STEPS], param2[TABLE_STEPS], param3[TABLE_STEPS];
      for (int j = 0; j < TABLE_STEPS; j++) {
        cmd1[j] = table.steps_[j].cmd1;
        cmd2[j] = table.steps_[j].cmd2;
        cmd3[j] = table.steps_[j].cmd3;
        param1[j] = table.steps_[j].param1;
        param2[j] = table.steps_[j].param2;
        param3[j] = table.steps_[j].param3;
      }
      saveHexBuffer(printer, "Command1", cmd1, TABLE_STEPS);
      saveHexBuffer(printer, "Value1", param1, TABLE_STEPS);
      saveHexBuffer(printer, "Command2", cmd2, TABLE_STEPS);
      saveHexBuffer(printer, "Value2", param2, TABLE_STEPS);
      saveHexBuffer(printer, "Command3", cmd3, TABLE_STEPS);
      saveHexBuffer(printer, "Value3", param3, TABLE_STEPS);
    }
    printer->CloseElement();
  }
}

void TableHolder::RestoreContent(PersistencyDocument *doc) {

  bool elem = doc->FirstChild();
  while (elem) {
    // Check it is a table
    if (!strcmp(doc->ElemName(), "Table")) {
      // Get the table ID
      unsigned char id = '\0';
      bool attr = doc->NextAttribute();
      while (attr) {
        if (!strcmp(doc->attrname_, "id")) {
          unsigned char b1 = (c2h__(doc->attrval_[0])) << 4;
          unsigned char b2 = c2h__(doc->attrval_[1]);
          id = b1 + b2;
          // found what we wanted
          break;
        }
        attr = doc->NextAttribute();
      }

      Table &table = table_[id];

      bool subelem = doc->FirstChild();
      while (subelem) {
        uint8_t cbuf[TABLE_STEPS];
        uint16_t pbuf[TABLE_STEPS];
        if (!strcmp("Command1", doc->ElemName())) {
          restoreHexBuffer(doc, cbuf);
          for (int j = 0; j < TABLE_STEPS; j++)
            table.steps_[j].cmd1 = cbuf[j];
        };
        if (!strcmp("Value1", doc->ElemName())) {
          restoreHexBuffer(doc, (unsigned char *)pbuf);
          for (int j = 0; j < TABLE_STEPS; j++)
            table.steps_[j].param1 = pbuf[j];
        };
        if (!strcmp("Command2", doc->ElemName())) {
          restoreHexBuffer(doc, cbuf);
          for (int j = 0; j < TABLE_STEPS; j++)
            table.steps_[j].cmd2 = cbuf[j];
        };
        if (!strcmp("Value2", doc->ElemName())) {
          restoreHexBuffer(doc, (unsigned char *)pbuf);
          for (int j = 0; j < TABLE_STEPS; j++)
            table.steps_[j].param2 = pbuf[j];
        };
        if (!strcmp("Command3", doc->ElemName())) {
          restoreHexBuffer(doc, cbuf);
          for (int j = 0; j < TABLE_STEPS; j++)
            table.steps_[j].cmd3 = cbuf[j];
        };
        if (!strcmp("Value3", doc->ElemName())) {
          restoreHexBuffer(doc, (unsigned char *)pbuf);
          for (int j = 0; j < TABLE_STEPS; j++)
            table.steps_[j].param3 = pbuf[j];
        };
        subelem = doc->NextSibling();
      }
      allocation_[id] = !table.IsEmpty();
    }
    elem = doc->NextSibling();
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
