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
    : UIIntVarField(position, v, format, 0, v.GetListSize(), 0, 0 /*dummy 4 last*/) {
  NAssert(v.GetType() == Variable::CHAR_LIST);
}

void UISortedVarList::ProcessArrow(uint16_t mask) {
  int value = src_.GetInt();

  switch (mask) {
    case EPBM_UP:
      //			HERE
      /*			// Look for the first in next alphabet
                              char search=src_.GetString()[0] ;

                              value+=yOffset_ ;
      */
      break;
    case EPBM_DOWN:
    /*			value-=yOffset_ ;
                            break ;
    */
    case EPBM_LEFT:
      value -= 1;
      break;
    case EPBM_RIGHT:
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
