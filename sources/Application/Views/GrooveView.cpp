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
#include "Application/Utils/ContextLegend.generated.h"
#include "Application/Utils/char.h"
#include "Application/Views/SampleEditorView.h"
#include "ViewData.h"
#include <Application/AppWindow.h>
#include <algorithm>
#include <nanoprintf.h>

GrooveView::GrooveView(GUIWindow &w, ViewData *viewData) : ScreenView(w, viewData) {
  position_ = 0;
  lastPosition_ = 0;
  clipboard_.active_ = false;
  clipboard_.row_ = 0;
  clipboard_.height_ = 0;
  saveRow_ = 0;
}

GrooveView::~GrooveView() {
}

void GrooveView::Reset() {
  position_ = 0;
  lastPosition_ = 0;
  clipboard_.active_ = false;
  clipboard_.row_ = 0;
  clipboard_.height_ = 0;
  saveRow_ = 0;
}

void GrooveView::updateCursor(int dir) {
  position_ += dir;
  if (clipboard_.active_) {
    // a selection stops at the ends
    position_ = std::clamp(position_, 0, 15);
  } else {
    if (position_ < 0)
      position_ += 16;
    if (position_ > 15)
      position_ -= 16;
  }
  updateFocusRect();
  isDirty_ = true;
}

void GrooveView::updateFocusRect() {
  // the value of a step is 2 characters, the focus is the cell of the cursor or the selection
  const GUIPoint anchor = GetAnchor();
  int first = position_;
  int last = position_;
  if (clipboard_.active_) {
    selectionRows(first, last);
  }
  focusRect_ = GUIRect(anchor.x_, anchor.y_ + first, 2, last - first + 1);
}

void GrooveView::updateCursorValue(int val, bool sync) {
  const unsigned char *current = Groove::GetInstance()->GetGrooveData(viewData_->currentGroove_);
  int value = current[position_];
  val += value;
  if (val < 1)
    val = 1;
  if (val > 0xF)
    val = 0xF;
  if (val != value) {
    Groove::GetInstance()->EditGrooveData(viewData_->currentGroove_)[position_] = val;
  }
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
  const unsigned char *grooveData = Groove::GetInstance()->GetGrooveData(viewData_->currentGroove_);
  if (grooveData[position_] == NO_GROOVE_DATA) {
    Groove::GetInstance()->EditGrooveData(viewData_->currentGroove_)[position_] = 1;
  };
  isDirty_ = true;
}

void GrooveView::clearCursorValue() {
  const unsigned char *grooveData = Groove::GetInstance()->GetGrooveData(viewData_->currentGroove_);
  if (grooveData[position_] != NO_GROOVE_DATA) {
    Groove::GetInstance()->EditGrooveData(viewData_->currentGroove_)[position_] = NO_GROOVE_DATA;
  }
  isDirty_ = true;
}

bool GrooveView::navigationDown(uint16_t direction) {
  switch (direction) {
    case BM_DOWN:
      NavigateToPhrase(vtRevealFromBottom);
      return true;
    case BM_LEFT:
      Navigate(VT_PROJECT, vtRevealFromLeft);
      return true;
  }
  return false;
}

void GrooveView::beginSelection() {
  if (!clipboard_.active_) {
    clipboard_.active_ = true;
    clipboard_.row_ = position_;
    saveRow_ = position_;
  }
}

void GrooveView::selectionRows(int &first, int &last) const {
  first = std::min<int>(clipboard_.row_, position_);
  last = std::max<int>(clipboard_.row_, position_);
}

void GrooveView::ButtonDown(uint16_t newButton, uint16_t mask) {
  // the menu that ALT + EDIT opened: any other button starts the selection
  if (viewMode_ == VM_CLONE) {
    viewMode_ = VM_SELECTION;
  }

  if (viewMode_ == VM_SELECTION) {
    beginSelection();
    processSelectionButtonMask(newButton, mask);
  } else {
    viewMode_ = VM_NORMAL;
    processNormalButtonMask(newButton, mask);
  }
}

void GrooveView::ButtonUp(uint16_t button, uint16_t mask) {
  // there is nothing to do in the menu without ALT: the selection starts here instead of with the next button (when
  // the menu was used for undo / redo it just ends)
  if (viewMode_ == VM_CLONE && button == BM_ALT) {
    if (menuUsedUndo_) {
      viewMode_ = VM_NORMAL;
    } else {
      viewMode_ = VM_SELECTION;
      beginSelection();
    }
    updateFocusRect();
    isDirty_ = true;
  }
}

// the menu that ALT + EDIT opened stays open while ALT is held
void GrooveView::AfterUndoRedo() {
  View::AfterUndoRedo();
  if (menuOpen_) {
    viewMode_ = VM_CLONE;
  }
}

