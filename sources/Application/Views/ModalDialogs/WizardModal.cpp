/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "WizardModal.h"
#include "Application/AppWindow.h"
#include "Foundation/Constants/GraphicCharacters.h"
#include "Foundation/Constants/SpecialCharacters.h"
#include "System/Console/nanoprintf.h"
#include <string.h>

#define LABEL_X 2
#define VALUE_X 12
#define PROMPT_Y 3
#define PANEL_Y 4
#define PANEL_H 6
#define ROWS_Y (PANEL_Y + 1)
#define VALUE_STEP 16

WizardModal::WizardModal(View &view, int width, int height) : ModalView(view), width_(width), height_(height) {
}

WizardModal::~WizardModal() {
}

int WizardModal::valueWidth() const {
  return width_ - VALUE_X - 4;
}

// --- drawing -----------------------------------------------------------------

void WizardModal::DrawView() {
  SetBackgroundColor(Theme::Dialog::bg);

  left_ = (SCREEN_WIDTH - width_) / 2;
  top_ = (SCREEN_HEIGHT - height_) / 2;

  DrawWindow(0, 0, width_, height_, GetTitle());
  drawProgress();

  const StepInfo info = GetStepInfo(step_);

  SetBackgroundColor(Theme::Dialog::bg);
  SetColor(Theme::Dialog::fg);
  DrawString(LABEL_X, PROMPT_Y, info.prompt);

  DrawFilledBorder(1, PANEL_Y, width_ - 2, PANEL_H, Theme::Dialog::Panel::bg, true);
  DrawStep(step_);

  drawButtons(info);
}

// Right aligned progress bar in the title bar. step_ is zero based, so the bar
// is full on the last step.
void WizardModal::drawProgress() {
  char bar[7];
  horizontal_bar_graph_6(bar, (uint8_t)((step_ + 1) * 60 / NumSteps()));

  SetBackgroundColor(Theme::Dialog::Title::bg);
  SetColor(Theme::Dialog::Title::fg);
  DrawString(width_ - 1 - 6, 1, bar, fGraphic);
}

void WizardModal::drawField(int x, int y, const char *text, int width, bool focused, char indicator) {
  const bool down = focused && enterDown_;
  char buf[SCREEN_WIDTH + 1];
  if (width > SCREEN_WIDTH) {
    width = SCREEN_WIDTH;
  }

  int len = 0;
  while (len < width - 1 && text[len]) {
    buf[len] = text[len];
    len++;
  }
  while (len < width) {
    buf[len++] = ' ';
  }

  if (focused) {
    buf[len - 1] = indicator;
  }

  buf[len] = 0;

  SetColor(Theme::Dialog::Panel::Input::fg(focused));
  SetBackgroundColor(Theme::Dialog::Panel::Input::bg(focused));
  DrawString(x + 1, y, buf);

  SetColor(Theme::Dialog::Panel::Input::bg(focused));
  SetBackgroundColor(Theme::Dialog::Panel::bg);
  DrawChar(x, y, focused ? char_button_left(down) : CHAR(char_block_left_s));
  DrawChar(x + 1 + width, y, focused ? char_button_right(down) : CHAR(char_block_right_s));

  if (focused) {
    focusRect_ = GUIRect(x + left_, y + top_, width + 2, 1);
  }
}

void WizardModal::drawListItem(int line, const char *text, bool focused) {
  drawField(LABEL_X, ROWS_Y + line, text, fieldWidth(), focused, CHAR(char_button_enter_s));
}

void WizardModal::drawLabeledField(int line, const char *label, const char *value, bool focused) {
  SetBackgroundColor(Theme::Dialog::Panel::bg);
  SetColor(Theme::Dialog::fg);
  DrawString(LABEL_X, ROWS_Y + line, label);

  drawField(VALUE_X, ROWS_Y + line, value, valueWidth(), focused);
}

void WizardModal::drawPanelText(int line, const char *text) {
  SetBackgroundColor(Theme::Dialog::Panel::bg);
  SetColor(Theme::Dialog::fg);
  DrawString(LABEL_X, ROWS_Y + line, text);
}

