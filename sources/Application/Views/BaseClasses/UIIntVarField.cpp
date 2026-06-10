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

#include "UIIntVarField.h"

#include "Application/AppWindow.h"
#include "System/Console/Trace.h"
#include "UIFramework/Interfaces/I_GUIGraphics.h"
#include "UIIntVarField.h"
#include "ViewUtils.h"
#include <System/Console/nanoprintf.h>
#include <string.h>

#define abs(x) (x < 0 ? -x : x)

UIIntVarField::UIIntVarField(const GUIPoint &position, Variable &v, const char *format, int min, int max, int xOffset,
                             int yOffset, int displayOffset)
    : UIField(position), src_(v) {
  format_ = format;
  min_ = min;
  max_ = max;
  xOffset_ = xOffset;
  yOffset_ = yOffset;
  displayOffset_ = displayOffset;
}

void UIIntVarField::Draw(GUIWindow &w, int offset) {
  GUIPoint position = GetPosition();
  position.y_ += offset;

  Variable::Type type = src_.GetType();
  char buffer[MAX_FIELD_WIDTH + 1];
  switch (type) {
    case Variable::INT:
      {
        int ivalue = src_.GetInt() + displayOffset_;
        npf_snprintf(buffer, sizeof(buffer), format_, ivalue, ivalue);
      }
      break;
    case Variable::CHAR_LIST:
      // if no value initialize with "NONE"
      if (src_.GetInt() < 0) {
        npf_snprintf(buffer, sizeof(buffer), format_, "None");
      } else {
        auto value = src_.GetString();
        const char *cvalue = value.c_str();
        npf_snprintf(buffer, sizeof(buffer), format_, cvalue);
      }
      break;
    case Variable::BOOL:
      {
        auto value = src_.GetString();
        const char *cvalue = value.c_str();
        npf_snprintf(buffer, sizeof(buffer), format_, cvalue);
      }
      break;

    default:
      strcpy(buffer, "++wtf++");
  }

  DrawLabeledField(w, position, buffer, focus_);
}

void UIIntVarField::ProcessArrow(uint16_t mask) {
  int value = src_.GetInt();

  switch (mask) {
    case BM_UP:
      value += yOffset_;
      break;
    case BM_DOWN:
      value -= yOffset_;
      break;
    case BM_LEFT:
      value -= xOffset_;
      break;
    case BM_RIGHT:
      value += xOffset_;
      break;
  };
  if (value < min_) {
    value = min_;
  };
  if (value > max_) {
    value = max_;
  }

  src_.SetInt(value);

  SetChanged();
  NotifyObservers(reinterpret_cast<I_ObservableData *>(static_cast<uintptr_t>(src_.GetID())));
}

void UIIntVarField::ProcessClear() {
  if (!src_.IsModified())
    return;

  src_.Reset();

  SetChanged();
  NotifyObservers(reinterpret_cast<I_ObservableData *>(static_cast<uintptr_t>(src_.GetID())));
}

FourCC UIIntVarField::GetVariableID() {
  return src_.GetID();
}

Variable &UIIntVarField::GetVariable() {
  return src_;
}

void UIIntVarField::SetRange(int min, int max, int xOffset, int yOffset) {
  min_ = min;
  max_ = max;
  xOffset_ = xOffset;
  yOffset_ = yOffset;
}
