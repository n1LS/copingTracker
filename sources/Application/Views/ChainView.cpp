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

#include "ChainView.h"
#include "Application/Utils/char.h"
#include "Application/Views/SampleEditorView.h"
#include "ScreenView.h"
#include "System/Console/Trace.h"
#include "UIController.h"
#include "ViewData.h"
#include <nanoprintf.h>

ChainView::ChainView(GUIWindow &w, ViewData *viewData) : ScreenView(w, viewData) {
  updatingPhrase_ = false;
  lastPhrase_ = 0;
  lastPlayingPos_ = 0;
  lastQueuedPos_ = 0;

  clipboard_.active_ = false;
  clipboard_.width_ = 0;
  clipboard_.height_ = 0;

  for (int i = 0; i < 16; i++) {
    clipboard_.steps_[i] = {0xFF, 0};
  };
}

void ChainView::Reset() {
  updatingPhrase_ = false;
  lastPhrase_ = 0;
  lastPlayingPos_ = 0;
  lastQueuedPos_ = 0;

  clipboard_.active_ = false;
  clipboard_.width_ = 0;
  clipboard_.height_ = 0;
  clipboard_.col_ = 0;
  clipboard_.row_ = 0;
  for (int i = 0; i < 16; i++) {
    clipboard_.steps_[i] = {0xFF, 0};
  }

  saveRow_ = 0;
  saveCol_ = 0;
  needsUIUpdate_ = false;
}

void ChainView::setPhrase(unsigned char value) {
  viewData_->SetChainPhrase(value);
  lastPhrase_ = value;
  isDirty_ = true;
}

void ChainView::cutPosition() {
  clipboard_.active_ = true;
  clipboard_.row_ = viewData_->chainRow_;
  clipboard_.col_ = viewData_->chainCol_;
  saveRow_ = viewData_->chainRow_;
  saveCol_ = viewData_->chainCol_;
  cutSelection();
}

void ChainView::pasteLastPhrase() {
  // If we're on an empty spot, we past the last phrase
  // otherwise we take the current phrase as last

  unsigned char *c = viewData_->GetCurrentChainPointer();
  if ((*c == 0xFF)) {
    *c = lastPhrase_;
    isDirty_ = true;
  } else {
    lastPhrase_ = *c;
  }
}

void ChainView::updateCursor(int dx, int dy) {
  viewData_->UpdateChainCursor(dx, dy);
  isDirty_ = true;
}

void ChainView::updateCursorValue(int offset, int dx, int dy) {
  unsigned char v = viewData_->UpdateChainCursorValue(offset, dx, dy);

  if (viewData_->chainCol_ == 0) {
    lastPhrase_ = v;
    updatingPhrase_ = true;
    updateRow_ = viewData_->chainRow_;
  }

  isDirty_ = true;
}

void ChainView::updateSelectionValue(int offset) { // HERE
  int savecol = viewData_->chainCol_;
  int saverow = viewData_->chainRow_;
  GUIRect r = getSelectionRect();
  viewData_->chainCol_ = r.Left();
  viewData_->chainRow_ = r.Top();
  for (int i = 0; i <= r.Width(); i++) {
    for (int j = 0; j <= r.Height(); j++) {
      updateCursorValue(offset, i, j);
    }
  }
  viewData_->chainCol_ = savecol;
  viewData_->chainRow_ = saverow;
}

void ChainView::warpInColumn(int offset) {
  // save current data

  int saveY = viewData_->songY_;
  int saveOffset = viewData_->songOffset_;

  // move and check we're on valid chain

  viewData_->UpdateSongCursor(0, offset);
  unsigned char *data = viewData_->GetCurrentSongPointer();
  if (*data != 0xFF) {
    viewData_->currentChain_ = *data;
    isDirty_ = true;
  } else {
    // restore old position
    viewData_->songY_ = saveY;
    viewData_->songOffset_ = saveOffset;
  }
}

