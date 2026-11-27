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
#include "Application/Model/Generators/PhraseGenerator.h"
#include "Application/Utils/ContextLegend.generated.h"
#include "Application/Utils/char.h"
#include "Application/Views/ModalDialogs/GeneratorWizardModal.h"
#include "Application/Views/SampleEditorView.h"
#include "ScreenView.h"
#include "System/Console/Trace.h"
#include "UIController.h"
#include "ViewData.h"

#include <nanoprintf.h>

const int columnPositions_[2] = {0, 3};
const int columnWidths_[2] = {2, 2};

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

// Runs the generator the wizard collected parameters for, on the current chain.
void ChainView::GeneratorWizardCallback(View &v, ModalView &dialog) {
  if (dialog.GetReturnCode() != GeneratorWizardModal::GWM_ACCEPT) {
    return;
  }

  auto &view = static_cast<ChainView &>(v);
  auto &wizard = static_cast<GeneratorWizardModal &>(dialog);

  if (wizard.GetType() != GeneratorWizardModal::GT_DRUM && wizard.GetType() != GeneratorWizardModal::GT_BASSLINE) {
    return; // other generators aren't implemented yet
  }

  ViewData *data = view.viewData_;
  const ChainStep *chain = data->song_->chain_.Steps(data->currentChain_);
  uint8_t length = 0;
  for (int y = 0; y < STEPS_PER_PHRASE; y++) {
    if (chain[y].phrase == EMPTY_CHAIN_VALUE) {
      break;
    }
    length++;
  }

  if (wizard.GetType() == GeneratorWizardModal::GT_BASSLINE) {
    PhraseGenerator::GenerateBasslinePhrases(data->project_, chain, length, wizard.GetBasslineSettings());
  } else {
    PhraseGenerator::GenerateDrumPhrases(data->project_, chain, length, wizard.GetDrumSettings());
  }
  view.isDirty_ = true;
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

  const unsigned char *c = viewData_->GetCurrentChainPointer();
  if ((*c == 0xFF)) {
    *viewData_->EditCurrentChainPointer() = lastPhrase_;
    isDirty_ = true;
  } else {
    lastPhrase_ = *c;
  }
}

void ChainView::updateCursor(int dx, int dy) {
  viewData_->UpdateChainCursor(dx, dy);
  isDirty_ = true;

  updateFocusRect();
}

void ChainView::updateFocusRect() {
  int col = viewData_->chainCol_;
  int x = 5 + columnPositions_[col];
  int y = 3 + viewData_->chainRow_;
  focusRect_ = GUIRect(x, y, columnWidths_[col], 1);
}

void ChainView::updateCursorValue(int offset, int dx, int dy) {
  int v = viewData_->UpdateChainCursorValue(offset, dx, dy);

  if (v == -1)
    return;

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
  const unsigned char *data = viewData_->GetCurrentSongPointer();
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
    const unsigned char *c = viewData_->GetCurrentSongPointer();
    if (*c != 0xFF) {
      viewData_->currentChain_ = *c;
      isDirty_ = true;
    } else {
      viewData_->songX_ -= offset;
    }
  }
}

void ChainView::clonePosition() {

  const unsigned char *pos = viewData_->GetCurrentChainPointer();
  unsigned char current = *pos;
  if (current == 255)
    return;

  uint16_t next = viewData_->song_->phrase_.GetNext();
  if (next == NO_MORE_PHRASE)
    return;

  const PhraseStep *src = viewData_->song_->phrase_.Steps(current);
  PhraseStep *dst = viewData_->song_->phrase_.EditSteps(next);
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
  // anchor and cursor in either order: the origin is the top left most point.
  // Width and height are the distance, callers add 1 for the covered cells.
  const int left = std::min<int>(clipboard_.col_, viewData_->chainCol_);
  const int right = std::max<int>(clipboard_.col_, viewData_->chainCol_);
  const int top = std::min<int>(clipboard_.row_, viewData_->chainRow_);
  const int bottom = std::max<int>(clipboard_.row_, viewData_->chainRow_);
  return GUIRect(left, top, right - left, bottom - top);
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
  clipboard_.col_ = selRect.Left();

  // Copy the data

  const ChainStep *base = viewData_->song_->chain_.Steps(viewData_->currentChain_);

  for (int i = 0; i < clipboard_.height_; i++) {
    clipboard_.steps_[i] = base[clipboard_.row_ + i];
  };
}

