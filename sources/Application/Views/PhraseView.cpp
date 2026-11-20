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

#include "PhraseView.h"
#include "Application/Instruments/CommandList.h"
#include "Application/Instruments/SampleInstrument.h"
#include "Application/Model/Scale.h"
#include "Application/Model/Table.h"
#include "Application/Utils/ContextLegend.generated.h"
#include "Application/Utils/char.h"
#include "Application/Views/CommandView.h"
#include "Application/Views/ModalDialogs/MessageBox.h"
#include "Application/Views/SampleEditorView.h"
#include "Application/Views/ToastView.h"
#include "Foundation/Constants/GraphicCharacters.h"
#include "System/Console/Trace.h"
#include "UIController.h"
#include "ViewData.h"
#include <Application/AppWindow.h>
#include <algorithm>
#include <cstdint>
#include <etl/string.h>
#include <nanoprintf.h>
#include <stdlib.h>

static const uint8_t columnPositions_[7] = {0, 4, 7, 9, 12, 17, 20};
static const uint8_t columnWidths_[7] = {3, 2, 1, 3, 4, 3, 4};

void PhraseView::onCommandPicked(Token command) {
  if (col_ != colCmd1 && col_ != colCmd2) {
    return;
  }

  setCommand(row_, col_ == colCmd2, command);
  SetDirty(true);
}

// Column-specific value update handlers

void PhraseView::updateNoteValue(ViewUpdateDirection direction, int yOffset) {
  // LEFT / RIGHT change the note by one step of the scale, UP / DOWN by an octave
  changeNote(row_ + yOffset, direction);

  // Need to restart audition to update it with the new note
  startAudition(false);
}

void PhraseView::updateInstrumentValue(ViewUpdateDirection direction, int yOffset) {
  changeInstrument(row_ + yOffset, direction);
  // restart audition so it picks up the new instrument
  startAudition(false);
}

void PhraseView::updateVolumeValue(ViewUpdateDirection direction, int yOffset) {
  changeVolume(row_ + yOffset, direction);
  // restart audition so it picks up the new volume
  startAudition(false);
}

void PhraseView::updateCommandValue(PhraseColumn col, ViewUpdateDirection direction, int yOffset) {
  changeCommand(row_ + yOffset, col != colCmd1, direction);
}

void PhraseView::updateCommandParam(PhraseColumn col, ViewUpdateDirection direction, int yOffset) {
  changeCommandParam(row_ + yOffset, col != colCmdVal1, direction);
}

PhraseView::PhraseView(GUIWindow &w, ViewData *viewData) : PhraseEditingView(w, viewData) {
  phrase_ = &(viewData_->song_->phrase_);
  lastPlayingPos_ = 0;
  row_ = 0;
  viewData->phraseCurPos_ = 0;
  col_ = colNote;
  viewData_->phraseLastNote_ = NOTE_C3;
  viewData_->phraseLastInstrument_ = 0;
  viewData_->phraseLastCmd_ = Token::InstrumentCommandNone;
  viewData_->phraseLastParam_ = 0;
  viewData_->phraseLastVolume_ = NO_VOLUME;

  clipboard_.active_ = false;
  clipboard_.width_ = 0;
  clipboard_.height_ = 0;
}

PhraseView::~PhraseView() {};

void PhraseView::Reset() {
  phrase_ = &(viewData_->song_->phrase_);
  lastPlayingPos_ = 0;
  row_ = 0;
  col_ = colNote;
  viewData_->phraseLastNote_ = NOTE_C3;
  viewData_->phraseLastInstrument_ = 0;
  viewData_->phraseLastCmd_ = Token::InstrumentCommandNone;
  viewData_->phraseLastParam_ = 0;
  viewData_->phraseLastVolume_ = NO_VOLUME;
  viewData_->phraseCurPos_ = 0;

  clipboard_.active_ = false;
  clipboard_.width_ = 0;
  clipboard_.height_ = 0;
  clipboard_.col_ = 0;
  clipboard_.row_ = 0;

  // the clipboard of both views is empty
  viewData_->clipboardWidth_ = 0;
  viewData_->clipboardHeight_ = 0;

  saveCol_ = colNote;
  saveRow_ = 0;
  needsUIUpdate_ = false;
}

