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

#include "UIBitmaskVarField.h"
#include "Application/AppWindow.h"
#include "ViewUtils.h"
#include <System/Console/nanoprintf.h>
#include <string.h>

UIBitmaskVarField::UIBitmaskVarField(const GUIPoint &position, Variable &v, const char *format, int len)
    : UIIntVarField(position, v, format, 0, 0xffff, 0, 0, 0) {
  len_ = len;
  format_ = format;
}

void UIBitmaskVarField::Draw(GUIWindow &w, int offset) {

  GUIPoint position = GetPosition();
  position.y_ += offset;

  char buffer[MAX_FIELD_WIDTH + 1];
  int value = src_.GetInt();
  npf_snprintf(buffer, sizeof(buffer), format_, value);

  if (false && focus_) {
    ((AppWindow &)w).SetColor(cHighlight2);
    w.DrawString(buffer, position);

    int percentPos = -1;
    for (unsigned int i = 0; i < strlen(format_); i++) {
      if (format_[i] == '%') {
        percentPos = i;
        break;
      };
    };
    if (percentPos >= 0) {
      int offset = (len_ - position_) + percentPos;
      buffer[offset + 1] = 0;
      position.x_ += offset;
      ((AppWindow &)w).SetColor(cNormal);
      w.DrawString(buffer + offset, position);
    }
  } else {
    DrawLabeledField(w, position, buffer, focus_);
  }
}

void UIBitmaskVarField::ProcessArrow(unsigned short mask) {

  int value = src_.GetInt();

  switch (mask) {
  case EPBM_LEFT:
    if (position_ < len_) {
      position_++;
    };
    break;
  case EPBM_RIGHT:
    if (position_ > 0) {
      position_--;
    };
    break;
  case EPBM_UP:
    value = value ^ (1 << (position_ - 1));
    break;

  case EPBM_DOWN:
    value = value ^ (1 << (position_ - 1));
    break;
  };

  src_.SetInt(value);

  SetChanged();
  NotifyObservers(reinterpret_cast<I_ObservableData *>(static_cast<uintptr_t>(src_.GetID())));
}
