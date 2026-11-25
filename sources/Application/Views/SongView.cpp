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

#include "SongView.h"
#include "Application/AppWindow.h"
#include "Application/Mixer/MixerService.h"
#include "Application/Player/Player.h"
#include "Application/Utils/ContextLegend.generated.h"
#include "Application/Utils/Core0Load.h"
#include "Application/Utils/char.h"
#include "Application/Views/BaseClasses/View.h"
#include "Application/Views/SampleEditorView.h"
#include "System/Console/Trace.h"
#include "System/System/System.h"
#include "UIController.h"
#include "ViewData.h"
#include <algorithm>
#include <cstring>
#include <nanoprintf.h>
#include <stdlib.h>
#include <string>

/******************************************************************************
 Constructor
 ******************************************************************************/

SongView::SongView(GUIWindow &w, ViewData *viewData) : ScreenView(w, viewData) {

  lastChain_ = 0;

  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
    this->lastPlayedPosition_[i] = 0;
    this->lastQueuedPosition_[i] = 0;
  }
  clipboard_.active_ = false;
  clipboard_.data_ = nullptr;
}

/******************************************************************************
 Destructor
******************************************************************************/

SongView::~SongView() {
}

/******************************************************************************
 Reset
******************************************************************************/

void SongView::Reset() {
  lastChain_ = 0;

  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
    lastPlayedPosition_[i] = 0;
    lastQueuedPosition_[i] = 0;
  }

  clipboard_.active_ = false;
  clipboard_.data_ = nullptr;
  clipboard_.x_ = 0;
  clipboard_.y_ = 0;
  clipboard_.offset_ = 0;
  clipboard_.width_ = 0;
  clipboard_.height_ = 0;
  memset(clipboard_.storage_, 0xFF, sizeof(clipboard_.storage_));

  saveX_ = 0;
  saveY_ = 0;
  saveOffset_ = 0;
  invertBatt_ = false;
  needClear_ = false;
  needsUIUpdate_ = false;
  needsPlayTimeUpdate_ = false;
}

/*******************************************************************************
 updateChain:
        update current chain value by adding offset
        parameter
******************************************************************************/

void SongView::updateChain(int offset) {
  unsigned char chain = viewData_->UpdateSongChain(offset);
  if (chain != 0xFF) {
    viewData_->song_->chain_.SetUsed(chain);
    lastChain_ = chain;
  }
  isDirty_ = true;
}

/*******************************************************************************
 updateChain:
        set current chain value to value parameter
******************************************************************************/

void SongView::setChain(unsigned char value) {
  viewData_->SetSongChain(value);
  lastChain_ = value;
  isDirty_ = true;
}

/*******************************************************************************
 updateSongOffset:
        modify top of the page row in song view by
        adding offset parameter
******************************************************************************/

void SongView::updateSongOffset(int offset) {
  viewData_->UpdateSongOffset(offset);
  isDirty_ = true;
}

/*******************************************************************************
 updateCursor:
        modify location of cursor in view by
        adding dx & dy parameters
******************************************************************************/

void SongView::updateCursor(int dx, int dy) {
  viewData_->UpdateSongCursor(dx, dy);
  isDirty_ = true;
}

/*******************************************************************************
 cutPosition:
        copy current position content to clipboard &
        erase current position value
******************************************************************************/

void SongView::cutPosition() {

  // prepare selection data
  clipboard_.x_ = viewData_->songX_;
  clipboard_.y_ = viewData_->songY_;
  clipboard_.offset_ = viewData_->songOffset_;

  saveX_ = viewData_->songX_;
  saveY_ = viewData_->songY_;
  saveOffset_ = viewData_->songOffset_;

  // cut selection
  cutSelection();
}

/*******************************************************************************
 pastePosition:
        set current position to last chain value if
        current step is empty
******************************************************************************/

void SongView::pasteLast() {

  // If we're on an empty spot, we past the last chain
  // otherwise we take the current chain as last

  unsigned char *c = viewData_->GetCurrentSongPointer();
  if (*c == 0xFF) {
    *c = lastChain_;
    viewData_->song_->chain_.SetUsed(*c);
    isDirty_ = true;
  } else {
    lastChain_ = *c;
  }
}

/*******************************************************************************
 clonePosition:
        slim clone current position
******************************************************************************/

void SongView::clonePosition() {

  unsigned char *pos = viewData_->GetCurrentSongPointer();
  unsigned char current = *pos;
  if (current == 255)
    return;

  uint16_t next = viewData_->song_->chain_.GetNext();
  if (next == NO_MORE_CHAIN)
    return;

  ChainStep *src = viewData_->song_->chain_.steps_[current];
  ChainStep *dst = viewData_->song_->chain_.steps_[next];

  for (int i = 0; i < 16; i++) {
    *dst++ = *src++;
  }
  setChain((unsigned char)next);
  isDirty_ = true;
}

// Moves a position (a row on the screen and the scroll offset of the screen) by rows. The screen only
// scrolls if the row would leave it, and the position stays valid.
static void shiftSongRow(int &y, int &offset, int delta) {
  const int row = y + offset + delta;
  const int rows = View::songRowCount_;

  if (row < offset) {
    offset = row;
  } else if (row > offset + rows - 1) {
    offset = row - (rows - 1);
  }
  offset = std::clamp(offset, 0, SONG_ROW_COUNT - rows);
  y = row - offset;
}

