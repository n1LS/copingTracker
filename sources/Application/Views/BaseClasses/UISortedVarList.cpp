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

#include "UISortedVarList.h"

UISortedVarList::UISortedVarList(const GUIPoint &position, Variable &v, const char *format)
    : UIIntVarField(position, v, format, 0, v.GetListSize(), 0, 0) {
  NAssert(v.GetType() == Variable::CHAR_LIST);
}

void UISortedVarList::ProcessArrow(uint16_t mask) {
  int value = src_.GetInt();

  switch (mask) {
    case BM_UP:
      value = max_;
      break;
    case BM_DOWN:
      value = 0;
      break;
    case BM_LEFT:
      value -= 1;
      break;
    case BM_RIGHT:
      value += 1;
      break;
  };
  if (value < min_) {
    value = min_;
  };
  if (value > max_) {
    value = max_;
  }

  src_.SetInt(value);
}