void ChainView::warpToNeighbour(int offset) {

  int newPos = viewData_->songX_ + offset;
  if ((newPos > -1) && (newPos < SONG_CHANNEL_COUNT)) {
    viewData_->songX_ = newPos;
    unsigned char *c = viewData_->GetCurrentSongPointer();
    if (*c != 0xFF) {
      viewData_->currentChain_ = *c;
      isDirty_ = true;
    } else {
      viewData_->songX_ -= offset;
    }
  }
}

void ChainView::clonePosition() {

  unsigned char *pos = viewData_->GetCurrentChainPointer();
  unsigned char current = *pos;
  if (current == 255)
    return;

  uint16_t next = viewData_->song_->phrase_.GetNext();
  if (next == NO_MORE_PHRASE)
    return;

  PhraseStep *src = viewData_->song_->phrase_.steps_[current];
  PhraseStep *dst = viewData_->song_->phrase_.steps_[next];
  for (int i = 0; i < 16; i++) {
    *dst++ = *src++;
  };

  setPhrase((unsigned char)next);
  isDirty_ = true;
}

/*******************************************************************************
 getSelectionRect:
        gets the normalized rectangle of the current
        selection. Valid only while selection is drawn
******************************************************************************/

GUIRect ChainView::getSelectionRect() {
  GUIRect r(clipboard_.col_, clipboard_.row_, viewData_->chainCol_, viewData_->chainRow_);
  r.Normalize();
  return r;
}

/*******************************************************************************
 fillClipboardData:

        copies the necessary information from the
        current selection to the clipboard for future
        paste. We're copying data all across the row
        because we"re too lazy to try to figure a better
        procedure
******************************************************************************/

void ChainView::fillClipboardData() {

  // Get Current normalized selection rect

  GUIRect selRect = getSelectionRect();

  // Get size & store in clipboard

  clipboard_.width_ = selRect.Width() + 1;
  clipboard_.height_ = selRect.Height() + 1;
  clipboard_.row_ = selRect.Top();
  ;
  clipboard_.col_ = selRect.Left();

  // Copy the data

  ChainStep *base = viewData_->song_->chain_.steps_[viewData_->currentChain_];

  for (int i = 0; i < clipboard_.height_; i++) {
    clipboard_.steps_[i] = base[clipboard_.row_ + i];
  };
}

void ChainView::extendSelection() {
  GUIRect rect = getSelectionRect();
  if (rect.Left() > 0 || rect.Right() < 1) {
    if (viewData_->chainCol_ < clipboard_.col_) {
      viewData_->chainCol_ = 0;
      clipboard_.col_ = 1;
    } else {
      viewData_->chainCol_ = 1;
      clipboard_.col_ = 0;
    }
    isDirty_ = true;
  } else {
    if (viewData_->chainRow_ < clipboard_.row_) {
      viewData_->chainRow_ = 0;
      clipboard_.row_ = 15;
    } else {
      clipboard_.row_ = 0;
      viewData_->chainRow_ = 15;
    }
    isDirty_ = true;
  }
}

/*******************************************************************************
 copySelection:
        copies data in the current selection to the
        clipboard & end selection process
******************************************************************************/

void ChainView::copySelection() {

  // Keep up with row,col of selection because fillClipboardData will trash it
  //	saveClipboardPosition() ;

  fillClipboardData();

  clipboard_.active_ = false;
  viewMode_ = VM_NORMAL;

  viewData_->chainRow_ = saveRow_;
  viewData_->chainCol_ = saveCol_;

  isDirty_ = true;
}

/*******************************************************************************
 cut:  copies data in the current selection to the
       clipboard, clear selection content & end selection
       process
******************************************************************************/

