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

#include "ScreenView.h"
#include <Application/AppWindow.h>
#include <cstring>
#include <nanoprintf.h>

ScreenView::ScreenView(GUIWindow &w, ViewData *viewData) : View(w, viewData) {
}

ScreenView::~ScreenView() {
}

/// Updates the animation by redrawing the battery gauge and power button UI on
/// every clock tick
void ScreenView::AnimationUpdate() {
  drawBattery();
  drawPlaybackIndicator();
}

void ScreenView::Navigate(ViewType target) {
  ViewEvent ve(VET_SWITCH_VIEW, &target);
  SetChanged();
  NotifyObservers(&ve);
}

const char *ScreenView::emptyStateMessage() const {
  return "No items to show";
}

void ScreenView::drawEmptyState() {
  const char *msg = emptyStateMessage();
  int len = (int)strlen(msg);
  int x = (SCREEN_WIDTH - len) / 2;
  int y = SCREEN_HEIGHT / 2;
  SetColor(Theme::View::fg);
  SetBackgroundColor(Theme::View::bg);
  DrawString(x, y, msg);
}