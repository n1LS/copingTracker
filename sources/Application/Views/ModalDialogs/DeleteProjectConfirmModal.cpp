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

#include "DeleteProjectConfirmModal.h"
#include "Application/AppWindow.h"
#include "Application/Utils/DrawUtils.h"
#include "Application/Views/ModalDialogs/MessageBox.h"
#include "System/System/System.h"
#include <algorithm>
#include <cstring>

#define DELETE_HOLDmask_ (BM_ALT | BM_PLAY | BM_EDIT)
#define DELETE_HOLD_DURATION_MS 2000

bool DeleteProjectConfirmModal::inUse_ = false;
alignas(
    DeleteProjectConfirmModal) static unsigned char DeleteProjectConfirmModalStorage[sizeof(DeleteProjectConfirmModal)];
void *DeleteProjectConfirmModal::storage_ = DeleteProjectConfirmModalStorage;

DeleteProjectConfirmModal *DeleteProjectConfirmModal::Create(View &view, const char *name) {
  if (inUse_) {
    auto *existing = reinterpret_cast<DeleteProjectConfirmModal *>(storage_);
    existing->~DeleteProjectConfirmModal();
    inUse_ = false;
  }
  inUse_ = true;
  return new (storage_) DeleteProjectConfirmModal(view, name);
}

DeleteProjectConfirmModal::DeleteProjectConfirmModal(View &view, const char *projectName)
    : ModalView(view), projectLine_("Delete \"") {
  projectLine_.append(projectName);
  projectLine_.append("\"");
}

DeleteProjectConfirmModal::~DeleteProjectConfirmModal() {
}

void DeleteProjectConfirmModal::Destroy() {
  this->~DeleteProjectConfirmModal();
  inUse_ = false;
}

void DeleteProjectConfirmModal::UpdateProgress_() {
  const bool comboHeld = (currentMask_ & DELETE_HOLDmask_) == DELETE_HOLDmask_;
  const unsigned long now = System::GetInstance()->GetClock();

  if (!comboHeld) {
    if (holdingCombo_ || (holdProgressMs_ != 0)) {
      holdingCombo_ = false;
      holdProgressMs_ = 0;
      isDirty_ = true;
      static_cast<AppWindow &>(w_).SetDirty();
    }
    return;
  }

  if (!holdingCombo_) {
    holdingCombo_ = true;
    holdStartMs_ = now;
    holdProgressMs_ = 0;
    isDirty_ = true;
    static_cast<AppWindow &>(w_).SetDirty();
    return;
  }

  unsigned long elapsed = now - holdStartMs_;
  if (elapsed > DELETE_HOLD_DURATION_MS) {
    elapsed = DELETE_HOLD_DURATION_MS;
  }

  if (holdProgressMs_ != elapsed) {
    holdProgressMs_ = elapsed;
    isDirty_ = true;
    static_cast<AppWindow &>(w_).SetDirty();
  }

  if (holdProgressMs_ >= DELETE_HOLD_DURATION_MS) {
    EndModal(MBL_YES);
  }
}

void DeleteProjectConfirmModal::AnimationUpdate() {
  UpdateProgress_();
}

void DeleteProjectConfirmModal::ButtonDown(uint16_t newButton, uint16_t mask) {
  currentMask_ = mask | newButton;

  if (newButton == BM_ENTER) {
    // cancel is confirmed on release (see ButtonUp)
    enterDown_ = true;
    isDirty_ = true;
    return;
  }

  UpdateProgress_();
}

void DeleteProjectConfirmModal::ButtonUp(uint16_t button, uint16_t mask) {
  currentMask_ = mask;

  // Only if the press happened inside this dialog, so the release of whatever
  // opened it doesn't cancel it right away.
  if (button == BM_ENTER && enterDown_) {
    enterDown_ = false;
    EndModal(MBL_CANCEL);
    return;
  }

  UpdateProgress_();
}

void DeleteProjectConfirmModal::DrawView() {
  // same window style as MessageBox: title bar, content starts below it (y = 3)
  const int contentWidth = 26;
  const int width = 4 + contentWidth;
  const int height = 11;

  left_ = (SCREEN_WIDTH - width) / 2;
  top_ = (SCREEN_HEIGHT - height) / 2;
  DrawWindow(0, 0, width, height, "Delete project");

  SetColor(Theme::Dialog::fg);

  const int projectLineX = 2 + std::max(0, (contentWidth - (int)projectLine_.size()) / 2);
  DrawString(projectLineX, 3, projectLine_.c_str());
  DrawString(2, 4, "Press & hold ALT+PLAY+EDIT");

  if (holdingCombo_ || holdProgressMs_ > 0) {
    progressBar_t progressBar;
    fillProgressBar(holdProgressMs_, DELETE_HOLD_DURATION_MS, &progressBar);
    DrawString(2 + (contentWidth - 12) / 2, 6, progressBar);
  }

  const char *cancelButton = "Cancel";
  const int buttonY = 8;
  SetColor(Theme::Dialog::Button::fg(true));
  SetBackgroundColor(Theme::Dialog::Button::bg(true));
  int len = strlen(cancelButton);
  int x = 2 + (contentWidth - len) / 2;
  DrawString(x, buttonY, cancelButton);

  // button ends
  SetBackgroundColor(Theme::Dialog::bg);
  SetColor(Theme::Dialog::Button::bg(true));
  DrawChar(x - 1, buttonY, char_button_left(enterDown_));
  DrawChar(x + len, buttonY, char_button_right(enterDown_));

  focusRect_ = GUIRect(x - 1 + left_, buttonY + top_, len + 2, 1);
}