void SongView::nudgeSelection(int dx, int dy) {
  const GUIRect r = getSelectionRect();
  const int left = r.Left();
  const int top = r.Top();
  const int width = r.Width();
  const int height = r.Height();
  SongRow *rows = viewData_->song_->rows_;

  // the first / last row or channel
  if (left + dx < 0 || left + width - 1 + dx >= SONG_CHANNEL_COUNT || top + dy < 0 ||
      top + height - 1 + dy >= SONG_ROW_COUNT) {
    return;
  }

  if (dy != 0) {
    // the row in the way jumps to the row that was just emptied, in the columns of the selection
    unsigned char jumped[SONG_CHANNEL_COUNT];
    memcpy(jumped, rows[dy > 0 ? top + height : top - 1].chains + left, width);

    if (dy > 0) {
      for (int row = top + height - 1; row >= top; --row) {
        memcpy(rows[row + 1].chains + left, rows[row].chains + left, width);
      }
      memcpy(rows[top].chains + left, jumped, width);
    } else {
      for (int row = top; row < top + height; ++row) {
        memcpy(rows[row - 1].chains + left, rows[row].chains + left, width);
      }
      memcpy(rows[top + height - 1].chains + left, jumped, width);
    }
  }

  if (dx != 0) {
    // the same for the channel in the way, in the rows of the selection
    for (int row = top; row < top + height; ++row) {
      unsigned char *chains = rows[row].chains;
      if (dx > 0) {
        const unsigned char jumped = chains[left + width];
        memmove(chains + left + 1, chains + left, width);
        chains[left] = jumped;
      } else {
        const unsigned char jumped = chains[left - 1];
        memmove(chains + left - 1, chains + left, width);
        chains[left + width - 1] = jumped;
      }
    }
  }

  // the selection, the position it started at (the cursor goes back there after a copy) and the cursor move
  clipboard_.x_ += dx;
  saveX_ += dx;
  viewData_->songX_ += dx;
  if (dy != 0) {
    shiftSongRow(clipboard_.y_, clipboard_.offset_, dy);
    shiftSongRow(saveY_, saveOffset_, dy);
    shiftSongRow(viewData_->songY_, viewData_->songOffset_, dy);
  }
  isDirty_ = true;
}

void SongView::extendSelection() {
  GUIRect rect = getSelectionRect();
  if (rect.Left() > 0 || rect.Right() < SONG_CHANNEL_COUNT) {
    if (viewData_->songX_ < clipboard_.x_) {
      viewData_->songX_ = 0;
      clipboard_.x_ = 7;
    } else {
      viewData_->songX_ = 7;
      clipboard_.x_ = 0;
    }
    isDirty_ = true;
  } else {
    if (viewData_->songY_ < clipboard_.y_) {
      viewData_->songY_ = 0;
      clipboard_.y_ = 0x17;
    } else {
      clipboard_.y_ = 0;
      viewData_->songY_ = 0x17;
    }
    isDirty_ = true;
  }
}

/*******************************************************************************
 OnFocus:
        called when current view is becoming active
******************************************************************************/

void SongView::OnFocus() {
  clipboard_.active_ = false;
  titleScrollStart_ = AppWindow::GetAnimationFrameCounter();

  // eg. if the user was in master channel in mixerview and came to songview
  // we need to make sure we're not outside channel range
  if (viewData_->songX_ > SONG_CHANNEL_COUNT - 1) {
    viewData_->songX_ = 0; // default to channel 1
  }
}

// The title is hidden by the indicators from INDICATOR_LEFT_MOST on, the project name after the 5 character prefix
// ("Song " / "Live ") scrolls when it is longer than the rest: waits at the start, steps one character at a time,
// waits at the end, starts over. The animation frame counter is the time base.
static constexpr int titlePrefixLength = 5;
static constexpr int titleNameWidth = INDICATOR_LEFT_MOST - titlePrefixLength;
static constexpr uint32_t titleScrollStepFrames = etClock_HZ / 4; // ~4 characters per second
static constexpr uint32_t titleScrollPauseFrames = etClock_HZ * 3 / 2;

int SongView::titleScrollOffset() const {
  Variable *v = viewData_->project_->FindVariable(Token::VarProjectName);
  const int overflow = static_cast<int>(v->GetString().length()) - titleNameWidth;
  if (overflow <= 0) {
    return 0;
  }
  const uint32_t scrollFrames = overflow * titleScrollStepFrames;
  const uint32_t cycle = titleScrollPauseFrames + scrollFrames + titleScrollPauseFrames;
  const uint32_t t = (AppWindow::GetAnimationFrameCounter() - titleScrollStart_) % cycle;
  if (t < titleScrollPauseFrames) {
    return 0;
  }
  if (t < titleScrollPauseFrames + scrollFrames) {
    return (t - titleScrollPauseFrames) / titleScrollStepFrames;
  }
  return overflow;
}

