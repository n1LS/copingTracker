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

#include "MessageBox.h"
#include "System/Console/n_assert.h"
#include "System/Console/nanoprintf.h"
#include <Application/AppWindow.h>
#include <new>

static const char *buttonText[MBL_LAST] = {"Ok", "Yes", "Cancel", "No", "Stop"};

bool MessageBox::inUse_ = false;
alignas(MessageBox) static unsigned char MessageBoxStorage[sizeof(MessageBox)];
void *MessageBox::storage_ = MessageBoxStorage;

MessageBox *MessageBox::Create(View &view, const char *title, const char *message, int btnFlags) {
  if (inUse_) {
    auto *existing = reinterpret_cast<MessageBox *>(storage_);
    existing->~MessageBox();
    inUse_ = false;
  }
  inUse_ = true;
  return new (storage_) MessageBox(view, title, message, btnFlags);
}

MessageBox *MessageBox::Create(View &view, const char *title, const char *message, const char *message2, int btnFlags) {
  if (inUse_) {
    auto *existing = reinterpret_cast<MessageBox *>(storage_);
    existing->~MessageBox();
    inUse_ = false;
  }
  inUse_ = true;
  return new (storage_) MessageBox(view, title, message, message2, btnFlags);
}

MessageBox::MessageBox(View &view, const char *title, const char *message, int btnFlags)
    : ModalView(view), title_(title), line1_(message) {
  buttonCount_ = 0;

  for (int i = 0; i < MBL_LAST; i++) {
    if (btnFlags & (1 << (i))) {
      button_[buttonCount_] = i;
      buttonCount_++;
    }
  }

  selected_ = buttonCount_ - 1;
  NAssert(buttonCount_ != 0);
}

// Constructor for 2 line message
MessageBox::MessageBox(View &view, const char *title, const char *messageLine1, const char *messageLine2, int btnFlags)
    : ModalView(view), title_(title), line1_(messageLine1), line2_(messageLine2) {
  buttonCount_ = 0;

  for (int i = 0; i < MBL_LAST; i++) {
    if (btnFlags & (1 << (i))) {
      button_[buttonCount_] = i;
      buttonCount_++;
    }
  }

  selected_ = buttonCount_ - 1;
  NAssert(buttonCount_ != 0);
}

MessageBox::~MessageBox() {
}

void MessageBox::Destroy() {
  this->~MessageBox();
  inUse_ = false;
}

void MessageBox::DrawView() {
  SetBackgroundColor(Theme::Dialog::bg);

  // message size
  int size1 = (int)line1_.size();
  int size2 = (int)line2_.size();
  int messageWidth = std::max(size1, size2);

  // Calculate total button text width for centering
  int buttonWidth = 0;

  for (int i = 0; i < buttonCount_; i++) {
    const char *text = buttonText[button_[i]];
    buttonWidth += strlen(text) + 2; // +2 for border chars
  }

  // Ensure width is at least as wide as the buttons, title and messages
  int width = 4 + std::max(messageWidth, std::max(buttonWidth, (int)title_.size()));

  int numMessageLines = (line2_.size() > 0) ? 2 : 1;
  int height = 4 + numMessageLines + 3;

  // Use DrawWindow instead of DrawFilledBorder to get a title bar
  left_ = (SCREEN_WIDTH - width) / 2;
  top_ = (SCREEN_HEIGHT - height) / 2;
  DrawWindow(0, 0, width, height, title_.c_str());

  // draw text
  int y = 3; // start after title bar
  SetColor(Theme::Dialog::fg);
  DrawString(2, y, line1_.c_str());
  if (line2_.size() > 0) {
    y++;
    DrawString(2, y, line2_.c_str());
  }

  y += 2;       // gap before buttons
  buttonY_ = y; // Store the Y position of the buttons for focus rectangle calculation

  // Center the buttons based on total calculated width
  int buttonStartX = (width - buttonWidth) / 2;

  for (int i = 0; i < buttonCount_; i++) {
    bool sel = i == selected_;
    bool down = enterDown_ && sel;

    const char *text = buttonText[button_[i]];
    int textLen = (int)strlen(text);

    // Draw left border
    SetColor(Theme::Dialog::Button::bg(sel));
    SetBackgroundColor(Theme::Dialog::bg);
    DrawChar(buttonStartX, y, char_button_left(down));

    // Draw button text
    SetColor(Theme::Dialog::Button::fg(sel));
    SetBackgroundColor(Theme::Dialog::Button::bg(sel));
    DrawString(buttonStartX + 1, y, text);

    // Draw right border
    SetColor(Theme::Dialog::Button::bg(sel));
    SetBackgroundColor(Theme::Dialog::bg);
    DrawChar(buttonStartX + 1 + textLen, y, char_button_right(down));

    if (sel) {
      focusRect_ = GUIRect(buttonStartX + left_, y + top_, textLen + 2, 1);
    }

    buttonStartX += textLen + 3; // text + left border + right border + 1 space between buttons
  }
}

void MessageBox::OnPlayerUpdate(PlayerEventType, unsigned int currentTick) {
}

void MessageBox::OnFocus() {
}

void MessageBox::ButtonDown(uint16_t newButton, uint16_t mask) {
  // Dialogs are usually opened mid-chord, so other held buttons are ignored here
  if (newButton == BM_LEFT) {
    selected_ = (selected_ + 1);
    if (selected_ >= buttonCount_) {
      selected_ = 0;
    }
  } else if (newButton == BM_RIGHT) {
    selected_ = (selected_ - 1);
    if (selected_ < 0) {
      selected_ = buttonCount_ - 1;
    }
  } else if (newButton == BM_ENTER) {
    // the choice is confirmed on release (see ButtonUp)
    enterDown_ = true;
  }
  isDirty_ = true;
}

void MessageBox::ButtonUp(uint16_t button, uint16_t mask) {
  // Confirm on ENTER release, and only if the press happened inside this dialog:
  // the dialog is usually opened by an ENTER press whose release must not
  // dismiss it right away.
  if (button == BM_ENTER && enterDown_) {
    enterDown_ = false;
    EndModal(button_[selected_]);
  }
}