void ChainView::cutSelection() {

  // Keep up with row,col of selection because
  // fillClipboardData will trash it

  //	saveClipboardPosition() ;

  fillClipboardData();

  // Loop over selection col, row & clear data inside it

  ChainStep *base = viewData_->song_->chain_.steps_[viewData_->currentChain_];

  for (int i = 0; i < clipboard_.width_; i++) {
    for (int j = 0; j < clipboard_.height_; j++) {
      switch (i + clipboard_.col_) {
        case 0:
          base[j + clipboard_.row_].phrase = 0xFF;
          break;
        case 1:
          base[j + clipboard_.row_].transpose = 0x00;
          break;
      }
    }
  }

  // Clear selection, end selection process & reposition cursor

  clipboard_.active_ = false;
  viewMode_ = VM_NORMAL;
  viewData_->chainRow_ = saveRow_;
  viewData_->chainCol_ = saveCol_;
  isDirty_ = true;
}

/*******************************************************************************
 pasteClipboard:
        copies data in the clipboard to the current step
******************************************************************************/

void ChainView::pasteClipboard() {

  // Get number of row to paste

  int height = clipboard_.height_;
  if (viewData_->chainRow_ + height > 16) {
    height = 16 - viewData_->chainRow_;
  }

  ChainStep *base = viewData_->song_->chain_.steps_[viewData_->currentChain_];

  for (int i = 0; i < clipboard_.width_; i++) {
    for (int j = 0; j < height; j++) {
      switch (i + clipboard_.col_) {
        case 0:
          base[j + viewData_->chainRow_].phrase = clipboard_.steps_[j].phrase;
          break;
        case 1:
          base[j + viewData_->chainRow_].transpose = clipboard_.steps_[j].transpose;
          break;
      }
    }
  }
  updateCursor(0x00, height);
  isDirty_ = true;
}

void ChainView::unMuteAll() {

  UIController *controller = UIController::GetInstance();
  controller->UnMuteAll();
}

void ChainView::toggleMute() {

  UIController *controller = UIController::GetInstance();
  controller->ToggleMute(viewData_->songX_, viewData_->songX_);
  viewMode_ = (viewMode_ != VM_MUTEON) ? VM_MUTEON : VM_NORMAL;
}

void ChainView::switchSoloMode() {

  UIController *controller = UIController::GetInstance();
  controller->SwitchSoloMode(viewData_->songX_, viewData_->songX_, (viewMode_ == VM_NORMAL));
  viewMode_ = (viewMode_ != VM_SOLOON) ? VM_SOLOON : VM_NORMAL;
  isDirty_ = true;
}