void PhraseView::updateCursor(int dx, int dy) {
  int lastCol = col_;
  int lastRow = row_;

  col_ += dx;
  row_ += dy;

  if (row_ > 15) {
    // Try to see if the current chain has a phrase after this one
    if ((viewMode_ != VM_SELECTION) && (viewData_->chainRow_ < 15)) {
      viewData_->chainRow_++;
      unsigned char *p = viewData_->GetCurrentChainPointer();
      if (*p != 0xFF) {
        viewData_->currentPhrase_ = *p;
        row_ = 0;
      } else { // rollback
        viewData_->chainRow_--;
        row_ = 15;
      }
    } else {
      row_ = 15;
    }
  }

  if (row_ < 0) {
    // Try to see if the current chain has a phrase before this one
    if ((viewMode_ != VM_SELECTION) && (viewData_->chainRow_ > 0)) {
      viewData_->chainRow_--;
      unsigned char *p = viewData_->GetCurrentChainPointer();
      if (*p != 0xFF) {
        viewData_->currentPhrase_ = *p;
        row_ = 15;
      } else { // rollback
        viewData_->chainRow_++;
        row_ = 0;
      }
    } else {
      row_ = 0;
    }
  }

  GUIPoint anchor = GetAnchor();
  GUIPoint p(anchor);

  switch (col_) {
    case colCmdVal1:
      p.x_ += columnPositions_[colCmdVal1];
      p.y_ += row_;
      cmdEditField_.SetPosition(p);
      cmdEdit_.SetInt(currentSteps()[row_].param1);
      break;
    case colCmdVal2:
      p.x_ += columnPositions_[colCmdVal2];
      p.y_ += row_;
      cmdEditField_.SetPosition(p);
      cmdEdit_.SetInt(currentSteps()[row_].param2);
      break;
    default:
      break;
  }

  viewData_->phraseCurPos_ = row_;

  int x = 5 + columnPositions_[col_];
  focusRect_ = GUIRect(x, row_ + 3, columnWidths_[col_], 1);

  if (lastCol != col_ || lastRow != row_) {
    expandFrame_ = 0;
  }

  isDirty_ = true;
}

void PhraseView::updateCursorValue(ViewUpdateDirection direction, int xOffset, int yOffset) {
  switch (col_ + xOffset) {
    case colNote:
      updateNoteValue(direction, yOffset);
      break;
    case colInstrument:
      updateInstrumentValue(direction, yOffset);
      break;
    case colVolume:
      updateVolumeValue(direction, yOffset);
      break;
    case colCmd1:
    case colCmd2:
      updateCommandValue(col_ + xOffset, direction, yOffset);
      break;
    case colCmdVal1:
    case colCmdVal2:
      updateCommandParam(col_ + xOffset, direction, yOffset);
      break;
  }
  isDirty_ = true;
}

// If we're on an empty spot, we past the last element
// otherwise we take the current phrase as last

// the field of a column
static StepField fieldOf(PhraseColumn col) {
  static const StepField fields[7] = {StepField::Note,   StepField::Instrument, StepField::Volume, StepField::Command1,
                                      StepField::Param1, StepField::Command2,   StepField::Param2};
  return fields[col];
}

void PhraseView::pasteLast() {
  if (pasteLastValue(row_, fieldOf(col_))) {
    isDirty_ = true;
  }
}

void PhraseView::cutPosition() {
  // cutting an empty note slot adds a note off
  uint8_t *note = &currentSteps()[row_].note;
  if (col_ == colNote && *note == NO_NOTE) {
    *note = NOTE_OFF;
    isDirty_ = true;
    return;
  }

  clipboard_.active_ = true;
  clipboard_.row_ = row_;
  clipboard_.col_ = (int)col_;
  saveRow_ = row_;
  saveCol_ = col_;

  // This way, A+B on note cuts the instruments/volume, too...
  if (col_ == colNote) {
    col_ += 2;
  }
  // ... and parameters get cut with commands
  if (col_ == colCmd1 || col_ == colCmd2) {
    col_ += 1;
  }

  cutSelection();
}

/*****************************************************************************
getSelectionRect:
gets the normalized rectangle of the current
selection. Valid only while selection is drawn
*****************************************************************************/

GUIRect PhraseView::getSelectionRect() {
  if (!clipboard_.active_) {
    return GUIRect(-1, -1, 0, 0);
  }
  // anchor and cursor in either order: the origin is the top left most point
  const int left = std::min<int>(clipboard_.col_, col_);
  const int right = std::max<int>(clipboard_.col_, col_);
  const int top = std::min<int>(clipboard_.row_, row_);
  const int bottom = std::max<int>(clipboard_.row_, row_);
  return GUIRect(left, top, right - left + 1, bottom - top + 1);
}

/*****************************************************************************
fillClipboardData:
copies the necessary information from the
current selection to the clipboard for future
paste. We're copying data all across the row
because we"re too lazy to try to figure a better
procedure
*****************************************************************************/

void PhraseView::fillClipboardData() {

  // Get Current normalized selection rect

  GUIRect selRect = getSelectionRect();

  // Get size & store in clipboard

  // the selection rect already is inclusive (width/height are counts)
  clipboard_.width_ = selRect.Width();
  clipboard_.height_ = selRect.Height();
  clipboard_.row_ = selRect.Top();
  clipboard_.col_ = selRect.Left();

  // Copy the data

  PhraseStep *base = currentSteps();

  for (int i = 0; i < clipboard_.height_; i++) {
    viewData_->clipboardSteps_[i] = base[clipboard_.row_ + i];
  }
  viewData_->clipboardCol_ = clipboard_.col_;
  viewData_->clipboardWidth_ = clipboard_.width_;
  viewData_->clipboardHeight_ = clipboard_.height_;

  updateCursor(0, 0);
}

