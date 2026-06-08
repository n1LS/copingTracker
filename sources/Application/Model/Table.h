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

#ifndef _TABLE_H_

#define _TABLE_H_

#include "Application/Persistency/Persistent.h"
#include "Foundation/T_Singleton.h"
#include "Foundation/Types/Types.h"
#include <stdint.h>

#define TABLE_COUNT 0x20
#define TABLE_STEPS 16
#define TABLE_COLUMNS 3

#define NO_MORE_TABLE TABLE_COUNT + 10

struct TableStep {
  uint8_t  cmd1;
  uint8_t  cmd2;
  uint8_t  cmd3;
  uint8_t  _pad;
  uint16_t param1;
  uint16_t param2;
  uint16_t param3;
};

class Table {
public:
  Table();
  void Reset();
  bool IsEmpty();
  void Copy(const Table &other);

  inline FourCC getCmd(int step, int col) const {
    switch (col) {
      case 0:  return FourCC::enum_type(steps_[step].cmd1);
      case 1:  return FourCC::enum_type(steps_[step].cmd2);
      default: return FourCC::enum_type(steps_[step].cmd3);
    }
  }

  inline uint16_t getParam(int step, int col) const {
    switch (col) {
      case 0:  return steps_[step].param1;
      case 1:  return steps_[step].param2;
      default: return steps_[step].param3;
    }
  }

public:
  TableStep steps_[TABLE_STEPS];
};

class TableHolder : public T_Singleton<TableHolder>, Persistent {
public:
  TableHolder();
  void Reset();
  Table &GetTable(int table);
  void SetUsed(int table);
  int GetNext();
  int Clone(int table);
  virtual void SaveContent(tinyxml2::XMLPrinter *printer);
  virtual void RestoreContent(PersistencyDocument *doc);

private:
  Table table_[TABLE_COUNT];
  bool allocation_[TABLE_COUNT];
};

#endif
