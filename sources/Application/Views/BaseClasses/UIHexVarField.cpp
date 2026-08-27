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

#include "UIHexVarField.h"
#include "Application/AppWindow.h"
#include "ViewUtils.h"
#include <System/Console/nanoprintf.h>
#include <string.h>

UIHexVarField::UIHexVarField(const GUIPoint &position, Variable &v, int precision, const char *format, int min, int max,
                             int power, bool wrap)
    : UIIntVarField(position, v, format, min, max, 0, 0) {
  precision_ = precision - 1;
  power_ = power;
  position_ = 0;
  wrap_ = wrap;
}

void UIHexVarField::Draw(GUIWindow &w, int offset) {

  GUIPoint position = GetPosition();
  position.y_ += offset;

  char buffer[MAX_FIELD_WIDTH + 1];
  int value = src_.GetInt();
  npf_snprintf(buffer, sizeof(buffer), format_, value);

  int subSelectionOffset = -1;
  int valueOffset = FindFormatValueOffset(format_);
  if (valueOffset >= 0) {
    subSelectionOffset = valueOffset + (precision_ - position_);
  }
  DrawLabeledField(w, position, buffer, subSelectionOffset);
}

void UIHexVarField::ProcessArrow(uint16_t mask) {

  int32_t value = src_.GetInt();
  int32_t offset = 1;
  for (unsigned int i = 0; i < position_; i++) {
    offset *= power_;
  }

  switch (mask) {
    case BM_LEFT:
      if (position_ < precision_) {
        position_++;
      };
      break;
    case BM_RIGHT:
      if (position_ > 0) {
        position_--;
      };
      break;
    case BM_UP:
      if (wrapDigits_) {
        value += offset;
      } else {
        uint32_t bits = position_ * 4;
        uint32_t mask = 0xF << bits;
        int32_t nibble = (value >> bits) & 0xF;
        nibble = (nibble > 0xE) ? 0xF : nibble + 1;
        value = (value & ~mask) | (nibble << bits);
      }
      break;
    case BM_DOWN:
      if (wrapDigits_) {
        value -= offset;
      } else {
        uint32_t bits = position_ * 4;
        uint32_t mask = 0xF << bits;
        int32_t nibble = (value >> bits) & 0xF;
        nibble = (nibble > 0) ? nibble - 1 : 0;
        value = (value & ~mask) | (nibble << bits);
      }
      break;
  }

  if (value > max_) {
    value = (wrap_) ? value - max_ + min_ - 1 : max_;
  }

  if (value < min_) {
    value = (wrap_) ? max_ + (value - min_) + 1 : min_;
  }

  src_.SetInt(value);

  SetChanged();
  NotifyObservers(reinterpret_cast<I_ObservableData *>(static_cast<uintptr_t>(src_.GetID())));
}

void UIHexVarField::SetWrapDigits(bool wrapDigits) {
  wrapDigits_ = wrapDigits;
}

void UIHexVarField::SetWrap(bool wrap) {
  wrap_ = wrap;
}

void UIHexVarField::SetColumn(uint8_t index) {
  position_ = (index > 7) ? 7 : index;
}

int UIHexVarField::GetColumn() {
  return position_;
}