void PhraseView::updateSelectionValue(ViewUpdateDirection direction) {
  saveRow_ = row_;
  saveCol_ = col_;

  GUIRect r = getSelectionRect();
  col_ = (PhraseColumn)r.Left();
  row_ = (PhraseColumn)r.Top();

  for (int i = 0; i < r.Width(); i++) {
    for (int j = 0; j < r.Height(); j++) {
      if (col_ + i < 2) {
        updateCursorValue(direction, i, j);
      }
    }
  }
  row_ = saveRow_;
  col_ = saveCol_;
}

void PhraseView::nudgeSelection(int delta) {
  // only the columns of the selection move
  const GUIRect r = getSelectionRect();
  const StepField firstField = fieldOf(static_cast<PhraseColumn>(r.Left()));
  const StepField lastField = fieldOf(static_cast<PhraseColumn>(r.Left() + r.Width() - 1));

  if (!nudgeSteps(r.Top(), r.Top() + r.Height() - 1, firstField, lastField, delta)) {
    return;
  }

  // the selection, the position it started at (the cursor goes back there after a copy) and the cursor move
  clipboard_.row_ += delta;
  saveRow_ += delta;
  row_ += delta;
  updateCursor(0, 0);
}

void PhraseView::extendSelection() {
  GUIRect rect = getSelectionRect();
  if (rect.Left() > 0 || rect.Right() <= colCmdVal2) {
    if (col_ < clipboard_.col_) {
      col_ = colNote;
      clipboard_.col_ = colCmdVal2;
    } else {
      col_ = colCmdVal2;
      clipboard_.col_ = colNote;
    }
    isDirty_ = true;
  } else {
    if (row_ < clipboard_.row_) {
      row_ = 0;
      clipboard_.row_ = 15;
    } else {
      clipboard_.row_ = 0;
      row_ = 15;
    }
    viewData_->phraseCurPos_ = row_;
    isDirty_ = true;
  }
}

/*****************************************************************************
copySelection:
copies data in the current selection to the
clipboard & end selection process
*****************************************************************************/

void PhraseView::copySelection() {

  // Keep up with row,col of selection because
  // fillClipboardData will trash it

  fillClipboardData();

  clipboard_.active_ = false;
  viewMode_ = VM_NORMAL;
  row_ = saveRow_;
  col_ = saveCol_;
  viewData_->phraseCurPos_ = row_;

  isDirty_ = true;
}

/*****************************************************************************
cut:  copies data in the current selection to the
clipboard, clear selection content & end selection
process
*****************************************************************************/

void PhraseView::cutSelection() {

  // Keep up with row,col of selection because
  // fillClipboardData will trash it

  fillClipboardData();

  // Loop over selection col, row & clear data inside it

  static const uint8_t kNone = static_cast<uint8_t>(static_cast<char>(Token::InstrumentCommandNone));
  PhraseStep *base = currentSteps();

  for (int i = 0; i < clipboard_.width_; i++) {
    for (int j = 0; j < clipboard_.height_; j++) {
      int r = j + clipboard_.row_;
      switch (i + clipboard_.col_) {
        case colNote:
          base[r].note = NO_NOTE;
          break;
        case colInstrument:
          base[r].instrument = NO_INSTRUMENT;
          break;
        case colCmd1:
          base[r].cmd1 = kNone;
          break;
        case colCmdVal1:
          base[r].param1 = 0x0000;
          break;
        case colCmd2:
          base[r].cmd2 = kNone;
          break;
        case colCmdVal2:
          base[r].param2 = 0x0000;
          break;
        case colVolume:
          base[r].volume = NO_VOLUME;
          break;
      }
    }
  }

  // Clear selection, end selection process & reposition cursor

  clipboard_.active_ = false;
  viewMode_ = VM_NORMAL;
  row_ = saveRow_;
  col_ = saveCol_;
  updateCursor(0, 0);
  isDirty_ = true;
}

/*****************************************************************************
pasteClipboard:
copies data in the clipboard to the current step
*****************************************************************************/

void PhraseView::pasteClipboard() {
  // the paste itself is the same in the piano roll
  const int height = pasteClipboardAt(row_);

  int offset = (row_ + height) % 16 - row_;
  updateCursor(0x00, offset);
  isDirty_ = true;
}

void PhraseView::unMuteAll() {
  UIController *controller = UIController::GetInstance();
  controller->UnMuteAll();
}

void PhraseView::toggleMute() {
  UIController *controller = UIController::GetInstance();
  controller->ToggleMute(viewData_->songX_, viewData_->songX_);
  viewMode_ = (viewMode_ != VM_MUTEON) ? VM_MUTEON : VM_NORMAL;
}