// fullTitle: the whole title line (DrawView), otherwise only the part the indicators leave free (animation)
void SongView::drawTitleText(bool fullTitle) {
  Variable *v = viewData_->project_->FindVariable(Token::VarProjectName);
  const char *name = v->GetString().c_str();
  const int nameLength = static_cast<int>(strlen(name));
  const int offset = titleScrollOffset();
  titleScrollDrawn_ = offset;
  const char *prefix = Player::GetInstance()->GetSequencerMode() == SM_SONG ? "Song " : "Live ";

  char shown[SCREEN_WIDTH + 1];
  const int width = fullTitle ? INDICATOR_BATTERY_GAUGE_X : INDICATOR_LEFT_MOST;
  const int visible = std::max(0, std::min(titleNameWidth, nameLength - offset));
  npf_snprintf(shown, sizeof(shown), "%s", prefix);
  memcpy(shown + titlePrefixLength, name + std::min(offset, nameLength), visible);
  shown[titlePrefixLength + visible] = '\0';
  if (fullTitle) {
    DrawTitle("%s", shown);
    return;
  }
  // pad to the free width, the prefix is part of it
  size_t len = strlen(shown);
  memset(shown + len, ' ', width - len);
  shown[width] = '\0';
  SetBackgroundColor(Theme::View::Title::bg);
  SetColor(Theme::View::Title::fg);
  DrawString(0, 0, shown, fBold);
}

GUIRect SongView::getSelectionRect() {
  // anchor and cursor in either order: the origin is the top left most point
  const int anchorY = clipboard_.y_ + clipboard_.offset_;
  const int cursorY = viewData_->songY_ + viewData_->songOffset_;
  const int left = std::min<int>(clipboard_.x_, viewData_->songX_);
  const int right = std::max<int>(clipboard_.x_, viewData_->songX_);
  const int top = std::min(anchorY, cursorY);
  const int bottom = std::max(anchorY, cursorY);
  return GUIRect(left, top, right - left + 1, bottom - top + 1);
}

/*******************************************************************************
 fillClipboard:
        fill clipboard with current selection value
******************************************************************************/

void SongView::fillClipboardData() {

  // Clear current selection data

  clipboard_.data_ = clipboard_.storage_;

  // Prepare selection related information

  GUIRect selRect = getSelectionRect();

  // Set current selection  data

  // the selection rect already is inclusive (width/height are counts)
  clipboard_.width_ = selRect.Width();
  clipboard_.height_ = selRect.Height();

  unsigned char *src = viewData_->song_->rows_[selRect.Top()].chains + selRect.Left();
  unsigned char *dst = clipboard_.data_;

  for (int j = 0; j < clipboard_.height_; j++) {
    for (int i = 0; i < clipboard_.width_; i++) {
      *dst++ = *src++;
    }
    src += (SONG_CHANNEL_COUNT - clipboard_.width_);
  }
}

/*******************************************************************************
 copySelection:
        copy current selection to clipboard
******************************************************************************/

void SongView::copySelection() {

  fillClipboardData();
  clipboard_.active_ = false;
  viewMode_ = VM_NORMAL;
  viewData_->songX_ = saveX_;
  viewData_->songY_ = saveY_;
  viewData_->songOffset_ = saveOffset_;
  isDirty_ = true;
}

/*******************************************************************************
 cutSelection:
        cut current selection to clipboard
******************************************************************************/

void SongView::cutSelection() {

  // first copy the data to clipboard

  fillClipboardData();
  GUIRect selRect = getSelectionRect();

  // now move all rows up for cut

  unsigned char *dst = viewData_->song_->rows_[selRect.Top()].chains + selRect.Left();
  unsigned char *src = dst + SONG_CHANNEL_COUNT * clipboard_.height_;

  int rowCount = SONG_ROW_COUNT - selRect.Bottom();

  for (int j = 0; j < rowCount; j++) {

    for (int i = 0; i < clipboard_.width_; i++) {
      *dst++ = *src++;
    }
    src += (SONG_CHANNEL_COUNT - clipboard_.width_);
    dst += (SONG_CHANNEL_COUNT - clipboard_.width_);
  }

  for (int j = 0; j < clipboard_.height_; j++) {
    for (int i = 0; i < clipboard_.width_; i++) {
      *dst++ = 0xFF;
    }
    dst += (SONG_CHANNEL_COUNT - clipboard_.width_);
  };

  clipboard_.active_ = false;
  viewMode_ = VM_NORMAL;
  viewData_->songX_ = saveX_;
  viewData_->songY_ = saveY_;
  viewData_->songOffset_ = saveOffset_;

  isDirty_ = true;
}

/*******************************************************************************
 pasteSelection:
        paste clipboard content to song
******************************************************************************/

void SongView::pasteClipboard() {

  if (!clipboard_.data_)
    return;

  // Check we're not out of scope

  int width = clipboard_.width_;
  int height = clipboard_.height_;

  if (viewData_->songX_ + width > SONG_CHANNEL_COUNT) {
    width = SONG_CHANNEL_COUNT - viewData_->songX_;
  }
  if (viewData_->songY_ + viewData_->songOffset_ + height > SONG_ROW_COUNT) {
    height = SONG_ROW_COUNT - viewData_->songY_ - viewData_->songOffset_;
  } else {

    // Move down from insert point

    unsigned char *dst = viewData_->song_->rows_[SONG_ROW_COUNT - 1].chains + viewData_->songX_;
    unsigned char *src = dst - height * SONG_CHANNEL_COUNT;

    int rowCount = SONG_ROW_COUNT - (viewData_->songY_ + viewData_->songOffset_);

    for (int j = 0; j < rowCount; j++) {
      for (int i = 0; i < width; i++) {
        *dst++ = *src++;
      }
      dst -= (SONG_CHANNEL_COUNT + width);
      src -= (SONG_CHANNEL_COUNT + width);
    }
  }

  // Prepare copy pointer

  unsigned char *dst = viewData_->GetCurrentSongPointer();
  unsigned char *src = clipboard_.data_;

  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      *dst++ = *src++;
    }
    dst += (SONG_CHANNEL_COUNT - width);
    src += (clipboard_.width_ - width);
  }

  updateCursor(0, height);
}