void GrooveView::processNormalButtonMask(uint16_t newButton, uint16_t mask) {

  Player *player = Player::GetInstance();

  if (IsPair(newButton, mask, BM_EDIT, BM_ALT)) {
    // the menu: undo / redo, and any other button starts the selection
    viewMode_ = VM_CLONE;
  } else if (IsChord(newButton, mask, BM_ENTER, BM_ALT)) {
    pasteClipboard();
  } else if (mask == BM_EDIT) {
    switch (newButton) {
      case BM_LEFT:
        warpGroove(-1);
        break;
      case BM_RIGHT:
        warpGroove(1);
        break;
      case BM_DOWN:
        warpGroove(-0x10);
        break;
      case BM_UP:
        warpGroove(0x10);
        break;
      case BM_ENTER:
        cutPosition();
        break;
    }
  } else if (IsPair(newButton, mask, BM_EDIT, BM_ENTER)) {
    cutPosition();
  } else if (mask == BM_ENTER) {
    // ENTER modifier
    switch (newButton) {
      case BM_LEFT:
        updateCursorValue(-1);
        break;
      case BM_RIGHT:
        updateCursorValue(1);
        break;
      case BM_DOWN:
        updateCursorValue(-1, true);
        break;
      case BM_UP:
        updateCursorValue(1, true);
        break;
    }
  } else if (IsChord(newButton, mask, BM_ENTER)) {
    initCursorValue();
  } else if (mask == BM_NAV) {
    // NAV Modifier
    switch (newButton) {
      case BM_PLAY:
        player->OnStartButton(PM_PHRASE, viewData_->songX_, true, viewData_->chainRow_);
        break;
    }
  } else if (mask == 0) {
    // No modifier
    switch (newButton) {
      case BM_DOWN:
        updateCursor(1);
        break;
      case BM_UP:
        updateCursor(-1);
        break;
      case BM_PLAY:
        player->OnStartButton(PM_PHRASE, viewData_->songX_, false, viewData_->chainRow_);
        break;
    }
  }
}

void GrooveView::processSelectionButtonMask(uint16_t newButton, uint16_t mask) {

  Player *player = Player::GetInstance();

  if (IsChord(newButton, mask, BM_EDIT)) {
    copySelection();
  } else if (IsPair(newButton, mask, BM_EDIT, BM_ALT)) {
    extendSelection();
  } else if (IsChord(newButton, mask, BM_ENTER, BM_ALT)) {
    cutSelection();
  } else if ((newButton == BM_UP || newButton == BM_DOWN) && mask == (BM_ENTER | BM_EDIT)) {
    // ENTER + EDIT + UP / DOWN nudge the selection
    nudgeSelection(newButton == BM_DOWN ? +1 : -1);
  } else if (mask == BM_NAV) {
    if (newButton == BM_PLAY) {
      player->OnStartButton(PM_PHRASE, viewData_->songX_, true, viewData_->chainRow_);
    }
  } else if (mask == 0) {
    // No modifier
    switch (newButton) {
      case BM_DOWN:
        updateCursor(1);
        break;
      case BM_UP:
        updateCursor(-1);
        break;
      case BM_PLAY:
        player->OnStartButton(PM_PHRASE, viewData_->songX_, false, viewData_->chainRow_);
        break;
    }
  }
}

// the selection is over: the cursor goes back to where it started
void GrooveView::fillClipboardData() {
  int first, last;
  selectionRows(first, last);
  clipboard_.row_ = first;
  clipboard_.height_ = last - first + 1;
  const unsigned char *data = Groove::GetInstance()->GetGrooveData(viewData_->currentGroove_);
  for (int i = 0; i < clipboard_.height_; i++) {
    clipboard_.data_[i] = data[first + i];
  }
}

void GrooveView::copySelection() {
  fillClipboardData();
  clipboard_.active_ = false;
  viewMode_ = VM_NORMAL;
  position_ = saveRow_;
  updateFocusRect();
  isDirty_ = true;
}

void GrooveView::cutSelection() {
  fillClipboardData();

  // clear the rows, what is not set already is not recorded
  const Groove *groove = Groove::GetInstance();
  const unsigned char *data = groove->GetGrooveData(viewData_->currentGroove_);
  for (int i = 0; i < clipboard_.height_; i++) {
    if (data[clipboard_.row_ + i] != NO_GROOVE_DATA) {
      unsigned char *edit = Groove::GetInstance()->EditGrooveData(viewData_->currentGroove_);
      for (int j = i; j < clipboard_.height_; j++) {
        edit[clipboard_.row_ + j] = NO_GROOVE_DATA;
      }
      break;
    }
  }

  clipboard_.active_ = false;
  viewMode_ = VM_NORMAL;
  position_ = saveRow_;
  updateFocusRect();
  isDirty_ = true;
}