void PhraseView::switchSoloMode() {
  UIController *controller = UIController::GetInstance();
  controller->SwitchSoloMode(viewData_->songX_, viewData_->songX_, (viewMode_ == VM_NORMAL));
  viewMode_ = (viewMode_ != VM_SOLOON) ? VM_SOLOON : VM_NORMAL;
  isDirty_ = true;
}

void PhraseView::OnFocus() {
  PhraseEditingView::OnFocus();
  // the piano roll moves the same cursor row, the step
  row_ = viewData_->phraseCurPos_;
  phraseDiv_ = viewData_->project_->FindVariable(Token::VarPhraseDivision)->GetInt();
  clipboard_.active_ = false;
  viewMode_ = VM_NORMAL;
  updateCursor(0, 0);
}

void PhraseView::onPhraseSwitched() {
  updateCursor(0, 0);
}

void PhraseView::ButtonUp(uint16_t button, uint16_t mask) {
  // mask is what remains held after the release

  // Stop audition when ENTER is released
  if (!(mask & BM_ENTER)) {
    stopAudition();
  }

  // Exit mute-on mode when NAV is released
  if (viewMode_ == VM_MUTEON) {
    if (mask & BM_NAV) {
      toggleMute();
    }
  }

  // Exit solo-on mode when NAV is released
  if (viewMode_ == VM_SOLOON) {
    if (mask & BM_NAV) {
      switchSoloMode();
    }
  }
}

enum class TableCloneResult { NotApplicable, Cloned, NoFreeTable };

// Clones the table the TABLE command in the given command value column of the step points to and
// makes the step point to the copy (returned in clonedTable).
static TableCloneResult cloneStepTable(PhraseStep &step, int col, uint16_t &clonedTable) {
  uint16_t *param = nullptr;
  if (col == colCmdVal1 && Token::enum_type(step.cmd1) == Token::InstrumentCommandTable) {
    param = &step.param1;
  } else if (col == colCmdVal2 && Token::enum_type(step.cmd2) == Token::InstrumentCommandTable) {
    param = &step.param2;
  }

  if (param == nullptr) {
    return TableCloneResult::NotApplicable;
  }

  // the parameter is 16 bit but there are only TABLE_COUNT tables, the player
  // clamps it the same way, so this clones the table that actually plays
  int next = TableHolder::GetInstance()->Clone(std::min<int>(*param, TABLE_COUNT - 1));
  if (next == NO_MORE_TABLE) {
    return TableCloneResult::NoFreeTable;
  }

  *param = static_cast<uint16_t>(next);
  clonedTable = *param;
  return TableCloneResult::Cloned;
}

void PhraseView::ButtonDown(uint16_t newButton, uint16_t mask) {
  const bool enterAlone = IsChord(newButton, mask, BM_ENTER);

  // Handle "new item" mode: create new instrument or table when ENTER is pressed
  if (viewMode_ == VM_NEW) {
    bool consumed = false;
    if (enterAlone) {
      // Create new instrument for note or instrument columns
      if (col_ < colVolume) {
        InstrumentBank *bank = viewData_->project_->GetInstrumentBank();

        auto next = bank->GetNextFreeInstrumentSlotId();
        // New Instruments default to type NONE!
        if (next != NO_MORE_INSTRUMENT &&
            bank->AssignInstrumentToSlot(IT_NONE, next) == InstrumentAssignResult::Success) {
          unsigned char *c = &currentSteps()[row_].instrument;
          *c = (unsigned char)next;
          viewData_->phraseLastInstrument_ = next;
          isDirty_ = true;
        } else {
          // show error dialog that no more instruments are available
          MessageBox *mb = MessageBox::Create(*this, "Error", "No more instruments!", MBBF_OK);
          DoModal(mb);
          return;
        }
        consumed = true;
      } else {
        // Create new table for command parameter columns
        if ((col_ == colCmdVal1) && Token::enum_type(currentSteps()[row_].cmd1) == Token::InstrumentCommandTable) {
          TableHolder *th = TableHolder::GetInstance();
          uint16_t next = th->GetNext();
          if (next != NO_MORE_TABLE) {
            uint16_t *c = &currentSteps()[row_].param1;
            *c = next;
            isDirty_ = true;
            consumed = true;
            cmdEdit_.SetInt(next);
          }
        }
      };
    }
    if (consumed) {
      viewMode_ = VM_NORMAL;
      return;
    }
  }

  // Clone trigger: ALT+EDIT+ENTER or (in VM_CLONE mode) ALT+ENTER
  // Clones the current instrument or table
  const bool cloneChord = newButton == BM_ENTER && mask == (BM_ALT | BM_EDIT);
  const bool cloneModeChord = (viewMode_ == VM_CLONE) && newButton == BM_ENTER && (mask & BM_ALT);

  // Exit clone mode if ALT+ENTER are not both held
  if ((viewMode_ == VM_CLONE) && !cloneModeChord) {
    viewMode_ = VM_SELECTION;
  }

  if (cloneChord || cloneModeChord) {
    if (col_ < colVolume) {
      InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
      unsigned char *c = &currentSteps()[row_].instrument;
      if (*c != 0xFF) {
        uint16_t next = bank->Clone(*c);
        if (next != NO_MORE_INSTRUMENT) {
          *c = (unsigned char)next;
          viewData_->phraseLastInstrument_ = next;
          isDirty_ = true;
        }
      }
    } else {
      uint16_t clonedTable = 0;
      switch (cloneStepTable(currentSteps()[row_], col_, clonedTable)) {
        case TableCloneResult::Cloned:
          isDirty_ = true;
          cmdEdit_.SetInt(clonedTable);
          Trace::Log("PHRASEVIEW", "Cloned table -> %04x", clonedTable);
          break;
        case TableCloneResult::NoFreeTable:
          ToastView::GetInstance()->Show("No more tables available.", &ttWarning, ToastDuration::regular);
          break;
        case TableCloneResult::NotApplicable:
          break;
      }
    }
    viewMode_ = VM_NORMAL;
    clipboard_.active_ = false;
    return;
  }

  // Route to selection or normal button handling
  if (viewMode_ == VM_SELECTION) {
    // Initialize clipboard if starting a new selection
    if (!clipboard_.active_) {
      clipboard_.active_ = true;
      clipboard_.col_ = col_;
      clipboard_.row_ = row_;
      saveCol_ = col_;
      saveRow_ = row_;
    }
    processSelectionButtonMask(newButton, mask);
  } else {
    viewMode_ = VM_NORMAL;
    processNormalButtonMask(newButton, mask);
  };
}

