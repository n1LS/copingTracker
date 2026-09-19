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

#include "UIField.h"

#include "Application/AppWindow.h"
#include <string.h>

UIField::UIField(const GUIPoint &position) {
  x_ = position.x_;
  y_ = position.y_;
  focus_ = false;
}

UIField::~UIField() {};

GUIPoint UIField::GetPosition() {
  GUIPoint point(x_, y_);
  return point;
}

void UIField::SetPosition(const GUIPoint &p) {
  x_ = p.x_;
  y_ = p.y_;
}

void UIField::ClearFocus() {
  focus_ = false;
}

void UIField::SetFocus() {
  focus_ = true;
}

bool UIField::HasFocus() {
  return focus_;
}

bool UIField::IsStatic() {
  return false;
}

void UIField::SetActive(bool active) {
  active_ = active;
}

int UIField::DrawLabeledField(GUIWindow &w, GUIPoint position, char *buffer, int subSelectionOffset,
                              int subSelectionLength) {
  w.SetBackgroundColor(backgroundColor_);

  GUIPoint basePosition = position;

  char *colon = strchr(buffer, ':');

  // Draw the Label first

  int colonIndex = (int)(colon - buffer);
  int valueOffset = colonIndex + 1;

  if (colon) {
    buffer[colonIndex] = 0;

    w.SetColor(active_ ? labelColor_ : inactiveLabelColor_);
    w.DrawString(position.x_, position.y_, buffer);

    position.x_ += colonIndex + 1;
    buffer += colonIndex + 1;
  }

  const int valueLength = static_cast<int>(strlen(buffer));

  if (focus_) {
    // focused value drawing, needs a cursor
    w.SetBackgroundColor(fieldConfig_.activeBackgroundColor);
    w.SetColor(fieldConfig_.activeColor);

    // draw value
    w.DrawString(position.x_, position.y_, buffer);

    // overdraw the subselection
    for (int c = 0; c < subSelectionLength; c++) {
      w.SetBackgroundColor(fieldConfig_.activeColor);
      w.SetColor(fieldConfig_.activeBackgroundColor);

      char character = buffer[c + subSelectionOffset];
      w.DrawChar(position.x_ + subSelectionOffset + c, position.y_, character);
    }
  } else {
    // unfocused value drawing

    w.SetColor(fieldConfig_.color);
    w.SetBackgroundColor(fieldConfig_.backgroundColor);
    w.DrawString(position.x_, position.y_, buffer);
  }

  // draw field ends (rounded buttons when focused, square blocks when unfocused)

  char front = focus_ ? char_button_left(pressed_) : CHAR(char_block_left_s);
  char end = focus_ ? char_button_right(pressed_) : CHAR(char_block_right_s);

  w.SetBackgroundColor(backgroundColor_);

  if (focus_) {
    w.SetColor(fieldConfig_.activeBackgroundColor);
  } else {
    w.SetColor(fieldConfig_.backgroundColor);
  }

  w.DrawChar(basePosition.x_ + valueOffset - 1, basePosition.y_, front);
  w.DrawChar(basePosition.x_ + valueOffset + strlen(buffer), basePosition.y_, end);

  return valueLength + 2;
}
