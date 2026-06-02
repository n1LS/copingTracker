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

void NullView::ProcessButtonMask(unsigned short mask, bool pressed) {
}

void NullView::DrawView() {

  Clear();

  SetColor(cccccHighlight2);

  char buildString[SCREEN_WIDTH + 1];
  // todo: update and merge with instance from appwindow
  npf_snprintf(buildString, sizeof(buildString), "picoTracker build %s%s_%s", PROJECT_NUMBER, PROJECT_RELEASE,
               BUILD_COUNT);
  GUIPoint pos;
  pos.y_ = 22;
  pos.x_ = (32 - strlen(buildString)) / 2;
  DrawString(pos.x_, pos.y_, buildString);
}

void NullView::OnPlayerUpdate(PlayerEventType, unsigned int tick) {
}

void NullView::OnFocus() {};