void PhraseView::processNormalButtonMask(uint16_t newButton, uint16_t mask) {
  // PLAY, NAV + PLAY and EDIT + arrows are the same in the piano roll
  if (processPlaybackInput(newButton, mask) || processPhraseSwitchInput(newButton, mask)) {
    return;
  }

  if (IsChord(newButton, mask, BM_ENTER)) {
    // enter only: the picker for commands, if it is enabled
    if (col_ == colCmd1 || col_ == colCmd2) {
      openCommandPicker(row_, col_ == colCmd2);
    }

    // ENTER alone
    pasteLast();
    if ((col_ == colInstrument) || (col_ == colCmdVal1) || (col_ == colCmdVal2)) {
      viewMode_ = VM_NEW;
    }
    if (isAudible(fieldOf(col_))) {
      // commands don't affect the audition, so only for note, instrument and volume:
      // Start audition, stopping the audition happens in ButtonUp
      stopAudition();
      startAudition(true);
    }
    return;
  }

  if (IsPair(newButton, mask, BM_EDIT, BM_ALT)) {
    viewMode_ = VM_CLONE;
  } else if (IsPair(newButton, mask, BM_EDIT, BM_NAV)) {
    toggleMute();
  } else if (IsPair(newButton, mask, BM_NAV, BM_ALT)) {
    unMuteAll();
  } else if (mask == BM_ENTER) {
    // ENTER Modifer
    switch (newButton) {
      case BM_EDIT:
        cutPosition();
        break;
      case BM_DOWN:
        updateCursorValue(VUD_DOWN);
        break;
      case BM_UP:
        updateCursorValue(VUD_UP);
        break;
      case BM_LEFT:
        updateCursorValue(VUD_LEFT);
        break;
      case BM_RIGHT:
        updateCursorValue(VUD_RIGHT);
        break;
    }
  } else if (newButton == BM_ENTER) {
    if (mask == BM_ALT) {
      pasteClipboard();
    } else if (mask == BM_NAV) {
      switchSoloMode();
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
    }
  }
}

void PhraseView::processSelectionButtonMask(uint16_t newButton, uint16_t mask) {
  // PLAY and NAV + PLAY are the same in the piano roll
  if (processPlaybackInput(newButton, mask)) {
    return;
  }

  if (IsChord(newButton, mask, BM_EDIT)) {
    copySelection();
  } else if (IsPair(newButton, mask, BM_EDIT, BM_ALT)) {
    extendSelection();
  } else if (IsPair(newButton, mask, BM_NAV, BM_ALT)) {
    unMuteAll();
  } else if ((newButton == BM_UP || newButton == BM_DOWN) && mask == (BM_ENTER | BM_EDIT)) {
    // ENTER + EDIT + UP / DOWN nudge the selection
    nudgeSelection(newButton == BM_DOWN ? +1 : -1);
  } else if (mask == BM_ENTER) {
    // Enter Modifer
    switch (newButton) {
      case BM_DOWN:
        updateSelectionValue(VUD_DOWN);
        break;
      case BM_UP:
        updateSelectionValue(VUD_UP);
        break;
      case BM_LEFT:
        updateSelectionValue(VUD_LEFT);
        break;
      case BM_RIGHT:
        updateSelectionValue(VUD_RIGHT);
        break;
    }
  } else if (IsChord(newButton, mask, BM_ENTER, BM_ALT)) {
    cutSelection();
  } else if (IsChord(newButton, mask, BM_ENTER, BM_NAV)) {
    switchSoloMode();
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
    }
  }
}

