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

int UIField::DrawLabeledField(GUIWindow &w, GUIPoint position, char *buffer, int subSelectionOffset,
                              int subSelectionLength) {
  w.SetBackgroundColor(Theme::View::bg);
  w.SetColor(Theme::View::fg);

  GUIPoint basePosition = position;

  char *colon = strchr(buffer, ':');
  int valueOffset = 0;
  char *value = buffer;

  if (colon) {
    int labelLength = colon - buffer;

    // Temporarily terminate the label.
    *colon = '\0';

    w.SetColor(Theme::Input::label);
    w.DrawString(position.x_, position.y_, buffer);

    // Restore the caller's buffer.
    *colon = ':';

    valueOffset = labelLength + 1;
    position.x_ += valueOffset;
    value = colon + 1;
  }

  const int valueLength = static_cast<int>(strlen(value));

  if (focus_) {
    w.SetBackgroundColor(Theme::Input::bg(true));
    w.SetColor(Theme::Input::fg(true));

    w.DrawString(position.x_, position.y_, value);

    int valueSubSelectionOffset = subSelectionOffset - valueOffset;

    if (subSelectionOffset >= valueOffset && valueSubSelectionOffset < valueLength && subSelectionLength > 0) {
      if (valueSubSelectionOffset + subSelectionLength > valueLength) {
        subSelectionLength = valueLength - valueSubSelectionOffset;
      }

      char replaced = value[valueSubSelectionOffset + subSelectionLength];

      value[valueSubSelectionOffset + subSelectionLength] = '\0';

      position.x_ += valueSubSelectionOffset;

      w.SetBackgroundColor(Theme::Input::cursor);
      w.SetColor(Theme::Input::fg(true));

      w.DrawString(position.x_, position.y_, value + valueSubSelectionOffset);

      value[valueSubSelectionOffset + subSelectionLength] = replaced;
    }
  } else {
    w.SetColor(Theme::Input::fg(false));
    w.DrawString(position.x_, position.y_, value);
  }

  // draw highlight button ends
  char front = focus_ ? char_button_left(pressed_) : ' ';
  char end = focus_ ? char_button_right(pressed_) : ' ';

  if (focus_) {
    w.SetColor(Theme::Input::bg(true));
    w.SetBackgroundColor(Theme::View::bg);
  }

  w.DrawChar(basePosition.x_ + valueOffset - 1, basePosition.y_, front);
  w.DrawChar(basePosition.x_ + strlen(buffer), basePosition.y_, end);

  return valueLength + 2;
}