void SongView::unMuteAll() {

  UIController *controller = UIController::GetInstance();
  controller->UnMuteAll();
}

void SongView::toggleMute() {

  UIController *controller = UIController::GetInstance();

  int from = viewData_->songX_;
  int to = from;
  if (clipboard_.active_) {
    GUIRect r = getSelectionRect();
    from = r.Left();
    to = r.Right() - 1;
  };
  controller->BeginMute(from, to);
}

void SongView::switchSoloMode() {

  UIController *controller = UIController::GetInstance();
  int from = viewData_->songX_;
  int to = from;
  if (clipboard_.active_) {
    GUIRect r = getSelectionRect();
    from = r.Left();
    to = r.Right() - 1;
  };
  controller->BeginSolo(from, to);
  isDirty_ = true;
}

void SongView::onStart() {
  Player *player = Player::GetInstance();
  unsigned char from = viewData_->songX_;
  unsigned char to = from;
  if (clipboard_.active_) {
    GUIRect r = getSelectionRect();
    from = r.Left();
    to = r.Right() - 1;
  }
  player->OnSongStartButton(from, to, false, false);
}

void SongView::startCurrentRow() {
  Player *player = Player::GetInstance();
  player->SetSequencerMode(SM_LIVE);
  player->OnSongStartButton(0, 7, false, false);
}

void SongView::startImmediate() {
  Player *player = Player::GetInstance();

  unsigned char from = viewData_->songX_;
  unsigned char to = from;
  player->OnSongStartButton(from, to, false, true);
}

void SongView::onStop() {
  Player *player = Player::GetInstance();
  unsigned char from = viewData_->songX_;
  unsigned char to = from;
  if (clipboard_.active_) {
    GUIRect r = getSelectionRect();
    from = r.Left();
    to = r.Right() - 1;
  }
  player->OnSongStartButton(from, to, true, false);
}

void SongView::jumpToNextSection(int direction) {

  int current = viewData_->songY_ + viewData_->songOffset_;
  bool foundGap = false;
  for (int i = 0; i < SONG_ROW_COUNT; i++) {
    unsigned char *start = viewData_->song_->rows_[current].chains + viewData_->songX_;
    if (foundGap && (*start != 0xFF)) {
      break;
    } else {
      if (*start == 0xFF) {
        foundGap = true;
      }
    }
    current += direction;
    if (current < 0) {
      current += SONG_ROW_COUNT;
    }
    if (current >= SONG_ROW_COUNT) {
      current -= SONG_ROW_COUNT;
    }
  }
  // If we go backwards, we stil have to go to the beginning of the block

  if (direction < 0) {
    while (current > 0) {
      unsigned char *start = viewData_->song_->rows_[current].chains + viewData_->songX_;
      if (*start == 0xFF) {
        current++;
        break;
      };
      current--;
    };
  }

  // Update viewdata position from current

  if ((current - viewData_->songOffset_ > 0x17) || (current - viewData_->songOffset_ < 0)) {
    viewData_->songOffset_ = current - 4;
    if (viewData_->songOffset_ < 0) {
      viewData_->songOffset_ = 0;
    }
  }
  viewData_->songY_ = current - viewData_->songOffset_;
  isDirty_ = true;
}

/*******************************************************************************
 ButtonDown / ButtonUp:
        process button events coming from the main
        application window
******************************************************************************/

void SongView::beginSelection() {
  if (clipboard_.active_) {
    return;
  }
  clipboard_.active_ = true;
  clipboard_.x_ = viewData_->songX_;
  clipboard_.y_ = viewData_->songY_;
  clipboard_.offset_ = viewData_->songOffset_;
  saveX_ = clipboard_.x_;
  saveY_ = clipboard_.y_;
  saveOffset_ = clipboard_.offset_;
}

void SongView::ButtonUp(uint16_t button, uint16_t mask) {
  // mask is what remains held after the release

  // Cloning needs ALT (ENTER while ALT is held), so there is nothing to clone once ALT is let go: the selection
  // starts here instead of with the next key press
  if (viewMode_ == VM_CLONE && button == BM_ALT) {
    viewMode_ = VM_SELECTION;
    beginSelection();
    isDirty_ = true;
  }
}

bool SongView::navigationDown(uint16_t direction) {
  switch (direction) {
    case BM_RIGHT:
      enterChainUnderCursor();
      return true;
    case BM_UP:
      switchTo(VT_PROJECT, vtRevealFromTop);
      return true;
    case BM_DOWN:
      switchTo(VT_MIXER, vtRevealFromBottom);
      return true;
    case BM_LEFT:
      toggleLoopState();
      return true;
  }
  return false;
}