void PhraseView::setTextProps(int col, int row, Color textColor = Theme::View::fg, int limit = 16) {
  bool highlighted = false;

  if (clipboard_.active_) {
    GUIRect selRect = getSelectionRect();
    if (selRect.Contains(GUIPoint(col, row))) {
      highlighted = true;
    }
  } else {
    if ((col_ == col) && (row_ == row)) {
      highlighted = true;
    }
  }

  SetBackgroundColor(highlighted ? textColor : Theme::View::bg);

  if (row >= limit) {
    SetColor(Theme::View::inactive);
  } else {
    SetColor(highlighted ? Theme::View::bg : textColor);
  }
}

void PhraseView::DrawEditingValue() {
  GUIPoint pos = GetAnchor();
  int phraseLength = viewData_->project_->FindVariable(Token::VarPhraseLength)->GetInt();
  PhraseStep *stepsBase = currentSteps();
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  I_Instrument *instrObj = bank->noneInstrument();

  // highlighted / active display drawn last to make sure z-order is preserved
  switch (col_) {

    // columns need special handling to draw the cmdEditField
    case colCmdVal1:
    case colCmdVal2:
      if (viewMode_ != VM_SELECTION) {
        cmdEditField_.SetFocus();
        cmdEditField_.Draw(w_);
      }
      break;

    case colNote:
      {
        pos = GetAnchor();
        pos.y_ += row_;
        unsigned char lastInstr = NO_INSTRUMENT;

        char buffer[4];

        unsigned char d = stepsBase[row_].note;
        unsigned char instr = stepsBase[row_].instrument;
        if (instr != NO_INSTRUMENT) {
          lastInstr = instr;
          instrObj = bank->GetInstrument(lastInstr);
        }

        bool hilit = row_ % phraseDiv_ == 0;
        setTextProps(colNote, row_, Theme::Phrase::note(hilit), phraseLength);

        int xStart = pos.x_ - 1;
        int xEnd = pos.x_ + 3;

        if (d == NO_NOTE) {
          DrawString(pos.x_, pos.y_, "---");
        } else if (d == NOTE_OFF) {
          DrawString(pos.x_, pos.y_, "off");
        } else {
          if (clipboard_.active_) {
            // no extra wide display in selection mode
            instrObj->noteDisplay(d, buffer);
            DrawString(pos.x_, pos.y_, buffer);
          } else {
            char buf[12];
            instrObj->focusedNoteDisplay(d, buf);

            int len = (int)strlen(buf);
            int frame = std::min(4, expandFrame_);
            int visibleLen = std::min(len, 3 + frame * 2);
            int start = (len - visibleLen) / 2;

            if (visibleLen < len)
              buf[start + visibleLen] = '\0';

            int p = (visibleLen - 3) / 2;
            xStart = pos.x_ - p - 1;
            xEnd = xStart + visibleLen + 1;

            DrawString(pos.x_ - p, pos.y_, buf + start);
          }
        }

        if (!clipboard_.active_) {
          SetColor(Theme::Phrase::note(hilit));
          SetBackgroundColor(Theme::View::bg);
          DrawChar(xStart, pos.y_, CHAR(char_cap_left_s));
          DrawChar(xEnd, pos.y_, CHAR(char_cap_right_s));
          focusRect_ = GUIRect(xStart, row_ + 3, xEnd - xStart + 1, 1);
        }
        break;
      }
    default:
      // all other columns do not break out of their assigned rects and do hence not need to be redrawn
      break;
  }
}

