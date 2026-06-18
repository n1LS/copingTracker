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

#include "ModalView.h"
#include "Application/AppWindow.h"

// TODO remove
#include "System/System/System.h"

uint32_t ModalView::nextInstanceId_ = 0;

ModalView::ModalView(View &v)
    : View(v.w_, v.viewData_), instanceId_(++nextInstanceId_), finished_(false), returnCode_(0) {
}

ModalView::~ModalView() {
}

int ModalView::GetReturnCode() {
  return returnCode_;
}

uint32_t ModalView::GetInstanceId() const {
  return instanceId_;
}

bool ModalView::IsFinished() {
  return finished_;
}

void ModalView::EndModal(int returnCode) {
  returnCode_ = returnCode;
  finished_ = true;
}

void ModalView::Destroy() {
  delete this;
}

void ModalView::ClearTextRect(int x, int y, int w, int h) {
  // For ModalView, handle both offset-based (from SetWindow) and absolute coords.
  // If x, y are already in screen space (from SetWindow's direct call), don't offset.
  // Otherwise, offset by left_/top_ (for modal-relative calls).
  // SetWindow now calls ClearTextRect with global coords, so we use them directly.
  View::ClearTextRect(x, y, w, h);
}

// DrawString override to account for modal window position
void ModalView::DrawString(int x, int y, const char *text) {
  View::DrawString(x + left_, y + top_, text);
}

// DrawChar override to account for modal window position
void ModalView::DrawChar(int x, int y, char c, bool transparent) {
  View::DrawChar(x + left_, y + top_, c, transparent);
}

GUIPoint ModalView::GetAnchor() {
  // Get the base anchor point from View
  GUIPoint baseAnchor = View::GetAnchor();

  // Adjust for modal window position
  baseAnchor.x_ = left_;
  baseAnchor.y_ = top_;

  return baseAnchor;
}

void ModalView::SetWindow(int width, int height) {
  width = std::min(width, SCREEN_WIDTH - 4);
  height = std::min(height, SCREEN_HEIGHT - 4);

  left_ = (SCREEN_WIDTH - width) / 2;
  top_ = (SCREEN_HEIGHT - height) / 2;

  // Clear the entire modal area including the border frame using absolute screen coordinates.
  ClearTextRect(left_ - 2, top_ - 2, width + 4, height + 4);

  SetBackgroundColor(Theme::Dialog::bg);
  SetColor(Theme::Dialog::border);
  DrawFilledBorder(-2, -2, width + 4, height + 4, RED, System::GetInstance()->GetRandomNumber() % 2);
}