void SongView::ButtonDown(uint16_t newButton, uint16_t mask) {

  if (viewMode_ == VM_NEW) {
    if (IsChord(newButton, mask, BM_ENTER)) {
      uint16_t next = viewData_->song_->chain_.GetNext();
      if (next != NO_MORE_CHAIN) {
        setChain((unsigned char)next);
        isDirty_ = true;
      }
      viewMode_ = VM_NORMAL;
      return;
    }
  }

  if (viewMode_ == VM_CLONE) {
    if (newButton == BM_ENTER && (mask & BM_ALT)) {
      clonePosition();
      viewMode_ = VM_NORMAL;
      return;
    } else {
      viewMode_ = VM_SELECTION;
    }
  };

  if (clipboard_.active_) {
    viewMode_ = VM_SELECTION;
  };
  // Process selection related keys

  if (viewMode_ == VM_SELECTION) {
    beginSelection();
    processSelectionButtonMask(newButton, mask);
  } else {

    // Switch back to normal mode

    viewMode_ = VM_NORMAL;
    processNormalButtonMask(newButton, mask);
  }
}

/*******************************************************************************
 navigateToChain / navigate helpers shared by normal and selection mode
******************************************************************************/

void SongView::switchTo(ViewType vt, ViewTransition transition) {
  ViewEventData ved = {vt, transition};
  ViewEvent ve(vetSwitchView, &ved);
  SetChanged();
  NotifyObservers(&ve);
}

void SongView::enterChainUnderCursor() {
  unsigned char *data = viewData_->GetCurrentSongPointer();
  if (*data != 0xFF) {
    ViewEventData ved = {VT_CHAIN, vtRevealFromRight};
    ViewEvent ve(vetSwitchView, &ved);
    viewData_->currentChain_ = *data;
    SetChanged();
    NotifyObservers(&ve);
  }
}

/*******************************************************************************
 processNormalButtonMask:
        process button mask in the case there is no
        selection active
******************************************************************************/

void SongView::processNormalButtonMask(uint16_t newButton, uint16_t mask) {

  // EDIT chords
  if (IsPair(newButton, mask, BM_EDIT, BM_ALT)) {
    viewMode_ = VM_CLONE;
    return;
  }
  if (IsPair(newButton, mask, BM_EDIT, BM_NAV)) {
    toggleMute();
    return;
  }
  if (mask == BM_EDIT) {
    Player *player = Player::GetInstance();
    switch (newButton) {
      case BM_DOWN:
        updateSongOffset(View::songRowCount_);
        break;
      case BM_UP:
        updateSongOffset(-View::songRowCount_);
        break;
      case BM_LEFT:
      case BM_RIGHT:
        switch (player->GetSequencerMode()) {
          case SM_SONG:
            player->SetSequencerMode(SM_LIVE);
            break;
          case SM_LIVE:
            player->SetSequencerMode(SM_SONG);
            break;
        }
        isDirty_ = true;
        break;
      case BM_PLAY:
        if (player->GetSequencerMode() == SM_LIVE) {
          startImmediate();
        }
        break;
    }
    return;
  }

  // ENTER chords
  if (mask == BM_ENTER) {
    // ENTER modifier
    switch (newButton) {
      case BM_EDIT:
        cutPosition();
        break;
      case BM_DOWN:
        updateChain(-0x10);
        break;
      case BM_UP:
        updateChain(0x10);
        break;
      case BM_LEFT:
        updateChain(-0x01);
        break;
      case BM_RIGHT:
        updateChain(0x01);
        break;
    }
    return;
  }
  if (newButton == BM_ENTER) {
    if (mask == BM_ALT) {
      pasteClipboard();
    } else if (mask == BM_NAV) {
      switchSoloMode();
    } else if (mask == 0) {
      pasteLast();
      viewMode_ = VM_NEW;
    }
    return;
  }

  // NAV chords
  if (IsPair(newButton, mask, BM_NAV, BM_ALT)) {
    unMuteAll();
    return;
  }
  if (mask == BM_NAV) {
    switch (newButton) {
      case BM_PLAY:
        onStop();
        break;
    }
    return;
  }

  // ALT chords
  if (mask == BM_ALT) {
    switch (newButton) {
      case BM_DOWN:
        jumpToNextSection(1);
        break;
      case BM_UP:
        jumpToNextSection(-1);
        break;
      case BM_PLAY:
        startCurrentRow();
        break;
      case BM_LEFT:
        nudgeTempo(-1);
        break;
      case BM_RIGHT:
        nudgeTempo(1);
        break;
    }
    return;
  }

  // No modifier
  if (mask == 0) {
    switch (newButton) {
      case BM_DOWN:
        updateCursor(0, 1);
        break;
      case BM_UP:
        updateCursor(0, -1);
        break;
      case BM_LEFT:
        updateCursor(-1, 0);
        break;
      case BM_RIGHT:
        updateCursor(1, 0);
        break;
      case BM_PLAY:
        onStart();
        break;
    }
  }
}

/*******************************************************************************
 processSelectionButtonMask:
        process button mask in the case there is a
        selection active
******************************************************************************/

