/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

 #include "UITabField.h"
 #include "Application/AppWindow.h"

 UITabField::UITabField(const char *label, const GUIPoint &position, Variable &variable, const char *tabs[], int count) : UIIntVarField(position, variable, "", 0, count - 1, 1, count - 1) , src_(variable) {
  max_ = count - 1;
  label_ = label;

  src_ = variable;

  for (int n = 0; n < count; n++) {
    tabs_[n] = tabs[n];
  }
}

void UITabField::Draw(GUIWindow &window, int offset) {
  AppWindow &w = ((AppWindow &)window);
  GUIPoint position = GetPosition();

  int index = src_.GetInt();

  w.SetColor(Theme::View::fg);
  w.DrawString(position.x_, position.y_, label_);

  int x = position.x_ + strlen(label_) + 1;

  const char *space = " ";
  const char *noSpace = "";

  const char **nextSpace = &space;

  for (int t = 0; t < max_; t++) {
    int len = strlen(tabs_[t]); 

    if (index == t) {
      w.SetBackgroundColor(Theme::View::bg);
      w.SetColor(Theme::View::Button::bg(true));
      w.DrawChar(x, position.y_, CHAR(char_button_border_left_s));
      w.DrawChar(x + 1 + len, position.y_, CHAR(char_button_border_left_s));
      
      w.SetBackgroundColor(Theme::View::Button::bg(true));
      w.SetColor(Theme::View::Button::fg(true));
      w.DrawString(x, position.y_, tabs_[t]);

      nextSpace = &noSpace;
      focusWidth_ = 2 + len;
    } else {
      w.SetBackgroundColor(Theme::View::bg);
      w.SetColor(Theme::View::inactive);
      w.DrawString(x, position.y_, *nextSpace);
      w.DrawString(x + 1, position.y_, tabs_[t]);
      nextSpace =  &space;
    }

    x += 1 + len;
  }

  // last space
  w.DrawString(x + 1, position.y_, " ");

  // TODO nILS: if we ever need scrolling, this is where to put the indicator + handle the scroll position
}

int UITabField::GetFocusOffset() {
  return 0;
}

void Update(Observable &o, I_ObservableData *d) {

}