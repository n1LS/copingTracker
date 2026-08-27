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

#include "UIIntField.h"
#include "Application/AppWindow.h"
#include "System/Console/Trace.h"
#include "UIFramework/Interfaces/I_GUIGraphics.h"
#include <System/Console/nanoprintf.h>

#define abs(x) (x < 0 ? -x : x)

UIIntField::UIIntField(const GUIPoint &position, int *src, const char *format, int min, int max, int xOffset,
                       int yOffset)
    : UIField(position) {
  src_ = src;
  format_ = format;
  min_ = min;
  max_ = max;
  xOffset_ = xOffset;
  yOffset_ = yOffset;
}

void UIIntField::Draw(GUIWindow &w) {

  GUIPoint position = GetPosition();

  // ensure max field length
  char buffer[MAX_FIELD_WIDTH + 1];
  int value = *src_;
  npf_snprintf(buffer, sizeof(buffer), format_, value);

  DrawLabeledField(w, position, buffer);
}

void UIIntField::ProcessArrow(uint16_t mask) {

  int value = *src_;

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

  *src_ = value;
}