void SongView::processSelectionButtonMask(uint16_t newButton, uint16_t mask) {

  if (IsPair(newButton, mask, BM_EDIT, BM_NAV)) {
    toggleMute();
  } else if (IsPair(newButton, mask, BM_EDIT, BM_ALT)) {
    extendSelection();
  } else if ((newButton & BM_DIRECTIONAL) && mask == (BM_ENTER | BM_EDIT)) {
    // ENTER + EDIT + arrows nudge the selection
    nudgeSelection(newButton == BM_RIGHT  ? +1
                   : newButton == BM_LEFT ? -1
                                          : 0,
                   newButton == BM_DOWN ? +1
                   : newButton == BM_UP ? -1
                                        : 0);
  } else if (IsChord(newButton, mask, BM_EDIT)) {
    copySelection();
  } else if (IsChord(newButton, mask, BM_ENTER, BM_ALT)) {
    cutSelection();
  } else if (IsChord(newButton, mask, BM_ENTER, BM_NAV)) {
    switchSoloMode();
  } else if (IsPair(newButton, mask, BM_NAV, BM_ALT)) {
    unMuteAll();
  } else if (mask == BM_NAV) {
    switch (newButton) {
      case BM_PLAY:
        onStop();
        break;
    }
  } else if (mask == 0) {
    // No modifier
    switch (newButton) {
      case BM_DOWN:
        updateCursor(0, 1);
        break;
      case BM_UP:
        updateCursor(0, -1);
        break;
      case BM_LEFT:
        updateCursor(-1, 0);
        break;
      case BM_RIGHT:
        updateCursor(1, 0);
        break;
      case BM_PLAY:
        onStart();
        break;
    }
  }
}

/*******************************************************************************
 Redraw:
        redraw completely the song view
******************************************************************************/

void SongView::DrawView() {
  Clear();

  // Prepare selection related information
  GUIRect selRect;
  if (clipboard_.active_) {
    selRect = getSelectionRect();
  }

  // Draw title

  Player *player = Player::GetInstance();
  drawTitleText(true);

  // Compute song grid location

  GUIPoint anchor = GetAnchor();

  // Draw section header

  SetColor(Theme::View::inactive);
  SetBackgroundColor(Theme::View::bg);
  DrawString(anchor.x_, anchor.y_ - 1, "T1 T2 T3 T4 T5 T6 T7 T8");

  // Display row numbers
  drawRowNumbers(anchor.x_ - 3, anchor.y_, viewData_->songOffset_, 16);

  char row[3];
  GUIPoint pos = anchor;

  unsigned char *data = viewData_->song_->rows_[viewData_->songOffset_].chains;

  int16_t dx = 3;
  int16_t dy = 1;

  for (int j = 0; j < View::songRowCount_; j++) {
    char p = j + viewData_->songOffset_;
    bool isAlt = (p % ALT_ROW_NUMBER == 0);

    pos.x_ = anchor.x_;

    for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
      bool highlighted;
      bool nextHighlighted = false;

      // see if we need to invert current step
      // if there's a selection or we are at cursor position

      if (clipboard_.active_) {
        highlighted = selRect.Contains({i, j + viewData_->songOffset_});
        nextHighlighted = highlighted && selRect.Contains({i + 1, j + viewData_->songOffset_});
      } else {
        highlighted = (i == viewData_->songX_ && j == viewData_->songY_);
      }

      // draw current step
      unsigned char d = *data++;
      // last possible value gets special color to be more visible (typically this is the empty placeholder),
      // chains without content have their own colors: none for chains without phrases, the other for chains that
      // only have empty phrases
      SetBackgroundColor(Theme::View::bg);
      Color color = Theme::Song::fg(isAlt);
      if (d != EMPTY_SONG_VALUE) {
        switch (viewData_->song_->GetChainContent(d)) {
          case ChainContent::NoPhrases:
            color = Theme::View::empty(true);
            break;
          case ChainContent::OnlyEmptyPhrases:
            color = Theme::View::empty(false);
            break;
          case ChainContent::HasContent:
            break;
        }
      }
      SetColor(color);

      if (highlighted) {
        SwapColors();
      }

      // the focus follows the cursor, also while selecting
      if (i == viewData_->songX_ && j == viewData_->songY_) {
        focusRect_ = GUIRect(pos.x_, pos.y_, 2, 1);
      }

      if (d == 0xFF) {
        DrawString(pos.x_, pos.y_, "--");
      } else {
        byteToHexString(d, row);
        DrawString(pos.x_, pos.y_, row);
      }

      // highlight spacing also highlit if possible

      if (!(highlighted && nextHighlighted)) {
        SetBackgroundColor(Theme::View::bg);
      }

      DrawChar(pos.x_ + 2, pos.y_, ' ');

      // Next step

      pos.x_ += dx;
    }
    pos.y_ += dy;
  }

  SetColor(Theme::View::fg);

  drawMap();

  // context help trumps everything else in the info area: the notes and the
  // CPU load are less important than knowing what the buttons do right now
  if (!drawContextLegendInfoArea()) {
    infoAreaMode_ = InfoAreaDrawMode::Notes;
    drawNotes();
    drawRenderLoad(true);
  }
  drawChainPreview();

  if (player->IsRunning()) {
    OnPlayerUpdate(PET_UPDATE);
  }

  DrawLoopMarker();

  needsPlayTimeUpdate_ = true;
  needsUIUpdate_ = true;
  AnimationUpdate();
}

ContextLegend SongView::currentContextLegend() const {
  if (viewMode_ == VM_CLONE) {
    return ContextLegend::SongClone;
  }
  if (viewMode_ == VM_SELECTION || clipboard_.active_) {
    return ContextLegend::SongSelection;
  }
  if (loopState_ == lsSelecting) {
    return ContextLegend::SongLoop;
  }
  return ContextLegend::None;
}