void ChainView::nudgeSelection(int delta) {
  // the selection rect holds distances, the selection covers one more row and column
  const GUIRect r = getSelectionRect();
  const int left = r.Left();
  const int right = r.Left() + r.Width();
  const int top = r.Top();
  const int bottom = r.Top() + r.Height();

  // the first / last row of the chain
  if (top + delta < 0 || bottom + delta >= 16) {
    return;
  }

  Chain &chain = viewData_->song_->chain_;
  const int chainId = viewData_->currentChain_;
  const ChainStep *steps = chain.Steps(chainId);
  const ChainStep jumped = steps[delta > 0 ? bottom + 1 : top - 1];

  // only the columns of the selection move: the phrase (0) and the transpose (1)
  auto copyColumns = [&](ChainStep &target, const ChainStep &source) {
    for (int col = left; col <= right; col++) {
      if (col == 0) {
        target.phrase = source.phrase;
      } else {
        target.transpose = source.transpose;
      }
    }
  };

  if (delta > 0) {
    for (int row = bottom; row >= top; --row) {
      copyColumns(chain.EditStep(chainId, row + 1), steps[row]);
    }
    copyColumns(chain.EditStep(chainId, top), jumped);
  } else {
    for (int row = top; row <= bottom; ++row) {
      copyColumns(chain.EditStep(chainId, row - 1), steps[row]);
    }
    copyColumns(chain.EditStep(chainId, bottom), jumped);
  }

  // the selection, the position it started at (the cursor goes back there after a copy) and the cursor move
  clipboard_.row_ += delta;
  saveRow_ += delta;
  viewData_->chainRow_ += delta;
  updateFocusRect();
  isDirty_ = true;
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
  // the focus is the cell of the cursor again
  updateFocusRect();

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

  Chain &chain = viewData_->song_->chain_;
  const int chainId = viewData_->currentChain_;

  for (int i = 0; i < clipboard_.width_; i++) {
    for (int j = 0; j < clipboard_.height_; j++) {
      switch (i + clipboard_.col_) {
        case 0:
          chain.EditStep(chainId, j + clipboard_.row_).phrase = 0xFF;
          break;
        case 1:
          chain.EditStep(chainId, j + clipboard_.row_).transpose = 0x00;
          break;
      }
    }
  }

  // Clear selection, end selection process & reposition cursor

  clipboard_.active_ = false;
  viewMode_ = VM_NORMAL;
  viewData_->chainRow_ = saveRow_;
  viewData_->chainCol_ = saveCol_;
  // the focus is the cell of the cursor again
  updateFocusRect();
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

  Chain &chain = viewData_->song_->chain_;
  const int chainId = viewData_->currentChain_;

  for (int i = 0; i < clipboard_.width_; i++) {
    for (int j = 0; j < height; j++) {
      switch (i + clipboard_.col_) {
        case 0:
          chain.EditStep(chainId, j + viewData_->chainRow_).phrase = clipboard_.steps_[j].phrase;
          break;
        case 1:
          chain.EditStep(chainId, j + viewData_->chainRow_).transpose = clipboard_.steps_[j].transpose;
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
  controller->BeginMute(viewData_->songX_, viewData_->songX_);
}

void ChainView::switchSoloMode() {

  UIController *controller = UIController::GetInstance();
  controller->BeginSolo(viewData_->songX_, viewData_->songX_);
  isDirty_ = true;
}

void ChainView::beginSelection() {
  if (clipboard_.active_) {
    return;
  }
  clipboard_.active_ = true;
  clipboard_.col_ = viewData_->chainCol_;
  clipboard_.row_ = viewData_->chainRow_;
  saveCol_ = viewData_->chainCol_;
  saveRow_ = viewData_->chainRow_;
}

void ChainView::ButtonUp(uint16_t button, uint16_t mask) {
  // mask is what remains held after the release

  // Cloning needs ALT (ENTER while ALT is held), so there is nothing to clone once ALT is let go: the selection
  // starts here instead of with the next key press
  if (viewMode_ == VM_CLONE && button == BM_ALT) {
    if (menuUsedUndo_) {
      // the menu was used for undo / redo, nothing to select
      viewMode_ = VM_NORMAL;
    } else {
      viewMode_ = VM_SELECTION;
      beginSelection();
    }
    isDirty_ = true;
  }
  if (button == BM_ENTER) {
    finalizePhraseUpdate();
  }
}

void ChainView::finalizePhraseUpdate() {
  if (updatingPhrase_) {
    uint8_t p = viewData_->song_->chain_.Step(viewData_->currentChain_, updateRow_).phrase;
    viewData_->song_->phrase_.SetUsed(p);
    updatingPhrase_ = false;
  }
}

bool ChainView::navigationDown(uint16_t direction) {
  switch (direction) {
    case BM_LEFT:
      Navigate(VT_SONG, vtRevealFromLeft);
      return true;
    case BM_RIGHT:
      enterPhraseUnderCursor();
      return true;
    case BM_DOWN:
      openGeneratorWizard();
      return true;
  }
  return false;
}

void ChainView::ButtonDown(uint16_t newButton, uint16_t mask) {
  if (viewMode_ == VM_NEW) {
    if (IsChord(newButton, mask, BM_ENTER)) {
      uint16_t next = viewData_->song_->phrase_.GetNext();
      if (next != NO_MORE_PHRASE) {
        setPhrase((unsigned char)next);
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
  }

  // Process selection related keys

  if (viewMode_ == VM_SELECTION) {
    beginSelection();
    processSelectionButtonMask(newButton, mask);
  } else {
    // Switch back to normal mode

    viewMode_ = VM_NORMAL;
    processNormalButtonMask(newButton, mask);
  }

  if (!(mask & BM_ENTER) && newButton != BM_ENTER) {
    finalizePhraseUpdate();
  }
}

void ChainView::openGeneratorWizard() {
  auto *wizard = GeneratorWizardModal::Create(*this, viewData_->project_->GetInstrumentBank());
  const ChainStep *chain = viewData_->song_->chain_.Steps(viewData_->currentChain_);
  uint8_t length = 0;
  while (length < STEPS_PER_PHRASE && chain[length].phrase != EMPTY_CHAIN_VALUE) {
    length++;
  }
  wizard->SetPhraseCount(length);
  wizard->PresetKeyAndScale(viewData_->project_->GetScaleRoot(), viewData_->project_->GetScale());
  DoModal(wizard, ModalViewCallback::create<&ChainView::GeneratorWizardCallback>());
}

void ChainView::enterPhraseUnderCursor() {
  const unsigned char *data = viewData_->GetCurrentChainPointer();
  if (*data != 0xFF) {
    viewData_->currentPhrase_ = *data;
    NavigateToPhrase(vtRevealFromRight);
  }
}

void ChainView::processNormalButtonMask(uint16_t newButton, uint16_t mask) {
  Player *player = Player::GetInstance();

  if (IsPair(newButton, mask, BM_EDIT, BM_ALT)) {
    viewMode_ = VM_CLONE;
  } else if (IsPair(newButton, mask, BM_EDIT, BM_NAV)) {
    toggleMute();
  } else if (mask == BM_EDIT) {
    // EDIT Modifier
    switch (newButton) {
      case BM_LEFT:
        warpToNeighbour(-1);
        break;
      case BM_RIGHT:
        warpToNeighbour(+1);
        break;
      case BM_UP:
        warpInColumn(-1);
        break;
      case BM_DOWN:
        warpInColumn(+1);
        break;
    }
  } else if (mask == BM_ENTER) {
    // ENTER Modifier
    switch (newButton) {
      case BM_EDIT:
        cutPosition();
        break;
      case BM_DOWN:
        updateCursorValue(viewData_->chainCol_ == 0 ? -0x10 : -0x0C);
        break;
      case BM_UP:
        updateCursorValue(viewData_->chainCol_ == 0 ? 0x10 : 0x0C);
        break;
      case BM_LEFT:
        updateCursorValue(-1);
        break;
      case BM_RIGHT:
        updateCursorValue(+1);
        break;
    }
  } else if (newButton == BM_ENTER) {
    if (mask == BM_ALT) {
      pasteClipboard();
    } else if (mask == BM_NAV) {
      switchSoloMode();
    } else if (mask == 0) {
      pasteLastPhrase();
      if (viewData_->chainCol_ == 0)
        viewMode_ = VM_NEW;
    }
  } else if (IsPair(newButton, mask, BM_NAV, BM_ALT)) {
    unMuteAll();
  } else if (mask == BM_NAV) {
    // NAV Modifier
    switch (newButton) {
      case BM_PLAY:
        player->OnStartButton(PM_CHAIN, viewData_->songX_, true, viewData_->chainRow_);
        break;
    }
  } else if (mask == 0) {
    // NO modifier
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
        player->OnStartButton(PM_CHAIN, viewData_->songX_, false, viewData_->chainRow_);
        break;
    }
  }
}

void ChainView::processSelectionButtonMask(uint16_t newButton, uint16_t mask) {

  Player *player = Player::GetInstance();

  if (IsChord(newButton, mask, BM_EDIT)) {
    copySelection();
  } else if (IsPair(newButton, mask, BM_EDIT, BM_NAV)) {
    toggleMute();
  } else if (IsPair(newButton, mask, BM_EDIT, BM_ALT)) {
    extendSelection();
  } else if ((newButton == BM_UP || newButton == BM_DOWN) && mask == (BM_ENTER | BM_EDIT)) {
    // ENTER + EDIT + UP / DOWN nudge the selection
    nudgeSelection(newButton == BM_DOWN ? +1 : -1);
  } else if (mask == BM_ENTER) {
    // Enter modifier
    switch (newButton) {
      case BM_DOWN:
        updateSelectionValue(viewData_->chainCol_ == 0 ? -0x10 : -0x0C);
        break;
      case BM_UP:
        updateSelectionValue(viewData_->chainCol_ == 0 ? 0x10 : 0x0C);
        break;
      case BM_LEFT:
        updateSelectionValue(-0x01);
        break;
      case BM_RIGHT:
        updateSelectionValue(0x01);
        break;
    }
  } else if (IsChord(newButton, mask, BM_ENTER, BM_ALT)) {
    cutSelection();
  } else if (IsChord(newButton, mask, BM_ENTER, BM_NAV)) {
    switchSoloMode();
  } else if (IsPair(newButton, mask, BM_NAV, BM_ALT)) {
    unMuteAll();
  } else if (mask == BM_NAV) {
    switch (newButton) {
      case BM_PLAY:
        player->OnStartButton(PM_CHAIN, viewData_->songX_, true, viewData_->chainRow_);
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
        player->OnStartButton(PM_CHAIN, viewData_->songX_, false, viewData_->chainRow_);
        break;
    }
  }
}

void ChainView::OnFocus() {
  clipboard_.active_ = false;

  // reset the cursor position to within the range with data
  while (viewData_->chainRow_ > 0 &&
         viewData_->song_->chain_.Step(viewData_->currentChain_, viewData_->chainRow_).phrase == 0xFF) {
    viewData_->chainRow_--;
  }

  updateFocusRect();
}

void ChainView::setTextProps(int col, int row, bool emptyPhrase) {
  bool highlighted = false;

  if (clipboard_.active_) {
    GUIRect selRect = getSelectionRect();
    selRect.w_ += 1;
    selRect.h_ += 1;
    if (selRect.Contains(GUIPoint(col, row))) {
      highlighted = true;
    }
  } else {
    if ((viewData_->chainCol_ == col) && (viewData_->chainRow_ == row)) {
      highlighted = true;
    }
  }

  SetColor(emptyPhrase ? Theme::View::empty(true) : Theme::Song::fg(row % ALT_ROW_NUMBER == 0));
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
  DrawString(pos.x_, pos.y_ - 1, "Ph Tsp");

  // Display row numbers

  drawRowNumbers(pos.x_ - 3, pos.y_, 0, 16);

  // Display phrases
  const ChainStep *base = viewData_->song_->chain_.Steps(viewData_->currentChain_);

  char row[6];
  row[5] = 0;

  for (int j = 0; j < 16; j++) {
    unsigned char d = base[j].phrase;
    // a phrase without anything in it has its own color
    setTextProps(0, j, d < PHRASE_COUNT && viewData_->song_->phrase_.IsEmpty(d));
    if (d == EMPTY_CHAIN_VALUE) {
      DrawString(pos.x_, pos.y_ + j, "--");
    } else {
      byteToHexString(d, row);
      DrawString(pos.x_, pos.y_ + j, row);
    }

    GUIRect selRect = getSelectionRect();
    selRect.w_ += 1;
    selRect.h_ += 1;
    bool highlighted = selRect.Contains(GUIPoint(0, j));
    bool nextHighlighted = selRect.Contains(GUIPoint(1, j));

    if (highlighted && nextHighlighted) {
      DrawChar(pos.x_ + 2, pos.y_ + j, ' ');
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

  unsigned char phrase = viewData_->song_->chain_.Step(viewData_->currentChain_, viewData_->chainRow_).phrase;
  if (phrase != EMPTY_CHAIN_VALUE) {
    drawPhrasePreview(phrase);
  }

  drawMap();

  // context help trumps the notes
  if (!drawContextLegendInfoArea()) {
    infoAreaMode_ = InfoAreaDrawMode::Notes;
    drawNotes();
  }

  if (player->IsRunning()) {
    OnPlayerUpdate(PET_UPDATE);
  };
}

ContextLegend ChainView::currentContextLegend() const {
  if (UIController::GetInstance()->IsMuteSoloHeld()) {
    return ContextLegend::SoloMute;
  }
  if (viewMode_ == VM_CLONE) {
    return ContextLegend::ChainClone;
  }
  if (viewMode_ == VM_SELECTION || clipboard_.active_) {
    return ContextLegend::ChainSelection;
  }
  // a single button that is down (not an arrow): the chords that can follow
  switch (chordHintButtons()) {
    case BM_ALT:
      return ContextLegend::ChainChordsAlt;
    case BM_NAV:
      return ContextLegend::ChainChordsNav;
    case BM_EDIT:
      return ContextLegend::ChainChordsEdit;
    case BM_ENTER:
      return ContextLegend::ChainChordsEnter;
    default:
      break;
  }
  return ContextLegend::None;
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
    // Draw notes, unless the info area shows context help
    if (infoAreaMode_ == InfoAreaDrawMode::Notes) {
      drawNotes();
    }

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
          const unsigned char *chain = viewData_->song_->Row(songPos).chains + i;
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

  SetColor(Theme::Song::preview(false));
  DrawString(pos.x_, pos.y_ - 1, "Nte In");

  // Display notes
  const PhraseStep *steps = viewData_->song_->phrase_.Steps(phrase);
  unsigned char lastInstr = NO_INSTRUMENT;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  int currentInstrument = NO_INSTRUMENT;
  I_Instrument *instrument = bank->noneInstrument();

  char buffer[6];
  buffer[4] = 0;
  for (int j = 0; j < 16; j++) {
    SetColor(Theme::Song::preview((j % ALT_ROW_NUMBER) == 0));
    unsigned char note = steps[j].note;
    unsigned char instr = steps[j].getInstrument();

    if (instr != NO_INSTRUMENT) {
      instrument = bank->GetInstrument(instr);
    }

    char buffer[4];
    instrument->noteDisplay(note, buffer);
    DrawString(pos.x_, pos.y_, buffer);
    pos.y_++;
  }

  // Draw instruments
  pos = GetAnchor();
  pos.x_ += 21;

  const PhraseStep *instrSteps = viewData_->song_->phrase_.Steps(phrase);

  for (int j = 0; j < 16; j++) {
    SetColor(Theme::Song::preview((j % ALT_ROW_NUMBER) == 0));

    unsigned char d = instrSteps[j].getInstrument();

    if (d == NO_INSTRUMENT) {
      DrawString(pos.x_, pos.y_, "--");
    } else {
      npf_snprintf(buffer, sizeof(buffer), "%02X", d);
      DrawString(pos.x_, pos.y_, buffer);
    }
    pos.y_++;
  }
}

// the menu that ALT + EDIT opened stays open (the clone action is still there) while ALT is held
void ChainView::AfterUndoRedo() {
  View::AfterUndoRedo();
  if (menuOpen_) {
    viewMode_ = VM_CLONE;
  }
}
