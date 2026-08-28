/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "ListView.h"
#include "Application/AppWindow.h"
#include "Application/Model/ThemeConstants.h"
#include "Foundation/Constants/SpecialCharacters.h"
#include "ViewUtils.h"
#include <cstdio>

ListView::ListView(GUIWindow &w, ViewData *viewData, DataSource *dataSource, Delegate *delegate, size_t pageSize,
                   int initialTab)
    : ScreenView(w, viewData), dataSource_(dataSource), delegate_(delegate), pageSize_(pageSize),
      selectedTab_(initialTab) {
}

ListView::~ListView() {
}

void ListView::OnFocus() {
}

void ListView::ProcessButtonMask(uint16_t mask, bool pressed) {
  if (!pressed) {
    return;
  }

  if (mask & BM_UP) {
    HandleUp(mask & BM_ALT);
  } else if (mask & BM_DOWN) {
    HandleDown(mask & BM_ALT);
  } else if (mask & BM_ENTER) {
    HandleEnter();
  } else if (mask & BM_EDIT) {
    HandleEdit();
  }
}

void ListView::DrawView() {
  size_t itemCount = dataSource_->GetItemCount();

  if (itemCount == 0) {
    DrawEmptyState();
    return;
  }

  DrawListItems();
  DrawScrollBar();
}

void ListView::DrawListItems() {
  int x = 1;
  int y = 2;

  EnsureVisible();

  size_t itemCount = dataSource_->GetItemCount();

  char buffer[LIST_VIEW_LINE_LENGTH + 1];

  for (size_t i = topIndex_; i < topIndex_ + pageSize_ && i < itemCount; i++) {
    bool isSelected = (i == currentIndex_);

    Color bg = Theme::View::Selection::bg(isSelected);
    Color fg = Theme::View::Selection::fg(isSelected);

    dataSource_->PrepareItemDrawing(i, isSelected, &fg, &bg, buffer, sizeof(buffer));
    SetColor(fg);
    SetBackgroundColor(bg);
    dataSource_->DrawItem(x, y, i, isSelected, fg, bg, buffer);

    if (isSelected) {
      SwapColors();
      DrawString(0, y, char_button_border_left_s);
      DrawString(SCREEN_WIDTH - 2, y, char_button_border_right_s);

      focusRect_ = GUIRect(0, y, SCREEN_WIDTH - 1, y);
    }

    y++;
  }
}

void ListView::DrawScrollBar() {
  size_t itemCount = dataSource_->GetItemCount();

  if (itemCount == 0) {
    return;
  }

  drawScrollBar(SCREEN_WIDTH - 1, 2, pageSize_, topIndex_, itemCount);
}

void ListView::DrawEmptyState() {
  drawEmptyState();
}

void ListView::HandleUp(bool page) {
  size_t itemCount = dataSource_->GetItemCount();

  if (itemCount == 0) {
    return;
  }

  if (page) {
    currentIndex_ = (currentIndex_ >= pageSize_) ? currentIndex_ - pageSize_ : 0;
  } else {
    currentIndex_ = (currentIndex_ > 0) ? currentIndex_ - 1 : itemCount - 1;
  }

  EnsureVisible();

  if (delegate_) {
    delegate_->OnItemNavigated(currentIndex_);
  }

  isDirty_ = true;
}

void ListView::HandleDown(bool page) {
  size_t itemCount = dataSource_->GetItemCount();

  if (itemCount == 0) {
    return;
  }

  if (page) {
    currentIndex_ = (currentIndex_ + pageSize_ < itemCount) ? currentIndex_ + pageSize_ : itemCount - 1;
  } else {
    currentIndex_ = (currentIndex_ + 1) % itemCount;
  }

  EnsureVisible();

  if (delegate_) {
    delegate_->OnItemNavigated(currentIndex_);
  }

  isDirty_ = true;
}

void ListView::HandleEnter() {
  if (delegate_) {
    delegate_->OnItemSelected(currentIndex_, selectedTab_);
  }
}

void ListView::HandleEdit() {
  if (delegate_) {
    delegate_->OnItemEdit(currentIndex_, selectedTab_);
  }
}

void ListView::EnsureVisible() {
  if (currentIndex_ < topIndex_) {
    topIndex_ = currentIndex_;
  } else if (currentIndex_ >= topIndex_ + pageSize_) {
    topIndex_ = currentIndex_ - pageSize_ + 1;
  }
}

void ListView::SetCurrentIndex(size_t index) {
  size_t itemCount = dataSource_->GetItemCount();

  if (itemCount > 0 && index < itemCount) {
    currentIndex_ = index;
    EnsureVisible();
    isDirty_ = true;
  }
}
