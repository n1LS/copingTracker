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

void UIField::DrawLabeledField(GUIWindow &w, GUIPoint position, char *buffer, int subSelectionOffset,
                               int subSelectionLength) {
  ((AppWindow &)w).SetBackgroundColor(Theme::View::bg);
  ((AppWindow &)w).SetColor(Theme::View::fg);

  char *colon = strchr(buffer, ':');
  int valueOffset = 0;

  if (colon) {
    int index = colon - buffer;
    buffer[index] = 0;
    valueOffset = index + 1;

    ((AppWindow &)w).SetColor(Theme::Input::label(active_));
    w.DrawString(position.x_, position.y_, buffer);

    position.x_ += index + 1;
    buffer += index + 1;
  }

  if (focus_) {
    ((AppWindow &)w).SetBackgroundColor(Theme::Input::bg(true));
    ((AppWindow &)w).SetColor(Theme::Input::fg(true));

    w.DrawString(position.x_, position.y_, buffer);

    int valueSubSelectionOffset = subSelectionOffset - valueOffset;
    if (subSelectionOffset >= valueOffset && valueSubSelectionOffset < (int)strlen(buffer) && subSelectionLength > 0) {
      int valueLength = strlen(buffer);
      if (valueSubSelectionOffset + subSelectionLength > valueLength) {
        subSelectionLength = valueLength - valueSubSelectionOffset;
      }

      char replaced = buffer[valueSubSelectionOffset + subSelectionLength];
      buffer[valueSubSelectionOffset + subSelectionLength] = 0;
      position.x_ += valueSubSelectionOffset;
      ((AppWindow &)w).SetBackgroundColor(Theme::Input::cursor);
      ((AppWindow &)w).SetColor(Theme::Input::fg(true));
      w.DrawString(position.x_, position.y_, buffer + valueSubSelectionOffset);
      buffer[valueSubSelectionOffset + subSelectionLength] = replaced;
    }
  } else {
    ((AppWindow &)w).SetColor(Theme::Input::fg(false));
    w.DrawString(position.x_, position.y_, buffer);
  }
}