void ChainView::ProcessButtonMask(uint16_t mask, bool pressed) {

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
      uint16_t next = viewData_->song_->phrase_.GetNext();
      if (next != NO_MORE_PHRASE) {
        setPhrase((unsigned char)next);
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

  // Process selection related keys

  if (viewMode_ == VM_SELECTION) {

    if (clipboard_.active_ == false) {
      clipboard_.active_ = true;
      clipboard_.col_ = viewData_->chainCol_;
      clipboard_.row_ = viewData_->chainRow_;
      saveCol_ = viewData_->chainCol_;
      saveRow_ = viewData_->chainRow_;
    }
    processSelectionButtonMask(mask);
  } else {

    // Switch back to normal mode

    viewMode_ = VM_NORMAL;
    processNormalButtonMask(mask);
  }
}

void ChainView::processNormalButtonMask(uint16_t mask) {

  Player *player = Player::GetInstance();

  if (mask & BM_EDIT) {
    // EDIT Modifier
    if (mask & BM_LEFT)
      warpToNeighbour(-1);
    if (mask & BM_RIGHT)
      warpToNeighbour(+1);
    if (mask & BM_UP)
      warpInColumn(-1);
    if (mask & BM_DOWN)
      warpInColumn(+1);
    if (mask & BM_ENTER)
      cutPosition();
    if (mask & BM_ALT) {
      viewMode_ = VM_CLONE;
    };
    if (mask & BM_NAV)
      toggleMute();
  } else if (mask & BM_ENTER) {
    // ENTER Modifier
    if (mask & BM_DOWN)
      updateCursorValue(viewData_->chainCol_ == 0 ? -0x10 : -0x0C);
    if (mask & BM_UP)
      updateCursorValue(viewData_->chainCol_ == 0 ? 0x10 : 0x0C);
    if (mask & BM_LEFT)
      updateCursorValue(-1);
    if (mask & BM_RIGHT)
      updateCursorValue(+1);
    if (mask & BM_ALT)
      pasteClipboard();
    if (mask == BM_ENTER) {
      pasteLastPhrase();
      if (viewData_->chainCol_ == 0)
        viewMode_ = VM_NEW;
    }
    if (mask & BM_NAV)
      switchSoloMode();
  } else if (mask & BM_NAV) {
    // NAV Modifier
    if (mask & BM_LEFT) {
      Navigate(VT_SONG);
    } else if (mask & BM_RIGHT) {
      unsigned char *data = viewData_->GetCurrentChainPointer();
      if (*data != 0xFF) {
        viewData_->currentPhrase_ = *data;
        Navigate(VT_PHRASE);
      }
    }

    // We toggle full chain start only if we"re not in live mode
    // or if the player ain't playing yet

    if (mask & BM_PLAY) {
      player->OnStartButton(PM_CHAIN, viewData_->songX_, true, viewData_->chainRow_);
    }
    if (mask & BM_ALT)
      unMuteAll();
  } else {
    // NO modifier
    if (mask & BM_DOWN)
      updateCursor(0, 1);
    if (mask & BM_UP)
      updateCursor(0, -1);
    if (mask & BM_LEFT)
      updateCursor(-1, 0);
    if (mask & BM_RIGHT)
      updateCursor(1, 0);
    if (mask & BM_PLAY) {
      player->OnStartButton(PM_CHAIN, viewData_->songX_, false, viewData_->chainRow_);
    }
  }

  if ((!(mask & BM_ENTER)) && updatingPhrase_) {
    uint8_t p = viewData_->song_->chain_.steps_[viewData_->currentChain_][updateRow_].phrase;
    viewData_->song_->phrase_.SetUsed(p);
    updatingPhrase_ = false;
  }
}

void ChainView::processSelectionButtonMask(uint16_t mask) {

  Player *player = Player::GetInstance();

  // B Modifier

  if (mask & BM_EDIT) {
    if (mask == BM_EDIT)
      copySelection();
    if (mask & BM_NAV)
      toggleMute();
    if (mask & BM_ALT)
      extendSelection();
  } else {

    // A modifier

    if (mask & BM_ENTER) {

      if (mask & BM_DOWN)
        updateSelectionValue(viewData_->chainCol_ == 0 ? -0x10 : -0x0C);
      if (mask & BM_UP)
        updateSelectionValue(viewData_->chainCol_ == 0 ? 0x10 : 0x0C);
      if (mask & BM_LEFT)
        updateSelectionValue(-0x01);
      if (mask & BM_RIGHT)
        updateSelectionValue(0x01);

      if (mask & BM_ALT) {
        cutSelection();
      }
      if (mask & BM_NAV)
        switchSoloMode();
    } else {

      // R Modifier

      if (mask & BM_NAV) {

        if (mask & BM_LEFT) {
          Navigate(VT_SONG);
          ;
          ;
        }

        if (mask & BM_RIGHT) {
          unsigned char *data = viewData_->GetCurrentChainPointer();
          if (*data != 0xFF) {
            viewData_->currentPhrase_ = *data;
            Navigate(VT_PHRASE);
          }
        }

        if (mask & BM_PLAY) {
          player->OnStartButton(PM_CHAIN, viewData_->songX_, true, viewData_->chainRow_);
        }

        if (mask & BM_ALT)
          unMuteAll();

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
          player->OnStartButton(PM_CHAIN, viewData_->songX_, false, viewData_->chainRow_);
        }
      }
    }
  }
}