// Left button (Cancel / Back) on the left, right button (Next / Generate...) on
// the right.
void WizardModal::drawButtons(const StepInfo &info) {
  const int y = height_ - 3;
  const bool onButtons = isButtonRow();
  const bool leftSel = onButtons && button_ == BTN_LEFT;
  const bool rightSel = onButtons && button_ == BTN_RIGHT;

  char label[SCREEN_WIDTH + 1];

  const char *left = info.leftLabel ? info.leftLabel : (step_ == 0 ? "Cancel" : "Back");
  npf_snprintf(label, sizeof(label), " %s ", left);
  DrawButton(LABEL_X, y, label, leftSel, leftSel && enterDown_, Theme::Dialog::bg);
  if (leftSel) {
    focusRect_ = GUIRect(LABEL_X + left_, y + top_, (int)strlen(label) + 2, 1);
  }

  if (info.rightLabel) {
    npf_snprintf(label, sizeof(label), " %s ", info.rightLabel);
    const int len = (int)strlen(label);
    const int x = width_ - len - 4;
    DrawButton(x, y, label, rightSel, rightSel && enterDown_, Theme::Dialog::bg);
    if (rightSel) {
      focusRect_ = GUIRect(x + left_, y + top_, len + 2, 1);
    }
  }
}

// --- navigation --------------------------------------------------------------

// Keeps the focused row inside the visible window of the list.
void WizardModal::updateScroll() {
  if (row_ >= RowCount(step_)) {
    return; // button row
  }
  if (row_ < scroll_) {
    scroll_ = row_;
  } else if (row_ >= scroll_ + kVisibleRows) {
    scroll_ = row_ - (kVisibleRows - 1);
  }
}

void WizardModal::goToStep(uint8_t step, bool forward) {
  step_ = step;
  row_ = 0;
  scroll_ = 0;
  // a step with a primary action starts on it, the others on their only button
  button_ = GetStepInfo(step).rightLabel ? BTN_RIGHT : BTN_LEFT;

  OnEnterStep(step, forward);
  updateScroll();
  isDirty_ = true;
}

void WizardModal::advance() {
  if (!OnNext(step_, row_)) {
    return;
  }
  if (step_ + 1 >= NumSteps()) {
    EndModal(OnAccept() ? WM_ACCEPT : WM_CANCEL);
    return;
  }
  goToStep(step_ + 1, true);
}

void WizardModal::back() {
  if (step_ == 0) {
    EndModal(WM_CANCEL);
    return;
  }
  goToStep(step_ - 1, false);
}

// ENTER was released
void WizardModal::activate() {
  if (isButtonRow()) {
    if (button_ == BTN_LEFT) {
      back();
    } else {
      advance();
    }
  } else if (!GetStepInfo(step_).rightLabel) {
    // no primary button: ENTER on a row picks it
    advance();
  }
}

void WizardModal::ProcessButtonMask(uint16_t mask, bool pressed) {
  const uint16_t previous = lastMask_;
  lastMask_ = mask;

  if (!pressed) {
    // actions fire on ENTER release, so the release doesn't leak into
    // whatever the modal hands control to next
    if (enterDown_ && (previous & ~mask) == BM_ENTER) {
      enterDown_ = false;
      isDirty_ = true;

      if (valueChanged_) {
        valueChanged_ = false; // ENTER was only a modifier for editing
      } else {
        activate();
      }
    } else if (enterDown_ && !(mask & BM_ENTER)) {
      enterDown_ = false;
      valueChanged_ = false;
      isDirty_ = true;
    }
    return;
  }

  // rows 0..RowCount()-1 are the fields, RowCount() is the button row
  const uint8_t focusCount = RowCount(step_) + 1;

  if (mask == BM_ENTER) {
    enterDown_ = true;
  }

  if (enterDown_) {
    if ((mask & BM_DIRECTIONAL) && !isButtonRow()) {
      int delta;
      if (mask & BM_RIGHT) {
        delta = 1;
      } else if (mask & BM_LEFT) {
        delta = -1;
      } else {
        delta = (mask & BM_DOWN) ? -VALUE_STEP : VALUE_STEP;
      }
      OnValueChange(step_, row_, delta);
      valueChanged_ = true;
    }
  } else {
    if (mask == BM_UP || mask == BM_DOWN) {
      row_ = (row_ + focusCount + (mask == BM_UP ? -1 : 1)) % focusCount;
      if (!isButtonRow()) {
        OnRowFocused(step_, row_);
      }
      updateScroll();
    } else if (isButtonRow() && GetStepInfo(step_).rightLabel && (mask & (BM_LEFT | BM_RIGHT))) {
      button_ = (button_ == BTN_LEFT) ? BTN_RIGHT : BTN_LEFT;
    }
  }

  isDirty_ = true;
}