// ENTER + EDIT: the value at the cursor is copied and cleared
void GrooveView::cutPosition() {
  clipboard_.row_ = position_;
  clipboard_.height_ = 1;
  clipboard_.data_[0] = Groove::GetInstance()->GetGrooveData(viewData_->currentGroove_)[position_];
  if (clipboard_.data_[0] != NO_GROOVE_DATA) {
    Groove::GetInstance()->EditGrooveData(viewData_->currentGroove_)[position_] = NO_GROOVE_DATA;
  }
  isDirty_ = true;
}

void GrooveView::pasteClipboard() {
  if (clipboard_.height_ == 0) {
    return;
  }

  // the rows continue at the top after the last one, as in the table view
  const int height = clipboard_.height_;
  const unsigned char *data = Groove::GetInstance()->GetGrooveData(viewData_->currentGroove_);
  bool differs = false;
  for (int j = 0; j < height; j++) {
    differs = differs || data[(j + position_) % 16] != clipboard_.data_[j];
  }
  if (differs) {
    unsigned char *edit = Groove::GetInstance()->EditGrooveData(viewData_->currentGroove_);
    for (int j = 0; j < height; j++) {
      edit[(j + position_) % 16] = clipboard_.data_[j];
    }
  }
  updateCursor((position_ + height) % 16 - position_);
  isDirty_ = true;
}

void GrooveView::extendSelection() {
  if (position_ < clipboard_.row_) {
    position_ = 0;
    clipboard_.row_ = 15;
  } else {
    clipboard_.row_ = 0;
    position_ = 15;
  }
  updateFocusRect();
  isDirty_ = true;
}

void GrooveView::nudgeSelection(int delta) {
  int first, last;
  selectionRows(first, last);

  // the first / last row of the groove
  if (first + delta < 0 || last + delta >= 16) {
    return;
  }

  unsigned char *data = Groove::GetInstance()->EditGrooveData(viewData_->currentGroove_);
  const unsigned char jumped = data[delta > 0 ? last + 1 : first - 1];
  if (delta > 0) {
    for (int row = last; row >= first; --row) {
      data[row + 1] = data[row];
    }
    data[first] = jumped;
  } else {
    for (int row = first; row <= last; ++row) {
      data[row - 1] = data[row];
    }
    data[last] = jumped;
  }

  // the selection, the position it started at (the cursor goes back there after a copy) and the cursor move
  clipboard_.row_ += delta;
  saveRow_ += delta;
  position_ += delta;
  updateFocusRect();
  isDirty_ = true;
}

void GrooveView::DrawView() {
  Clear();

  // Draw title

  DrawTitle("Groove %2.2X", viewData_->currentGroove_);

  // Compute song grid location

  GUIPoint anchor = GetAnchor();
  GUIPoint pos = anchor;
  updateFocusRect();

  // Draw section header

  SetColor(Theme::View::inactive);
  SetBackgroundColor(Theme::View::bg);
  DrawString(pos.x_, pos.y_ - 1, "Grv");

  // Display row numbers
  pos = anchor;
  drawRowNumbers(pos.x_ - 3, pos.y_, 0, 16);

  // Display current groove
  pos = anchor;
  SetColor(Theme::View::fg);

  char buffer[6];

  const unsigned char *grooveData = Groove::GetInstance()->GetGrooveData(viewData_->currentGroove_);
  for (int j = 0; j < STEPS_PER_GROOVE; j++) {
    bool highlighted = (j == position_);
    if (clipboard_.active_) {
      int first, last;
      selectionRows(first, last);
      highlighted = j >= first && j <= last;
    }

    if (grooveData[j] != NO_GROOVE_DATA) {
      byteToHexString(grooveData[j], buffer);
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
  // the menu legend, or the notes
  if (!drawContextLegendInfoArea()) {
    infoAreaMode_ = InfoAreaDrawMode::Notes;
    drawNotes();
  }
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

  if (infoAreaMode_ == InfoAreaDrawMode::Notes) {
    drawNotes();
  }
}

ContextLegend GrooveView::currentContextLegend() const {
  if (viewMode_ == VM_CLONE) {
    return ContextLegend::GrooveMenu;
  }
  if (viewMode_ == VM_SELECTION || clipboard_.active_) {
    return ContextLegend::GrooveSelection;
  }
  // a single button that is down (not an arrow): the chords that can follow
  switch (heldButtons_) {
    case BM_ALT:
      return ContextLegend::GrooveChordsAlt;
    case BM_NAV:
      return ContextLegend::GrooveChordsNav;
    case BM_EDIT:
      return ContextLegend::GrooveChordsEdit;
    case BM_ENTER:
      return ContextLegend::GrooveChordsEnter;
    default:
      break;
  }
  return ContextLegend::None;
}

void GrooveView::OnFocus() {
  clipboard_.active_ = false;
  viewMode_ = VM_NORMAL;
  updateFocusRect();
}