void ChainView::OnFocus() {
  clipboard_.active_ = false;

  // reset the cursor position to within the range with data
  while (viewData_->chainRow_ > 0 &&
         viewData_->song_->chain_.steps_[viewData_->currentChain_][viewData_->chainRow_].phrase == 0xFF) {
    viewData_->chainRow_--;
  }
}

void ChainView::setTextProps(int col, int row) {
  bool highlighted = false;

  if (clipboard_.active_) {
    GUIRect selRect = getSelectionRect();
    if (selRect.Contains(GUIPoint(col, row))) {
      highlighted = true;
    }
  } else {
    if ((viewData_->chainCol_ == col) && (viewData_->chainRow_ == row)) {
      highlighted = true;
    }
  }

  SetColor(Theme::Song::fg(row % ALT_ROW_NUMBER == 0));
  SetBackgroundColor(Theme::View::bg);

  if (highlighted) {
    SwapColors();
  }
}

void ChainView::DrawView() {
  Clear();

  // Draw title

  DrawTitle("Chain %2.2X", viewData_->currentChain_);

  // Compute song grid location

  GUIPoint pos = GetAnchor();

  // Draw section header

  SetColor(Theme::View::inactive);
  SetBackgroundColor(Theme::View::bg);
  DrawString(pos.x_, pos.y_ - 1, "Ph Tsp           Nte In");

  // Display row numbers

  drawRowNumbers(pos.x_ - 3, pos.y_, 0, 16);

  // Display phrases
  ChainStep *base = viewData_->song_->chain_.steps_[viewData_->currentChain_];

  char row[6];
  row[5] = 0;

  for (int j = 0; j < 16; j++) {
    unsigned char d = base[j].phrase;
    setTextProps(0, j);
    if (d == EMPTY_CHAIN_VALUE) {
      DrawString(pos.x_, pos.y_ + j, "--");
    } else {
      byteToHexString(d, row);
      DrawString(pos.x_, pos.y_ + j, row);
    }
  }

  // Draw Transpose

  pos.x_ += 3;

  for (int j = 0; j < 16; j++) {
    unsigned char d = base[j].transpose;
    byteToHexString(d, row);
    setTextProps(1, j);
    DrawString(pos.x_, pos.y_, row);

    // also draw signed decimal display
    int8_t transpose = (int8_t)d;

    SetBackgroundColor(Theme::View::bg);

    if (transpose > 0) {
      npf_snprintf(row, sizeof(row), "+%2d", transpose);
      SetColor(Theme::Data::positive);
      DrawString(pos.x_ + 3, pos.y_, row);
    } else if (transpose < 0) {
      npf_snprintf(row, sizeof(row), "-%2d", -transpose);
      SetColor(Theme::Data::negative);
      DrawString(pos.x_ + 3, pos.y_, row);
    } else {
      DrawString(pos.x_ + 3, pos.y_, "   ");
    }

    pos.y_++;
  }

  Player *player = Player::GetInstance();

  unsigned char phrase = viewData_->song_->chain_.steps_[viewData_->currentChain_][viewData_->chainRow_].phrase;
  if (phrase != EMPTY_CHAIN_VALUE) {
    drawPhrasePreview(phrase);
  }

  drawMap();
  drawNotes();

  if (player->IsRunning()) {
    OnPlayerUpdate(PET_UPDATE);
  };
}

void ChainView::OnPlayerUpdate(PlayerEventType eventType, unsigned int tick) {
  // Since this can be called from core1 via the Observer pattern,
  // we need to ensure we don't call any drawing functions directly
  // Instead of drawing directly, we'll just update our state and let
  // AnimationUpdate handle the actual drawing

  // Flag that UI needs to be updated (notes, positions, VU meter)
  needsUIUpdate_ = true;
}

