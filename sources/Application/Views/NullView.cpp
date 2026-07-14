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

#include "NullView.h"
#include <Application/AppWindow.h>
#include <nanoprintf.h>

NullView::NullView(GUIWindow &w, ViewData *viewData) : View(w, viewData) {
}

NullView::~NullView() {
}

void NullView::ProcessButtonMask(uint16_t mask, bool pressed) {
}

void NullView::DrawView() {
  Clear();

  SetBackgroundColor(Theme::View::bg);
  SetColor(Theme::View::inactive);

  DrawString((SCREEN_WIDTH - strlen(VERSION_STRING)) / 2, SCREEN_HEIGHT - 2, VERSION_STRING);
}

void NullView::OnPlayerUpdate(PlayerEventType, unsigned int tick) {
}

void NullView::OnFocus() {};