void PhraseView::DrawView() {
  Token helpLegendCommand = Token::InstrumentCommandNone;

  Clear();

  // Draw title

  DrawTitle("Phrase %2.2X", viewData_->currentPhrase_);

  // Compute song grid location

  GUIPoint pos = GetAnchor();

  // Draw section header

  SetColor(Theme::View::inactive);
  SetBackgroundColor(Theme::View::bg);
  DrawString(pos.x_, pos.y_ - 1, "Nte In V Cmd1Val Cmd2Val");

  // global data

  int phraseLength = viewData_->project_->FindVariable(Token::VarPhraseLength)->GetInt();
  PhraseStep *stepsBase = currentSteps();
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  I_Instrument *instrObj = bank->noneInstrument();

  // Display row numbers

  drawRowNumbers(pos.x_ - 3, pos.y_, 0, 16, phraseLength, phraseDiv_);

  // Draw all columns row by row. Per row the notes are drawn after the
  // instrument (to allow overdrawing).

  const GUIPoint anchor = GetAnchor();
  unsigned char lastInstr = NO_INSTRUMENT;
  char buffer[6];
  char noteBuffer[4];

  GUIRect selRect = getSelectionRect();

  for (int j = 0; j < 16; j++) {
    const int y = anchor.y_ + j;
    const bool hilit = j % phraseDiv_ == 0;

    // note

    bool selected = selRect.Contains({0, j});
    bool nextSelected = selRect.Contains({1, j});

    unsigned char note = stepsBase[j].note;
    unsigned char instr = stepsBase[j].instrument;
    if (instr != NO_INSTRUMENT) {
      lastInstr = instr;
      instrObj = bank->GetInstrument(lastInstr);
    }

    setTextProps(colNote, j, Theme::Phrase::note(hilit), phraseLength);

    if (note == NO_NOTE) {
      DrawString(anchor.x_ + columnPositions_[colNote], y, "---");
    } else if (note == NOTE_OFF) {
      DrawString(anchor.x_ + columnPositions_[colNote], y, "off");
    } else {
      instrObj->noteDisplay(note, noteBuffer);
      DrawString(anchor.x_ + columnPositions_[colNote], y, noteBuffer);
    }

    if (selected && nextSelected) {
      DrawChar(anchor.x_ + columnPositions_[colNote] + columnWidths_[colNote], y, ' ');
    }

    // instrument

    selected = nextSelected;
    nextSelected = selRect.Contains({2, j});

    SetBackgroundColor(Theme::View::bg);
    setTextProps(colInstrument, j, Theme::Phrase::instrument(j % phraseDiv_ == 0), phraseLength);

    unsigned char d = stepsBase[j].instrument;

    if (d == NO_INSTRUMENT) {
      DrawString(anchor.x_ + columnPositions_[colInstrument], y, "--");
    } else {
      byteToHexString(d, buffer);
      DrawString(anchor.x_ + columnPositions_[colInstrument], y, buffer);
      // todo: move outside of the loop
      if (j == row_) {
        GUIPoint location = GetTitlePosition();

        npf_snprintf(buffer, sizeof(buffer), "%2.2X:", d);
        const int x = location.x_ + 10;
        const int maxLength = INDICATOR_TITLE_VU_X;
        etl::string<maxLength> instrLine = buffer;
        I_Instrument *instr = viewData_->project_->GetInstrumentBank()->GetInstrument(d);
        instrLine += instr->GetDisplayName();

        SetBackgroundColor(Theme::View::Title::bg);
        SetColor(Theme::View::Title::fg);

        DrawString(x, location.y_, instrLine.c_str());

        if ((int)instrLine.length() >= maxLength) {
          DrawChar(maxLength - 1, location.y_, CHAR(char_indicator_ellipsis_s));
        }
      }
    }

    if (selected && nextSelected) {
      DrawChar(anchor.x_ + columnPositions_[colInstrument] + columnWidths_[colInstrument], y, ' ');
    }

    // volume

    selected = nextSelected;
    nextSelected = selRect.Contains({3, j});

    uint8_t vol = stepsBase[j].volume;
    setTextProps(colVolume, j, Theme::Phrase::volume(hilit), phraseLength);
    DrawChar(anchor.x_ + columnPositions_[colVolume], y, vol == 0xFF ? '-' : hexChars[vol & 0xF]);

    if (selected && nextSelected) {
      DrawChar(anchor.x_ + columnPositions_[colVolume] + columnWidths_[colVolume], y, ' ');
    }

    // command 1

    selected = nextSelected;
    nextSelected = selRect.Contains({4, j});

    Token command1 = Token::enum_type(stepsBase[j].cmd1);
    setTextProps(colCmd1, j, Theme::Phrase::command1(hilit), phraseLength);
    DrawString(anchor.x_ + columnPositions_[colCmd1], y, command1.c_str());
    if (j == row_ && (col_ == colCmd1 || col_ == colCmdVal1)) {
      helpLegendCommand = command1;
    }

    // command 1 param

    selected = nextSelected;
    nextSelected = selRect.Contains({5, j});

    setTextProps(colCmdVal1, j, Theme::Phrase::command1(hilit), phraseLength);
    wordToHexString(stepsBase[j].param1, buffer);
    DrawString(anchor.x_ + columnPositions_[colCmdVal1], y, buffer);

    if (selected && nextSelected) {
      DrawChar(anchor.x_ + columnPositions_[colCmdVal1] + columnWidths_[colCmdVal1], y, ' ');
    }

    // command 2

    selected = nextSelected;
    nextSelected = selRect.Contains({6, j});

    Token command2 = Token::enum_type(stepsBase[j].cmd2);
    setTextProps(colCmd2, j, Theme::Phrase::command2(hilit), phraseLength);
    DrawString(anchor.x_ + columnPositions_[colCmd2], y, command2.c_str());
    if (j == row_ && (col_ == colCmd2 || col_ == colCmdVal2)) {
      helpLegendCommand = command2;
    }

    // command 2 param

    setTextProps(colCmdVal2, j, Theme::Phrase::command2(hilit), phraseLength);
    wordToHexString(stepsBase[j].param2, buffer);
    DrawString(anchor.x_ + columnPositions_[colCmdVal2], y, buffer);
  }

  // draw the map in the bottom left corner

  drawMap();

  // Set info area draw mode based on what will be drawn. Context help trumps
  // the command help, which trumps the notes.

  if (drawContextLegendInfoArea()) {
    // the legend for the current mode is shown
  } else if (helpLegendCommand != Token::InstrumentCommandNone) {
    infoAreaMode_ = InfoAreaDrawMode::HelpLegend;
    drawCommandLegend(5, SCREEN_HEIGHT - 4, helpLegendCommand);
  } else {
    infoAreaMode_ = InfoAreaDrawMode::Notes;
    drawNotes();
  }

  if (Player::GetInstance()->IsRunning()) {
    OnPlayerUpdate(PET_UPDATE);
  }

  // redraw/overdraw editing value

  DrawEditingValue();
}

