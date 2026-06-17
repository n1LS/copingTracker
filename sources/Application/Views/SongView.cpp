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
#include "Application/Player/Player.h"
#include "Application/Utils/char.h"
#include "Application/Views/BaseClasses/View.h"
#include "Application/Views/SampleEditorView.h"
#include "System/Console/Trace.h"
#include "System/System/System.h"
#include "UIController.h"
#include "ViewData.h"
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
  };
  setChain((unsigned char)next);
  isDirty_ = true;
}

void SongView::extendSelection() {
  GUIRect rect = getSelectionRect();
  if (rect.Left() > 0 || rect.Right() < 7) {
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

  // eg. if the user was in master channel in mixerview and came to songview
  // we need to make sure we're not outside channel range
  if (viewData_->songX_ > SONG_CHANNEL_COUNT - 1) {
    viewData_->songX_ = 0; // default to channel 1
  }
}

GUIRect SongView::getSelectionRect() {

  GUIRect selRect(clipboard_.x_, clipboard_.y_ + clipboard_.offset_, viewData_->songX_,
                  viewData_->songY_ + viewData_->songOffset_);

  selRect.Normalize();
  return selRect;
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

  clipboard_.width_ = selRect.Width() + 1;
  clipboard_.height_ = selRect.Height() + 1;

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

  int rowCount = SONG_ROW_COUNT - selRect.Bottom() - 1;

  for (int j = 0; j < rowCount; j++) {

    for (int i = 0; i < clipboard_.width_; i++) {
      *dst++ = *src++;
    }
    src += (SONG_CHANNEL_COUNT - clipboard_.width_);
    dst += (SONG_CHANNEL_COUNT - clipboard_.width_);
  }

  for (int j = 0; j > clipboard_.height_; j++) {
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
    to = r.Right();
  };
  controller->ToggleMute(from, to);
  viewMode_ = (viewMode_ != VM_MUTEON) ? VM_MUTEON : VM_NORMAL;
}

void SongView::switchSoloMode() {

  UIController *controller = UIController::GetInstance();
  int from = viewData_->songX_;
  int to = from;
  if (clipboard_.active_) {
    GUIRect r = getSelectionRect();
    from = r.Left();
    to = r.Right();
  };
  controller->SwitchSoloMode(from, to, (viewMode_ != VM_SOLOON));
  viewMode_ = (viewMode_ != VM_SOLOON) ? VM_SOLOON : VM_NORMAL;
  isDirty_ = true;
}

void SongView::onStart() {
  Player *player = Player::GetInstance();
  unsigned char from = viewData_->songX_;
  unsigned char to = from;
  if (clipboard_.active_) {
    GUIRect r = getSelectionRect();
    from = r.Left();
    to = r.Right();
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
    to = r.Right();
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
 ProcessButtonMask:
        process button mask even coming from the main
        application window
******************************************************************************/

void SongView::ProcessButtonMask(uint16_t mask, bool pressed) {

  if (!pressed) {
    if (viewMode_ == VM_MUTEON) {
      if (mask & BM_NAV) {
        toggleMute();
      }
    };
    if (viewMode_ == VM_SOLOON) {
      if (mask & BM_NAV) {
        switchSoloMode();
      }
    };
    return;
  };

  if (viewMode_ == VM_NEW) {
    if (mask == BM_ENTER) {
      uint16_t next = viewData_->song_->chain_.GetNext();
      if (next != NO_MORE_CHAIN) {
        setChain((unsigned char)next);
        isDirty_ = true;
      }
      mask &= (0xFFFF - BM_ENTER);
    }
  }

  if (viewMode_ == VM_CLONE) {
    if ((mask & BM_ENTER) && (mask & BM_ALT)) {
      clonePosition();
      mask &= (0xFFFF - (BM_ENTER | BM_ALT));
    } else {
      viewMode_ = VM_SELECTION;
    }
  };

  if (clipboard_.active_) {
    viewMode_ = VM_SELECTION;
  };
  // Process selection related keys

  if (viewMode_ == VM_SELECTION) {
    if (clipboard_.active_ == false) {
      clipboard_.active_ = true;
      clipboard_.x_ = viewData_->songX_;
      clipboard_.y_ = viewData_->songY_;
      clipboard_.offset_ = viewData_->songOffset_;
      saveX_ = clipboard_.x_;
      saveY_ = clipboard_.y_;
      saveOffset_ = clipboard_.offset_;
    }
    processSelectionButtonMask(mask);
  } else {

    // Switch back to normal mode

    viewMode_ = VM_NORMAL;
    processNormalButtonMask(mask);
  }
}

/*******************************************************************************
 processNormalButtonMask:
        process button mask in the case there is no
        selection active
******************************************************************************/

void SongView::processNormalButtonMask(unsigned int mask) {

  if (mask & BM_EDIT) {
    // EDIT Modifier
    Player *player = Player::GetInstance();
    if (mask & BM_DOWN)
      updateSongOffset(View::songRowCount_);
    if (mask & BM_UP)
      updateSongOffset(-View::songRowCount_);
    if (mask & (BM_RIGHT | BM_LEFT)) {
      switch (player->GetSequencerMode()) {
        case SM_SONG:
          player->SetSequencerMode(SM_LIVE);
          break;
        case SM_LIVE:
          player->SetSequencerMode(SM_SONG);
          break;
      }
      isDirty_ = true;
    }
    if ((mask & BM_ENTER) && (!(mask & BM_NAV)))
      cutPosition();
    if (mask & BM_ALT) {
      viewMode_ = VM_CLONE;
    };
    if (mask & BM_NAV) {
      toggleMute();
    };
    if (mask & BM_PLAY) {
      if (player->GetSequencerMode() == SM_LIVE) {
        startImmediate();
      }
    }
  } else if (mask & BM_ENTER) {
    // ENTER modifier

    if (mask & BM_DOWN)
      updateChain(-0x10);
    if (mask & BM_UP)
      updateChain(0x10);
    if (mask & BM_LEFT)
      updateChain(-0x01);
    if (mask & BM_RIGHT)
      updateChain(0x01);
    if (mask & BM_ALT)
      pasteClipboard();
    if (mask == BM_ENTER) {
      pasteLast();
      viewMode_ = VM_NEW;
    }
    if (mask & BM_NAV) {
      switchSoloMode();
    };
  } else if (mask & BM_NAV) {
    // NAV Modifier

    if (mask & BM_ALT) {
      unMuteAll();
    }

    if (mask & BM_RIGHT) {
      unsigned char *data = viewData_->GetCurrentSongPointer();
      if (*data != 0xFF) {
        ViewType vt = VT_CHAIN;
        ViewEvent ve(VET_SWITCH_VIEW, &vt);
        viewData_->currentChain_ = *data;
        SetChanged();
        NotifyObservers(&ve);
      }
    }

    if (mask & BM_UP) {
      ViewType vt = VT_PROJECT;
      ViewEvent ve(VET_SWITCH_VIEW, &vt);
      SetChanged();
      NotifyObservers(&ve);
    }

    if (mask & BM_DOWN) {
      ViewType vt = VT_MIXER;
      ViewEvent ve(VET_SWITCH_VIEW, &vt);
      SetChanged();
      NotifyObservers(&ve);
    }

    if (mask & BM_PLAY) {
      onStop();
    }

  } else if (mask & BM_ALT) {
    // ALT Modifier
    if (mask & BM_DOWN) {
      jumpToNextSection(1);
    }
    if (mask & BM_UP) {
      jumpToNextSection(-1);
    }
    if (mask & BM_PLAY) {
      startCurrentRow();
    }
    if (mask & BM_LEFT) {
      nudgeTempo(-1);
    }
    if (mask & BM_RIGHT) {
      nudgeTempo(1);
    }
  } else {
    // No modifier
    if (mask & BM_DOWN) {
      updateCursor(0, 1);
    }
    if (mask & BM_UP) {
      updateCursor(0, -1);
    }
    if (mask & BM_LEFT) {
      updateCursor(-1, 0);
    }
    if (mask & BM_RIGHT) {
      updateCursor(1, 0);
    }
    if (mask & BM_PLAY) {
      onStart();
    }
  }
}

/*******************************************************************************
 processSelectionButtonMask:
        process button mask in the case there is a
        selection active
******************************************************************************/

void SongView::processSelectionButtonMask(unsigned int mask) {

  if (mask & BM_EDIT) {
    // EDIT Modifier
    if (mask & BM_NAV) {
      toggleMute();
    };
    if (mask & BM_ALT) {
      extendSelection();
    };
    if (mask == BM_EDIT) {
      copySelection();
    }

  } else if (mask & BM_ENTER) {
    // ENTER modifier
    if (mask & BM_ALT) {
      cutSelection();
    }
    if (mask & BM_NAV) {
      switchSoloMode();
    };
  } else if (mask & BM_NAV) {
    // NAV Modifier
    if (mask & BM_ALT) {
      unMuteAll();
    }

    if (mask & BM_RIGHT) {
      unsigned char *data = viewData_->GetCurrentSongPointer();
      if (*data != 0xFF) {
        ViewType vt = VT_CHAIN;
        ViewEvent ve(VET_SWITCH_VIEW, &vt);
        viewData_->currentChain_ = *data;
        SetChanged();
        NotifyObservers(&ve);
      }
    }

    if (mask & BM_UP) {
      ViewType vt = VT_PROJECT;
      ViewEvent ve(VET_SWITCH_VIEW, &vt);
      SetChanged();
      NotifyObservers(&ve);
    }

    if (mask & BM_DOWN) {
      ViewType vt = VT_MIXER;
      ViewEvent ve(VET_SWITCH_VIEW, &vt);
      SetChanged();
      NotifyObservers(&ve);
    }

    if (mask & BM_PLAY) {
      onStop();
    }
  } else {
    // No modifier

    if (mask & BM_DOWN)
      updateCursor(0, 1);
    if (mask & BM_UP)
      updateCursor(0, -1);
    if (mask & BM_LEFT)
      updateCursor(-1, 0);
    if (mask & BM_RIGHT)
      updateCursor(1, 0);
    if (mask & BM_PLAY) {
      onStart();
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
    selRect = GUIRect(clipboard_.x_, clipboard_.y_ + clipboard_.offset_, viewData_->songX_,
                      viewData_->songY_ + viewData_->songOffset_);
    selRect.Normalize();
  }

  // Draw title

  Player *player = Player::GetInstance();
  Variable *v = viewData_->project_->FindVariable(FourCC::VarProjectName);
  DrawTitle(player->GetSequencerMode() == SM_SONG ? "Song %s" : "Live %s", v->GetString().c_str());

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

      // see if we need to invert current step
      // if there's a selection or we are at cursor position

      if (clipboard_.active_) {
        highlighted = selRect.Contains({i, j + viewData_->songOffset_});
      } else {
        highlighted = (i == viewData_->songX_ && j == viewData_->songY_);
      }

      // draw current step
      unsigned char d = *data++;
      // last possible value gets special color to be more visible (typically this is the empty placeholder)
      SetColor(d == 0xFE ? Theme::Song::placeholder : Theme::Song::fg(isAlt));

      if (highlighted) {
        SwapColors();
      }

      if (d == 0xFF) {
        DrawString(pos.x_, pos.y_, "--");
      } else {
        hex2char(d, row);
        DrawString(pos.x_, pos.y_, row);
      }

      // Put back drawing state
      SetColor(Theme::View::fg);
      SetBackgroundColor(Theme::View::bg);

      // Next step

      pos.x_ += dx;
    }
    pos.y_ += dy;
  }

  SetColor(Theme::View::fg);

  drawMap();
  drawNotes();
  drawChainPreview();

  if (player->IsRunning()) {
    OnPlayerUpdate(PET_UPDATE);
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

  for (int i = 0; i < PHRASES_PER_CHAIN; i++) {
    SetColor(Theme::Song::preview((i % ALT_ROW_NUMBER) == 0));
    unsigned char phraseId = viewData_->song_->chain_.steps_[chainId][i].phrase;

    if (phraseId == EMPTY_CHAIN_VALUE) {
      DrawString(pos.x_ + 24, pos.y_ + i, "--");
    } else {
      hex2char(phraseId, buffer);
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
  drawMasterVuMeter(player);

  // Use the consolidated flag for all UI updates
  if (needsUIUpdate_) {
    drawNotes();

    // Only handle play time updates if needed
    if (needsPlayTimeUpdate_) {
      GUIPoint timePos = 0;
      timePos.x_ = 27;
      timePos.y_ += 1;
      SetColor(Theme::View::fg);
      drawPlayTime(player, timePos);
      needsPlayTimeUpdate_ = false;
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
        DrawString(pos.x_, pos.y_, " ");
      }

      // Clear all last queued positions
      y = lastQueuedPosition_[i] - viewData_->songOffset_;
      if (y >= 0 && y < View::songRowCount_) {
        pos.y_ = anchor.y_ + y;
        DrawString(pos.x_, pos.y_, " ");
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
  w_.Flush();
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