void SongView::DrawLoopMarker() {
  // no loop marker when idle
  if (loopState_ == lsIdle) {
    return;
  }

  int b = (loopState_ == lsSelecting) ? (viewData_->songY_ + viewData_->songOffset_) : loopB_;
  int a = loopA_;

  int visibleStart = std::min(a, b) - viewData_->songOffset_;
  int visibleEnd = std::max(a, b) - viewData_->songOffset_ + 1;

  GUIPoint pos = GetAnchor();

  SetBackgroundColor(Theme::View::bg);
  SetColor(loopState_ == lsSelecting ? Theme::View::Button::bg(true) : Theme::View::fg);

  for (int y = std::max(0, visibleStart); y < std::min(visibleEnd, 16); y++) {
    char c = CHAR(char_border_single_vertical_s);

    if (y == visibleStart) {
      c = CHAR(char_border_single_topLeft_s);
    } else if (y == visibleEnd - 1) {
      c = CHAR(char_border_single_bottomLeft_s);
    }
    if (visibleStart == visibleEnd - 1) {
      c = CHAR(char_border_single_rightBracket_s);
    }

    DrawChar(pos.x_ - 4, pos.y_ + y, c);
  }

  if (visibleStart > 15) {
    DrawChar(pos.x_ - 4, pos.y_ + 15, CHAR(char_indicator_down_s));
  } else if (visibleEnd <= 0) {
    DrawChar(pos.x_ - 4, pos.y_, CHAR(char_indicator_up_s));
  }
}

void SongView::drawChainPreview() {
  // no preview when playing
  if (Player::GetInstance()->IsRunning()) {
    return;
  }

  char buffer[3];
  buffer[2] = 0;

  int row = viewData_->songY_ + viewData_->songOffset_;
  unsigned char chainId = viewData_->song_->rows_[row].chains[viewData_->songX_];

  if (chainId == EMPTY_SONG_VALUE) {
    return;
  }

  GUIPoint pos = GetAnchor();
  SetBackgroundColor(Theme::View::bg);
  SetColor(Theme::Song::preview(false));
  DrawString(pos.x_ + 24, pos.y_ - 1, "Ch");

  for (int i = 0; i < PHRASES_PER_CHAIN; i++) {
    SetColor(Theme::Song::preview((i % ALT_ROW_NUMBER) == 0));
    unsigned char phraseId = viewData_->song_->chain_.steps_[chainId][i].phrase;

    if (phraseId == EMPTY_CHAIN_VALUE) {
      DrawString(pos.x_ + 24, pos.y_ + i, "--");
    } else {
      byteToHexString(phraseId, buffer);
      DrawString(pos.x_ + 24, pos.y_ + i, buffer);
    }
  }
}

/*******************************************************************************
 OnPlayterUpdate:
        Called when positions in player change. Should
        provide visual feedback of currently played
        position
******************************************************************************/

void SongView::OnPlayerUpdate(PlayerEventType eventType, unsigned int tick) {
  // Since this can be called from core1 via the Observer pattern,
  // we need to ensure we don't call any drawing functions directly
  // Instead of drawing directly, we'll just update our state and let
  // AnimationUpdate handle the actual drawing
  SyncMaster *sync = SyncMaster::GetInstance();
  if ((eventType == PET_UPDATE) && (!sync->MajorSlice()))
    return;

  // Set the consolidated flag for UI updates
  needsUIUpdate_ = true;

  // Only set the play time update flag when not stopping
  if (eventType != PET_STOP) {
    needsPlayTimeUpdate_ = true;
  }

  // Create a memory barrier to ensure changes are visible across cores
  createMemoryBarrier();
}

// Shows how close the audio render (core1) is to missing a buffer, in the
// bottom right corner while playing. On the device the load of core 0 (the UI)
// is shown above it in light gray. Redrawn only when a shown value changes.
void SongView::drawRenderLoad(bool force) {
  constexpr int width = 3; // "72%"
  constexpr int x = SCREEN_WIDTH - width;
  constexpr int y = SCREEN_HEIGHT - 1;

  uint32_t load = 0;

  if (Player::GetInstance()->IsRunning()) {
    load = MixerService::GetInstance()->GetRenderLoad();
  }

#ifdef CORE0_LOAD_METER
  const uint32_t uiLoad = Core0Load::Percent();
  if (!force && load == lastRenderLoad_ && uiLoad == lastUiLoad_) {
    return;
  }
  lastUiLoad_ = uiLoad;
#else
  if (!force && load == lastRenderLoad_) {
    return;
  }
#endif

  lastRenderLoad_ = load;

  SetBackgroundColor(Theme::View::bg);

  if (load >= RENDER_LOAD_CRITICAL) {
    SetColor(Theme::View::error);
  } else if (load >= RENDER_LOAD_CRITICAL) {
    SetColor(Theme::View::warning);
  } else {
    SetColor(Theme::View::fg);
  }

  char buffer[width + 1];
  npf_snprintf(buffer, sizeof(buffer), load > 99 ? "%3d" : "%2d%%", load);

#ifdef CORE0_LOAD_METER
  // CPU, then core 0 (UI), then core 1 (audio)
  DrawString(x, y - 2, "CPU");
  DrawString(x, y, buffer);

  char uiBuffer[width + 1];
  npf_snprintf(uiBuffer, sizeof(uiBuffer), "%2d%%", static_cast<int>(uiLoad));
  SetColor(Theme::View::uiLoad);
  DrawString(x, y - 1, uiBuffer);
#else
  DrawString(x, y - 1, "CPU");
  DrawString(x, y, buffer);
#endif
}

