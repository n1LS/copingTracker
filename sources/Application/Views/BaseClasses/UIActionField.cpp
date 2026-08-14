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

UIActionField::UIActionField(const char *name, unsigned int token, GUIPoint &position) : UIField(position) {
  name_ = name;
  token_ = token;
}

UIActionField::~UIActionField() {};

void UIActionField::Draw(GUIWindow &w, int offset) {
  GUIPoint position(x_, y_ + offset);

  // enforce max field length
  char buffer[MAX_FIELD_WIDTH + 1];
  snprintf(buffer, sizeof(buffer), "(%.*s)", MAX_FIELD_WIDTH, name_);
  strncpy(buffer, name_, MAX_FIELD_WIDTH);
  buffer[MAX_FIELD_WIDTH] = '\0';

  ((AppWindow &)w).SetBackgroundColor(Theme::Button::bg(focus_));
  ((AppWindow &)w).SetColor(Theme::Button::fg(focus_));
  w.DrawString(buffer, position);

  // add button ends
  // draw highlight button ends
  if (focus_) {
    ((AppWindow &)w).SetColor(Theme::View::bg);
    ((AppWindow &)w).SwapColors();
    position.x_ -= 1;
    w.DrawChar(CHAR(char_button_border_left_s), position);
    position.x_ += strlen(buffer) + 1;
    w.DrawChar(CHAR(char_button_border_right_s), position);
  } else {
    position.x_ -= 1;
    w.DrawChar(' ', position);
    position.x_ += strlen(buffer) + 1;
    w.DrawChar(' ', position);
  }
}

void UIActionField::OnClick() {
  SetChanged();
  NotifyObservers((I_ObservableData *)token_);
}

const char *UIActionField::GetString() {
  return name_;
}