ContextLegend PhraseView::currentContextLegend() const {
  if (viewMode_ == VM_CLONE) {
    return ContextLegend::PhraseClone;
  }
  if (viewMode_ == VM_SELECTION || clipboard_.active_) {
    return ContextLegend::PhraseSelection;
  }
  return ContextLegend::None;
}

void PhraseView::AnimationUpdate() {
  // First call the parent class implementation to draw the battery gauge
  ScreenView::AnimationUpdate();

  // do not draw above the modal
  if (HasModalView()) {
    return;
  }

  // update the editing value as it might animate
  if (expandFrame_ != -1) {
    expandFrame_++;

    DrawEditingValue();
  }

  // Get player instance safely
  Player *player = Player::GetInstance();

  // Only process updates if we're fully initialized
  if (!viewData_ || !player) {
    return;
  }

  // Handle any pending updates from OnPlayerUpdate using the consolidated flag
  // This ensures all UI drawing happens on the "main" thread (core0)
  if (needsUIUpdate_) {
    // Only draw notes if help legend is not being shown (mutually exclusive)
    if (infoAreaMode_ == InfoAreaDrawMode::Notes) {
      drawNotes();
    }

    // Draw play position marker
    GUIPoint anchor = GetAnchor();
    GUIPoint pos = anchor;
    pos.x_ -= 1;

    SetBackgroundColor(Theme::View::bg);

    // Clear last played position
    pos.y_ = anchor.y_ + lastPlayingPos_;
    DrawString(pos.x_, pos.y_, " ");

    // Only update play position if player is running, the first channel that plays this phrase gives it
    int playingStep;
    bool muted;
    if (findPlayingStep(playingStep, muted)) {
      pos.y_ = anchor.y_ + playingStep;

      SetBackgroundColor(Theme::View::bg);

      if (!muted) {
        SetColor(Theme::Song::Playback::active);
        DrawString(pos.x_, pos.y_, char_indicator_position_s);
      } else {
        SetColor(Theme::Song::Playback::muted);
        DrawString(pos.x_, pos.y_, char_indicator_positionMuted_s);
      }

      lastPlayingPos_ = playingStep;
    }

    // Draw live indicators if in live mode
    if (const char *indicator = findLiveIndicator()) {
      pos = anchor;
      pos.x_ -= 1;
      SetColor(Theme::Song::Playback::live);
      DrawString(pos.x_, pos.y_, indicator);
    }

    // not ideal, TODO nILS
    DrawEditingValue();

    /*
    // Piano Rool Temp
    char black[8];
    char white[8];

    int lowest = HIGHEST_NOTE - 4 * 12;

    int notes[SONG_CHANNEL_COUNT];
    for (int c = 0; c < SONG_CHANNEL_COUNT; c++) {
      int note = player->GetChannelNote(c);
      if (note != NO_NOTE) {
        notes[c] = note;
        if (note < lowest) {
          lowest = note;
        }
      } else {
        notes[c] = -1;
      }
    }

    lowest -= (lowest % 12);

    int firstNote = lowest; // C3

    for (int octave = 0; octave < 4; ++octave) {
      uint16_t octaveNotes = 0;
      const int octaveStart = firstNote + octave * 12;

      for (int c = 0; c < SONG_CHANNEL_COUNT; ++c) {
        const int note = notes[c];

        if (note >= octaveStart && note < octaveStart + 12)
          octaveNotes |= 1u << (note - octaveStart);
      }

      // TODO nILS: placeholder for future use
      // drawPiano(octaveNotes, white, black);

      SetColor(Theme::View::bg);
      SetBackgroundColor(Theme::View::fg);
      DrawString(5 + 7 * octave, 20, black, fGraphic);
      DrawString(5 + 7 * octave, 19, white, fGraphic);
    }
    */

    // Create a memory barrier to ensure proper synchronization between cores
    createMemoryBarrier();

    // Reset the consolidated flag
    needsUIUpdate_ = false;
  }

  // Flush the window to ensure changes are displayed
}