void SongView::AnimationUpdate() {
  // First call the parent class implementation to draw the battery gauge
  ScreenView::AnimationUpdate();

  // Get player instance safely
  Player *player = Player::GetInstance();
  // Only process updates if we're fully initialized
  if (!viewData_ || !player) {
    return;
  }

  // Handle any pending updates from OnPlayerUpdate
  // This ensures all UI drawing happens on the "main" thread (core0)

  // Always update VU meter even if other parts of UI dont need updating
  if (titleScrollOffset() != titleScrollDrawn_) {
    drawTitleText(false);
  }
  drawMasterVuMeter(player);
  drawTitleVuMeter(player);
  drawTitleCPUMeter();
  if (infoAreaMode_ == InfoAreaDrawMode::Notes) {
    drawRenderLoad(false);
  }

  SetBackgroundColor(Theme::View::Title::bg);
  SetColor(Theme::View::Title::fg);
  char c = (loopState_ == lsLooping) ? CHAR(char_symbol_loop_s) : CHAR(char_symbol_noloop_s);
  DrawChar(INDICATOR_LOOP_MODE_X, 0, c);

  // Use the consolidated flag for all UI updates
  if (needsUIUpdate_) {
    if (infoAreaMode_ == InfoAreaDrawMode::Notes) {
      drawNotes();
    }

    // Only handle play time updates if needed
    GUIPoint timePos = {INDICATOR_PLAY_TIME_X, 0};

    SetColor(Theme::View::Title::fg);
    SetBackgroundColor(Theme::View::Title::bg);

    if (needsPlayTimeUpdate_ && Player::GetInstance()->IsRunning()) {
      drawPlayTime(player, timePos);
      needsPlayTimeUpdate_ = false;
    } else {
      DrawString(timePos.x_, timePos.y_, "00:00");
    }

    // Handle position updates
    GUIPoint anchor = GetAnchor();
    GUIPoint pos = anchor;
    pos.x_ -= 1;

    SetBackgroundColor(Theme::View::bg);

    // Loop on all channels
    for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
      // Clear all current positions
      int y = lastPlayedPosition_[i] - viewData_->songOffset_;
      if (y >= 0 && y < View::songRowCount_) {
        pos.y_ = anchor.y_ + y;
        DrawChar(pos.x_, pos.y_, ' ', fRegular, true);
      }

      // Clear all last queued positions
      y = lastQueuedPosition_[i] - viewData_->songOffset_;
      if (y >= 0 && y < View::songRowCount_) {
        pos.y_ = anchor.y_ + y;
        DrawChar(pos.x_, pos.y_, ' ', fRegular, true);
      }

      // For each playing position, draw current location
      if (player->IsChannelPlaying(i)) {
        if (viewData_->currentPlayChain_[i] != 0xFF) {
          int y = viewData_->songPlayPos_[i] - viewData_->songOffset_;
          if (y >= 0 && y < View::songRowCount_ && viewData_->playMode_ != PM_AUDITION) {
            pos.y_ = anchor.y_ + y;
            SetBackgroundColor(Theme::View::bg);
            if (!player->IsChannelMuted(i)) {
              SetColor(Theme::Song::Playback::active);
              DrawString(pos.x_, pos.y_, char_indicator_position_s);
            } else {
              SetColor(Theme::Song::Playback::muted);
              DrawString(pos.x_, pos.y_, char_indicator_positionMuted_s);
            }
            lastPlayedPosition_[i] = viewData_->songPlayPos_[i];
          }
        }
      }

      // If in live mode, update queued position
      if (player->GetSequencerMode() == SM_LIVE) {
        if (player->GetQueueingMode(i) != QM_NONE) {
          int y = player->GetQueuePosition(i) - viewData_->songOffset_;
          if (y >= 0 && y < View::songRowCount_) {
            pos.y_ = anchor.y_ + y;
            const char *indicator = player->GetLiveIndicator(i);
            SetColor(Theme::Song::Playback::active);
            DrawString(pos.x_, pos.y_, indicator);
            lastQueuedPosition_[i] = player->GetQueuePosition(i);
          }
        }
      }
      pos.x_ += 3;
    }

    // Create a memory barrier to ensure proper synchronization between cores
    createMemoryBarrier();

    // Reset the consolidated flag
    needsUIUpdate_ = false;
  }

  // Flush the window to ensure changes are displayed
}

void SongView::nudgeTempo(int direction) {
  ApplicationCommandDispatcher *dispatcher = ApplicationCommandDispatcher::GetInstance();
  switch (direction) {
    case -1:
      dispatcher->OnNudgeDown();
      break;
    case 1:
      dispatcher->OnNudgeUp();
      break;
  }
}

void SongView::toggleLoopState() {
  switch (loopState_) {
    case lsIdle:
      // lock in first position, have second loop point follow the cursor
      loopA_ = viewData_->songY_ + viewData_->songOffset_;
      loopB_ = loopA_;
      loopState_ = lsSelecting;
      break;

    case lsSelecting:
      // loop is fully selected
      loopB_ = viewData_->songY_ + viewData_->songOffset_;
      Player::GetInstance()->SetLoopPoints(loopA_, loopB_);
      loopState_ = lsLooping;
      break;

    case lsLooping:
      // end looping
      Player::GetInstance()->SetLoopPoints(NO_LOOP, NO_LOOP);
      loopState_ = lsIdle;
  }

  SetDirty(true);
}