void ChainView::AnimationUpdate() {
  // First call the parent class implementation to draw the battery gauge
  ScreenView::AnimationUpdate();

  // Get player instance safely
  Player *player = Player::GetInstance();

  // Always update VU meter even if other parts of UI dont need updating
  drawMasterVuMeter(player);

  // Handle any pending updates from OnPlayerUpdate
  // This ensures all UI drawing happens on the "main" thread (core0)
  if (needsUIUpdate_) {
    // Draw notes
    drawNotes();

    // Handle position updates
    GUIPoint anchor = GetAnchor();
    GUIPoint pos = anchor;
    pos.x_ -= 1;

    SetBackgroundColor(Theme::View::bg);
    SetColor(Theme::View::fg);

    // Clear last played & queued
    pos.y_ = anchor.y_ + lastPlayingPos_;
    DrawString(pos.x_, pos.y_, " ");

    pos.y_ = anchor.y_ + lastQueuedPos_;
    DrawString(pos.x_, pos.y_, " ");

    // Only update play position if player is running
    if (player->IsRunning()) {
      // Loop on all channels to see if one of them is playing current chain
      for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
        if (player->IsChannelPlaying(i)) {
          if (viewData_->currentPlayChain_[i] == viewData_->currentChain_ && viewData_->playMode_ != PM_AUDITION) {
            pos.y_ = anchor.y_ + viewData_->chainPlayPos_[i];
            SetBackgroundColor(Theme::View::bg);
            if (!player->IsChannelMuted(i)) {
              SetColor(Theme::Song::Playback::active);
              DrawString(pos.x_, pos.y_, char_indicator_position_s);
            } else {
              SetColor(Theme::Song::Playback::muted);
              DrawString(pos.x_, pos.y_, char_indicator_positionMuted_s);
            }
            lastPlayingPos_ = viewData_->chainPlayPos_[i];
            break;
          }
        }
      }
    }

    // Handle queue position updates if in live mode
    if (player->GetSequencerMode() == SM_LIVE) {
      for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
        // is anything queued?
        if (player->GetQueueingMode(i) != QM_NONE) {
          // find the chain queued in channel
          unsigned char songPos = player->GetQueuePosition(i);
          unsigned char *chain = viewData_->song_->rows_[songPos].chains + i;
          if (*chain == viewData_->currentChain_) {
            unsigned char chainPos = player->GetQueueChainPosition(i);
            pos.y_ = anchor.y_ + chainPos;
            const char *indicator = player->GetLiveIndicator(i);
            DrawString(pos.x_, pos.y_, indicator);
            lastQueuedPos_ = chainPos;
            break;
          }
        }
      }
    }

    // Mark UI update as complete
    needsUIUpdate_ = false;
  }

  // Flush the window to ensure changes are displayed
}

void ChainView::drawPhrasePreview(uint8_t phrase) {
  GUIPoint pos = GetAnchor();
  pos.x_ += 17;

  // Display notes
  PhraseStep *steps = viewData_->song_->phrase_.steps_[phrase];
  unsigned char lastInstr = NO_INSTRUMENT;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();

  char buffer[6];
  buffer[4] = 0;
  for (int j = 0; j < 16; j++) {
    SetColor(Theme::Song::preview((j % ALT_ROW_NUMBER) == 0));
    unsigned char d = steps[j].note;
    unsigned char instr = steps[j].instrument;

    char noteDisplay[4];
    formatNote(d, instr, bank, noteDisplay);

    DrawString(pos.x_, pos.y_, noteDisplay);
    pos.y_++;
  }

  // Draw instruments
  pos = GetAnchor();
  pos.x_ += 21;

  PhraseStep *instrSteps = viewData_->song_->phrase_.steps_[viewData_->currentPhrase_];

  for (int j = 0; j < 16; j++) {
    SetColor(Theme::Song::preview((j % ALT_ROW_NUMBER) == 0));

    unsigned char d = instrSteps[j].instrument;

    if (d == NO_INSTRUMENT) {
      DrawString(pos.x_, pos.y_, "--");
    } else {
      npf_snprintf(buffer, sizeof(buffer), "%02X", d);
      DrawString(pos.x_, pos.y_, buffer);
    }
    pos.y_++;
  }
}