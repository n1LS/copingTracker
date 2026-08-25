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

#include "UIActionField.h"
#include "Application/AppWindow.h"
#include "ViewUtils.h"
#include <string.h>

UIActionField::UIActionField(const char *name, unsigned int fourcc, GUIPoint &position) : UIField(position) {
  name_ = name;
  fourcc_ = fourcc;
}

UIActionField::~UIActionField() {};

void UIActionField::Draw(GUIWindow &w, int offset) {
  int x = x_;
  int y = y_ + offset;

  // enforce max field length 
  char buffer[MAX_FIELD_WIDTH + 1];
  snprintf(buffer, sizeof(buffer), "(%.*s)", MAX_FIELD_WIDTH, name_);
  strncpy(buffer, name_, MAX_FIELD_WIDTH);
  buffer[MAX_FIELD_WIDTH] = '\0';

  ((AppWindow &)w).SetBackgroundColor(Theme::Button::bg(focus_));
  ((AppWindow &)w).SetColor(Theme::Button::fg(focus_));
  w.DrawString(x, y, buffer);

  // add button ends
  // draw highlight button ends
  char front = focus_ ? CHAR(char_button_border_left_s) : ' ';
  char end = focus_ ? CHAR(char_button_border_right_s) : ' ';

  if (focus_) {
    ((AppWindow &)w).SetColor(Theme::View::bg);
    ((AppWindow &)w).SwapColors();
  }

  x -= 1;
  w.DrawChar(x, y, front);
  x += strlen(buffer) + 1;
  w.DrawChar(x, y, end);
}

void UIActionField::OnClick() {
  SetChanged();
  NotifyObservers((I_ObservableData *)fourcc_);
}

const char *UIActionField::GetString() {
  return name_;
}
