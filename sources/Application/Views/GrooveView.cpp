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

void GrooveView::ProcessButtonMask(unsigned short mask, bool pressed) {

  if (!pressed)
    return;

  Player *player = Player::GetInstance();

  if (mask & EPBM_EDIT) {
    if (mask & EPBM_LEFT) {
      warpGroove(-1);
    }
    if (mask & EPBM_RIGHT) {
      warpGroove(1);
    }
    if (mask & EPBM_DOWN) {
      warpGroove(-0x10);
    }
    if (mask & EPBM_UP) {
      warpGroove(0x10);
    }
    if (mask & EPBM_ENTER) {
      clearCursorValue();
    };
    if (mask & EPBM_PLAY) {
      // recording screen
      if (!Player::GetInstance()->IsRunning()) {
        switchToRecordView();
      }
    }
  } else if (mask & EPBM_ENTER) {
    // ENTER modifier
    if (mask & EPBM_LEFT) {
      updateCursorValue(-1);
    }
    if (mask & EPBM_RIGHT) {
      updateCursorValue(1);
    }
    if (mask & EPBM_DOWN) {
      updateCursorValue(-1, true);
    }
    if (mask & EPBM_UP) {
      updateCursorValue(1, true);
    }
    if (mask == EPBM_ENTER) {
      initCursorValue();
    };
  } else if (mask & EPBM_NAV) {
    // NAV Modifier
    if (mask & EPBM_DOWN) {
      ViewType vt = VT_PHRASE;
      ViewEvent ve(VET_SWITCH_VIEW, &vt);
      SetChanged();
      NotifyObservers(&ve);
    }
    if (mask & EPBM_PLAY) {
      player->OnStartButton(PM_PHRASE, viewData_->songX_, true, viewData_->chainRow_);
    }
  } else {
    // No modifier
    if (mask & EPBM_DOWN)
      updateCursor(1);
    if (mask & EPBM_UP)
      updateCursor(-1);
    if (mask & EPBM_PLAY) {
      player->OnStartButton(PM_PHRASE, viewData_->songX_, false, viewData_->chainRow_);
    }
  }
}

void GrooveView::DrawView() {

  Clear();

  GUIPoint pos = GetTitlePosition();

  // Draw title
  char title[SCREEN_WIDTH + 1];

  SetColor(cNormal);

  npf_snprintf(title, sizeof(title), "Groove: %2.2X", viewData_->currentGroove_);
  DrawString(pos.x_, pos.y_, title);

  // Compute song grid location

  GUIPoint anchor = GetAnchor();

  // Display row numbers
  SetColor(cHighlight1);
  char buffer[6];
  pos = anchor;
  pos.x_ -= 3;
  for (int j = 0; j < 16; j++) {
    ((j / ALT_ROW_NUMBER) % 2) ? SetColor(cAccent) : SetColor(cAccentAlt);
    hex2char(j, buffer);
    DrawString(pos.x_, pos.y_, buffer);
    pos.y_++;
  }

  // Display current groove
  pos = anchor;
  SetColor(cNormal);

  unsigned char *grooveData = Groove::GetInstance()->GetGrooveData(viewData_->currentGroove_);
  for (int j = 0; j < 16; j++) {
    if (grooveData[j] != NO_GROOVE_DATA) {
      hex2char(grooveData[j], buffer);
      buffer[3] = 0;
    } else {
      strcpy(buffer, "--");
    };
    DrawString(pos.x_, pos.y_, buffer);
    pos.y_++;
  }

  drawMap();
  drawNotes();
}

void GrooveView::OnPlayerUpdate(PlayerEventType, unsigned int tick) {

  GUIPoint anchor = GetAnchor();
  GUIPoint pos;

  SetBackgroundColor(cBackground);
  
  pos.x_ = anchor.x_ - 1;
  pos.y_ = anchor.y_ + lastPosition_;
  DrawString(pos.x_, pos.y_, " ");

  Groove *gr = Groove::GetInstance();
  // Get current channel
  int channel = viewData_->songX_;

  int groove;
  int groovepos;

  gr->GetChannelData(channel, &groove, &groovepos);

  if (groove == viewData_->currentGroove_ && viewData_->playMode_ != PM_AUDITION) {
    lastPosition_ = groovepos;
    pos.x_ = anchor.x_ - 1;
    pos.y_ = anchor.y_ + lastPosition_;
    SetColor(cAccent);
    SetBackgroundColor(cBackground);
    DrawString(pos.x_, pos.y_, ">");
  };

  drawNotes();
}

void GrooveView::OnFocus() {};
