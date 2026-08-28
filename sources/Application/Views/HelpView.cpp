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

#include "HelpView.h"
#include "Foundation/Constants/Documentation.generated.h"
#include <Application/AppWindow.h>
#include <nanoprintf.h>

static const int tabCount = 5;
static const int pageSize = 20;

HelpView::HelpView(GUIWindow &w, ViewData *viewData) : ScreenView(w, viewData) {
}

HelpView::~HelpView() {
}

void HelpView::ProcessButtonMask(uint16_t mask, bool pressed) {
  bool navDown = (mask & BM_NAV);
  if (navDown != navDown_) {
    navDown_ = navDown;
    SetDirty(true);
  }

  if (!pressed) {
    return;
  }

  if (mask & BM_NAV) {
    if (mask & BM_LEFT) {
      Navigate(VT_DEVICE, vtRevealFromLeft);
    }
  } else {
    if (mask & (BM_UP | BM_DOWN)) {
      // scrolling
      int delta = (mask & BM_UP) ? -1 : 1;
      delta *= (mask & BM_ALT) ? pageSize : 1;
      scrollBy(delta);
      return;
    }

    // no modifiers
    if (mask == BM_LEFT) {
      setTab(selectedTab_ - 1);
    } else if (mask == BM_RIGHT) {
      setTab(selectedTab_ + 1);
    }
  }
}

void HelpView::DrawView() {
  Clear();
  DrawTitle(char_back_s " Help!");

  drawTabs();
  drawContent();
  drawScrollBar(SCREEN_WIDTH - 1, 2, pageSize, offset_, numLines_);

  if (navDown_) {
    ClearTextRect(0, SCREEN_HEIGHT - 5, 3, 5);
    ClearTextRect(3, SCREEN_HEIGHT - 4, 1, 4);
    ClearTextRect(4, SCREEN_HEIGHT - 3, 1, 3);
    drawMap();
  }
}

void HelpView::drawContent() {
  unsigned int ptr = offset_ * 2 * 30;

  int x = 0;
  int y = 2;

  Color fg = WHITE;
  Color bg = BLACK;

  for (int n = 0; n < 30 * pageSize; n++) {
    uint8_t color = data_[ptr++];
    uint8_t ch = data_[ptr++];
    SetColor(color >> 4);
    SetBackgroundColor(color & 0xF);
    DrawChar(x, y, ch);

    x++;

    if (x > 29) {
      x = 0;
      y += 1;

      if (ptr >= dataSize_) {
        break;
      }
    }
  }
}

void HelpView::drawTabs() {
  int x = tabOffset_;

  for (int i = 0; i < tabCount; i++) {
    x += DrawTab(x, SCREEN_HEIGHT - 1, documentation[i].title, i == selectedTab_);
  }

  SetColor(Theme::View::fg);
  if (x >= SCREEN_WIDTH || tabOffset_ < 0) {
    DrawString(SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1,
               tabOffset_ < 0 ? char_indicator_leftRight_s : char_indicator_rightNoLeft_s);
  }
}

void HelpView::OnFocus() {
  setTab(0);
}

void HelpView::setTab(int tab) {
  tab = std::max(0, std::min(tab, tabCount - 1));
  selectedTab_ = tab;

  const DocumentationPage *page = &documentation[tab];

  // Calculate the x-position of the selected tab
  int tabX = tabOffset_;
  for (int i = 0; i < tab; i++) {
    tabX += strlen(documentation[i].title) + 2;
  }

  // Calculate the width of the selected tab (title + 2 chars)
  int tabWidth = strlen(page->title) + 2;

  // If it's the last tab, it can go to the screen edge, otherwise, it must end 1 character before the edge (<>)
  int maxRight = (tab == tabCount - 1) ? SCREEN_WIDTH - 1 : SCREEN_WIDTH - 2;

  // If the tab extends beyond the right edge, shift left
  int rightEdge = tabX + tabWidth;
  if (rightEdge > maxRight) {
    tabOffset_ -= (rightEdge - maxRight);
  }

  // If the tab is scrolled off the left edge, shift right
  if (tabX < 0) {
    tabOffset_ += -tabX;
  }

  data_ = page->data;
  dataSize_ = page->size;
  numLines_ = page->size / 60;

  offset_ = 0;

  SetDirty(true);
}

void HelpView::scrollBy(int delta) {
  offset_ = std::max(0, std::min(offset_ + delta, numLines_ - pageSize));
  SetDirty(true);
}