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

  fieldConfig_ = actionFieldConfiguration;
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

  w.SetBackgroundColor(focus_ ? fieldConfig_.activeBackgroundColor : fieldConfig_.backgroundColor);
  w.SetColor(focus_ ? fieldConfig_.activeColor : fieldConfig_.color);
  w.DrawString(x, y, buffer);

  w.SetBackgroundColor(backgroundColor_);
  w.SetColor(focus_ ? fieldConfig_.activeBackgroundColor : fieldConfig_.backgroundColor);

  w.DrawChar(x - 1, y, CHAR(char_button_border_left_s));
  w.DrawChar(x + (int)strlen(buffer), y, CHAR(char_button_border_right_s));
}

void UIActionField::OnClick() {
  SetChanged();
  NotifyObservers((I_ObservableData *)token_);
}

const char *UIActionField::GetString() {
  return name_;
}

int UIActionField::GetFocusOffset() {
  return -1;
}

int UIActionField::GetFocusWidth() {
  return (int)strlen(name_) + 2;
}
