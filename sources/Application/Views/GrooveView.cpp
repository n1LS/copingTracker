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

#include "GrooveView.h"
#include "Application/Model/Groove.h"
#include "Application/Utils/char.h"
#include "Application/Views/SampleEditorView.h"
#include "ViewData.h"
#include <Application/AppWindow.h>
#include <nanoprintf.h>

GrooveView::GrooveView(GUIWindow &w, ViewData *viewData) : ScreenView(w, viewData) {
  position_ = 0;
  lastPosition_ = 0;
}

GrooveView::~GrooveView() {
}

void GrooveView::Reset() {
  position_ = 0;
  lastPosition_ = 0;
}

void GrooveView::updateCursor(int dir) {
  position_ += dir;
  if (position_ < 0)
    position_ += 16;
  if (position_ > 15)
    position_ -= 16;
  isDirty_ = true;
}

void GrooveView::updateCursorValue(int val, bool sync) {
  unsigned char *grooveData = Groove::GetInstance()->GetGrooveData(viewData_->currentGroove_);
  int value = grooveData[position_];
  val += value;
  if (val < 1)
    val = 1;
  if (val > 0xF)
    val = 0xF;
  grooveData[position_] = val;
  isDirty_ = true;
}

void GrooveView::warpGroove(int dir) {
  int current = viewData_->currentGroove_;
  current += dir;
  if (current >= MAX_GROOVES) {
    current -= MAX_GROOVES;
  };
  if (current < 0) {
    current += MAX_GROOVES;
  };
  viewData_->currentGroove_ = current;
  isDirty_ = true;
}

void GrooveView::initCursorValue() {
  unsigned char *grooveData = Groove::GetInstance()->GetGrooveData(viewData_->currentGroove_);
  if (grooveData[position_] == NO_GROOVE_DATA) {
    grooveData[position_] = 1;
  };
  isDirty_ = true;
}

void GrooveView::clearCursorValue() {
  unsigned char *grooveData = Groove::GetInstance()->GetGrooveData(viewData_->currentGroove_);
  grooveData[position_] = NO_GROOVE_DATA;
  isDirty_ = true;
}

void GrooveView::ProcessButtonMask(uint16_t mask, bool pressed) {

  if (!pressed)
    return;

  Player *player = Player::GetInstance();

  if (mask & BM_EDIT) {
    if (mask & BM_LEFT) {
      warpGroove(-1);
    }
    if (mask & BM_RIGHT) {
      warpGroove(1);
    }
    if (mask & BM_DOWN) {
      warpGroove(-0x10);
    }
    if (mask & BM_UP) {
      warpGroove(0x10);
    }
    if (mask & BM_ENTER) {
      clearCursorValue();
    };
  } else if (mask & BM_ENTER) {
    // ENTER modifier
    if (mask & BM_LEFT) {
      updateCursorValue(-1);
    }
    if (mask & BM_RIGHT) {
      updateCursorValue(1);
    }
    if (mask & BM_DOWN) {
      updateCursorValue(-1, true);
    }
    if (mask & BM_UP) {
      updateCursorValue(1, true);
    }
    if (mask == BM_ENTER) {
      initCursorValue();
    };
  } else if (mask & BM_NAV) {
    // NAV Modifier
    if (mask & BM_DOWN) {
      Navigate(VT_PHRASE);
    } else if (mask & BM_LEFT) {
      Navigate(VT_PROJECT);
    } else if (mask & BM_PLAY) {
      player->OnStartButton(PM_PHRASE, viewData_->songX_, true, viewData_->chainRow_);
    }
  } else {
    // No modifier
    if (mask & BM_DOWN)
      updateCursor(1);
    if (mask & BM_UP)
      updateCursor(-1);
    if (mask & BM_PLAY) {
      player->OnStartButton(PM_PHRASE, viewData_->songX_, false, viewData_->chainRow_);
    }
  }
}

void GrooveView::DrawView() {
  Clear();

  // Draw title

  DrawTitle("Groove: %2.2X", viewData_->currentGroove_);

  // Compute song grid location

  GUIPoint anchor = GetAnchor();
  GUIPoint pos = anchor;

  // Display row numbers
  pos = anchor;
  drawRowNumbers(pos.x_ - 3, pos.y_, 0, 16);

  // Display current groove
  pos = anchor;
  SetColor(Theme::View::fg);

  char buffer[6];

  unsigned char *grooveData = Groove::GetInstance()->GetGrooveData(viewData_->currentGroove_);
  for (int j = 0; j < STEPS_PER_GROOVE; j++) {
    bool highlighted = (j == position_);

    if (grooveData[j] != NO_GROOVE_DATA) {
      hex2char(grooveData[j], buffer);
      buffer[3] = 0;
    } else {
      strcpy(buffer, "--");
    }

    // Valid pair: both empty, or both set and summing to 12
    int pairBase = j & ~1;
    bool bothEmpty = grooveData[pairBase] == NO_GROOVE_DATA && grooveData[pairBase + 1] == NO_GROOVE_DATA;
    bool bothValidSum = grooveData[pairBase] != NO_GROOVE_DATA && grooveData[pairBase + 1] != NO_GROOVE_DATA &&
                        (grooveData[pairBase] + grooveData[pairBase + 1]) == 12;
    bool pairInvalid = !bothEmpty && !bothValidSum;

    SetBackgroundColor(Theme::View::bg);
    SetColor(pairInvalid ? Theme::View::warning : Theme::View::fg);

    if (highlighted) {
      SwapColors();
    }

    DrawString(pos.x_, pos.y_, buffer);
    pos.y_++;
  }

  drawMap();
  drawNotes();
}

void GrooveView::OnPlayerUpdate(PlayerEventType, unsigned int tick) {
  GUIPoint anchor = GetAnchor();
  GUIPoint pos;

  SetBackgroundColor(Theme::View::bg);

  pos.x_ = anchor.x_ - 1;
  pos.y_ = anchor.y_ + lastPosition_;
  DrawChar(pos.x_, pos.y_, ' ');

  Groove *gr = Groove::GetInstance();
  
  // Get current channel
  int channel = viewData_->songX_;

  int groove;
  int groovepos;

  gr->GetChannelData(channel, &groove, &groovepos);

  // draw indicator
  if (groove == viewData_->currentGroove_ && viewData_->playMode_ != PM_AUDITION) {
    lastPosition_ = groovepos;
    pos.x_ = anchor.x_ - 1;
    pos.y_ = anchor.y_ + lastPosition_;
    SetColor(Theme::Song::Playback::live);
    SetBackgroundColor(Theme::View::bg);
    DrawString(pos.x_, pos.y_, char_indicator_position_s);
  };

  drawNotes();
}

void GrooveView::OnFocus() {};
