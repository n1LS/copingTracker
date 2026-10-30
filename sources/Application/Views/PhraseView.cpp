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
#include "Application/Utils/char.h"
#include "Application/Views/CommandView.h"
#include "Application/Views/ModalDialogs/MessageBox.h"
#include "Application/Views/SampleEditorView.h"
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

static const int16_t offsets_[4] = {-1, 1, 16, -16};
static const uint8_t columnPositions_[7] = {0, 4, 7, 9, 12, 17, 20};
static const uint8_t columnWidths_[7] = {3, 2, 1, 3, 4, 3, 4};

// static callback handlers

static void SetCommandCallback(View &v, ModalView &dialog) {
  PhraseView &view = (PhraseView &)v;
  view.setCurrentlySelectedCommand(Token::enum_type(dialog.GetReturnCode()));
}

void PhraseView::setCurrentlySelectedCommand(Token command) {
  PhraseStep &step = phrase_->steps_[viewData_->currentPhrase_][row_];

  if (col_ != colCmd1 && col_ != colCmd2) {
    return;
  }

  uint8_t *cmd = (col_ == colCmd1) ? &step.cmd1 : &step.cmd2;
  *cmd = command.raw();
  lastCmd_ = *cmd;

  if (command == Token::InstrumentCommandNone) {
    // clear the parameter as well
    uint16_t *value = (col_ == colCmd1) ? &step.param1 : &step.param2;
    *value = 0;
  }

  SetDirty(true);
}

// Column-specific value update handlers

void PhraseView::updateNoteValue(ViewUpdateDirection direction, int yOffset) {
  unsigned char *c = &phrase_->steps_[viewData_->currentPhrase_][row_ + yOffset].note;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  uint8_t instrumentId = getEffectiveInstrumentForRow(row_ + yOffset);
  I_Instrument *instrument = bank->GetInstrument(instrumentId);

  bool smallChange = (direction == VUD_LEFT || direction == VUD_RIGHT);
  if (instrument) {
    // some instruments want to manually set the change to the note
    int sign = (direction == VUD_LEFT || direction == VUD_DOWN) ? -1 : 1;
    if (instrument->AffectNote(c, sign, smallChange)) {
      lastNote_ = *c;
      startAudition(false);
      return;
    }
  }

  // Get the offset based on direction (using proper sequential indices)
  int offset = smallChange ? 1 : 12;
  if (direction == VUD_DOWN || direction == VUD_LEFT) {
    offset *= -1;
  }

  // when changing notes from note off, always start from C3
  if (*c == NOTE_OFF) {
    *c = NOTE_C3;
    offset = 0;
  }

  uint8_t sliceFirst = 0;
  uint8_t sliceLast = 0;

  // attempt to get the sample instrument for slice display
  SampleInstrument *sampleInstrument = nullptr;
  if (instrument && instrument->GetType() == IT_SAMPLE) {
    sampleInstrument = (SampleInstrument *)instrument;
  }

  // TODO nILS: this should also happen in SampleInstrument's AffectNote instead of getting special treatmnt here
  if (sampleInstrument && sampleInstrument->GetSliceNoteRange(sliceFirst, sliceLast)) {
    // Apply scale or slice range constraints
    int newNote = *c + offset;

    if (newNote < sliceFirst) {
      newNote = sliceFirst;
    } else if (newNote > sliceLast) {
      newNote = sliceLast;
    }
    *c = static_cast<unsigned char>(newNote);
  } else {
    int scale = viewData_->project_->GetScale();
    int scaleRoot = viewData_->project_->GetScaleRoot();

    // Calculate the new note with the offset
    int newNote = *c + offset;

    // Check if the note is in the scale (adjusted for root)
    if (instrument && instrument->SupportsScales()) {
      // Add/remove from offset to match selected scale
      while (newNote >= 0 && !scaleSteps[scale][(newNote + 12 - scaleRoot) % 12]) {
        offset > 0 ? offset++ : offset--;
        newNote = *c + offset;
      }
    }

    updateData(c, offset, instrument ? instrument->GetHighestNote() : HIGHEST_NOTE, true);
  }

  lastNote_ = *c;
  // Need to restart audition to update it with the new note
  startAudition(false);
}

void PhraseView::updateInstrumentValue(ViewUpdateDirection direction, int yOffset) {
  unsigned char *c = &phrase_->steps_[viewData_->currentPhrase_][row_ + yOffset].instrument;
  updateData(c, offsets_[direction], MAX_INSTRUMENT_COUNT - 1, false);
  lastInstr_ = *c;
  // restart audition so it picks up the new instrument
  startAudition(false);
}

void PhraseView::updateVolumeValue(ViewUpdateDirection direction, int yOffset) {
  uint8_t &vol = phrase_->steps_[viewData_->currentPhrase_][row_ + yOffset].volume;

  switch (direction) {
    case VUD_UP:
      vol = 0x0F;
      break;
    case VUD_DOWN:
      vol = 0xFF;
      break;
    case VUD_LEFT:
      if (vol == 0) {
        // below 0 is no value
        vol = NO_VOLUME;
      } else if (vol != NO_VOLUME) {
        vol--;
      }
      break;
    case VUD_RIGHT:
      if (vol == NO_VOLUME) {
        vol = 0;
      } else {
        vol = std::min(0x0F, vol + 1);
      }
      break;
  }
  lastVolume_ = vol;
  // restart audition so it picks up the new volume
  startAudition(false);
}

void PhraseView::updateCommandValue(PhraseColumn col, ViewUpdateDirection direction, int yOffset) {
  PhraseStep &step = phrase_->steps_[viewData_->currentPhrase_][row_ + yOffset];
  Token token = (col == colCmd1) ? Token::enum_type(step.cmd1) : Token::enum_type(step.cmd2);

  switch (direction) {
    case VUD_LEFT:
      token = CommandList::GetPrev(token);
      break;
    case VUD_RIGHT:
      token = CommandList::GetNext(token);
      break;
    case VUD_UP:
      token = CommandList::GetNextAlpha(token);
      break;
    case VUD_DOWN:
      token = CommandList::GetPrevAlpha(token);
      break;
  }

  if (col == colCmd1) {
    step.cmd1 = static_cast<uint8_t>(static_cast<char>(token));
  } else {
    step.cmd2 = static_cast<uint8_t>(static_cast<char>(token));
  }
  lastCmd_ = token;
}

void PhraseView::updateCommandParam(PhraseColumn col, ViewUpdateDirection direction, int yOffset) {
  PhraseStep &step = phrase_->steps_[viewData_->currentPhrase_][row_ + yOffset];

  // pick correct input and output for the selected column
  Token currentCmd = (col == colCmdVal1) ? Token::enum_type(step.cmd1) : Token::enum_type(step.cmd2);
  uint16_t *target = (col == colCmdVal1) ? &step.param1 : &step.param2;

  cmdEditField_.ProcessArrow(DirectionalButtons[direction]);
  uint16_t paramValue = cmdEdit_.GetInt();
  paramValue = CommandList::RangeLimitCommandParam(currentCmd, paramValue);
  cmdEdit_.SetInt(paramValue);

  *target = paramValue;
  lastParam_ = paramValue;
}

PhraseView::PhraseView(GUIWindow &w, ViewData *viewData)
    : ScreenView(w, viewData), cmdEdit_(Token::ActionEdit, 0), cmdEditPos_(0, 10),
      cmdEditField_(cmdEditPos_, cmdEdit_, 4, "%4.4X", 0, 0xFFFF, 16, true) {
  phrase_ = &(viewData_->song_->phrase_);
  lastPlayingPos_ = 0;
  row_ = 0;
  viewData->phraseCurPos_ = 0;
  col_ = colNote;
  lastNote_ = NOTE_C3;
  lastInstr_ = 0;
  lastCmd_ = Token::InstrumentCommandNone;
  lastParam_ = 0;
  lastVolume_ = NO_VOLUME;

  clipboard_.active_ = false;
  clipboard_.width_ = 0;
  clipboard_.height_ = 0;

  for (int i = 0; i < 16; i++) {
    clipboard_.steps_[i] = {NO_NOTE, 0, 0, 0, 0, 0, NO_VOLUME};
  };
}

PhraseView::~PhraseView() {};

void PhraseView::Reset() {
  phrase_ = &(viewData_->song_->phrase_);
  lastPlayingPos_ = 0;
  row_ = 0;
  col_ = colNote;
  lastNote_ = NOTE_C3;
  lastInstr_ = 0;
  lastCmd_ = Token::InstrumentCommandNone;
  lastParam_ = 0;
  lastVolume_ = NO_VOLUME;
  viewData_->phraseCurPos_ = 0;

  clipboard_.active_ = false;
  clipboard_.width_ = 0;
  clipboard_.height_ = 0;
  clipboard_.col_ = 0;
  clipboard_.row_ = 0;

  for (int i = 0; i < 16; i++) {
    clipboard_.steps_[i] = {NO_NOTE, 0, 0, 0, 0, 0, NO_VOLUME};
  }

  saveCol_ = colNote;
  saveRow_ = 0;
  needsUIUpdate_ = false;
  needsLiveIndicatorUpdate_ = false;
}

uint8_t PhraseView::getEffectiveInstrumentForRow(int row) {
  if (!phrase_ || row < 0) {
    return NO_INSTRUMENT;
  }

  for (int i = row; i >= 0; --i) {
    unsigned char instr = phrase_->steps_[viewData_->currentPhrase_][i].instrument;
    if (instr != 0xFF) {
      return instr;
    }
  }
  return NO_INSTRUMENT;
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
      cmdEdit_.SetInt(phrase_->steps_[viewData_->currentPhrase_][row_].param1);
      break;
    case colCmdVal2:
      p.x_ += columnPositions_[colCmdVal2];
      p.y_ += row_;
      cmdEditField_.SetPosition(p);
      cmdEdit_.SetInt(phrase_->steps_[viewData_->currentPhrase_][row_].param2);
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

void PhraseView::pasteLast() {

  switch (col_) {
    case colNote:
      {
        PhraseStep &step = phrase_->steps_[viewData_->currentPhrase_][row_];
        if (step.note == NO_NOTE) {
          step.note = lastNote_;
          step.instrument = lastInstr_;
          isDirty_ = true;
        } else {
          lastNote_ = step.note;
          lastInstr_ = step.instrument;
        }
        break;
      }
    case colInstrument:
      {
        uint8_t &instr = phrase_->steps_[viewData_->currentPhrase_][row_].instrument;
        if (instr == 0xFF) {
          instr = lastInstr_;
          isDirty_ = true;
        } else {
          lastInstr_ = instr;
        }
        break;
      }
    case colVolume:
      {
        uint8_t &vol = phrase_->steps_[viewData_->currentPhrase_][row_].volume;
        if (vol == 0xFF) {
          if (lastVolume_ != 0xFF) {
            vol = lastVolume_;
            isDirty_ = true;
          }
        } else {
          lastVolume_ = vol;
        }
        break;
      }

    case colCmd1:
      {
        PhraseStep &step = phrase_->steps_[viewData_->currentPhrase_][row_];
        Token cmd1 = Token::enum_type(step.cmd1);
        if (cmd1 == Token::InstrumentCommandNone) {
          step.cmd1 = static_cast<uint8_t>(static_cast<char>(lastCmd_));
          isDirty_ = true;
        } else {
          lastCmd_ = cmd1;
        }
        break;
      }
    case colCmdVal1:
      // TODO check if this is not needed
      /*			s=phrase_->param1_+(16*viewData_->currentPhrase_+row_) ;
      if (*s==0) {
      *s=lastParam_ ;
      cmdEdit_.SetInt(lastParam_) ;
      isDirty_=true ;
      }
      �*/
      break;

    case colCmd2:
      {
        PhraseStep &step = phrase_->steps_[viewData_->currentPhrase_][row_];
        Token cmd2 = Token::enum_type(step.cmd2);
        if (cmd2 == Token::InstrumentCommandNone) {
          step.cmd2 = static_cast<uint8_t>(static_cast<char>(lastCmd_));
          isDirty_ = true;
        } else {
          lastCmd_ = cmd2;
        }
        break;
      }
    case colCmdVal2:
      // TODO check if this is not needed
      /*			s=phrase_->param2_+(16*viewData_->currentPhrase_+row_) ;
      if (*s==0) {
      *s=lastParam_ ;
      isDirty_=true ;
      cmdEdit_.SetInt(lastParam_) ;
      }
      */
      break;
  }
}

void PhraseView::cutPosition() {
  // cutting an empty note slot adds a note off
  uint8_t *note = &phrase_->steps_[viewData_->currentPhrase_][row_].note;
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

void PhraseView::warpInChain(int offset) {

  int currentRow = viewData_->chainRow_;
  viewData_->chainRow_ += offset;
  if ((viewData_->chainRow_ < 16) && (viewData_->chainRow_ >= 0)) {
    unsigned char *p = viewData_->GetCurrentChainPointer();
    if (*p != 0xFF) {
      viewData_->currentPhrase_ = *p;
      switch (col_) {
        case colCmdVal1:
          cmdEdit_.SetInt(phrase_->steps_[viewData_->currentPhrase_][row_].param1);
          break;
        case colCmdVal2:
          cmdEdit_.SetInt(phrase_->steps_[viewData_->currentPhrase_][row_].param2);
          break;
        default:
          break;
      }
    } else { // rollback
      viewData_->chainRow_ = currentRow;
    }
  } else { // rollback
    viewData_->chainRow_ = currentRow;
  }
  isDirty_ = true;
}

void PhraseView::warpToNeighbour(int offset) {
  const int oldX = viewData_->songX_;
  const unsigned char oldChain = viewData_->currentChain_;

  // keep going in the direction of offset over empty channels (no chain, or no
  // phrase on the current chain row) until a phrase is found, else stay put
  for (int x = oldX + offset; x >= 0 && x < SONG_CHANNEL_COUNT; x += offset) {
    viewData_->songX_ = x;

    unsigned char *c = viewData_->GetCurrentSongPointer();
    if (*c == 0xFF) {
      continue;
    }

    viewData_->currentChain_ = *c;

    unsigned char *p = viewData_->GetCurrentChainPointer();
    if (*p == 0xFF) {
      continue;
    }

    viewData_->currentPhrase_ = *p;
    updateCursor(0, 0);
    isDirty_ = true;
    return;
  }

  viewData_->songX_ = oldX;
  viewData_->currentChain_ = oldChain;
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

  PhraseStep *base = viewData_->song_->phrase_.steps_[viewData_->currentPhrase_];

  for (int i = 0; i < clipboard_.height_; i++) {
    clipboard_.steps_[i] = base[clipboard_.row_ + i];
  }

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
  PhraseStep *base = viewData_->song_->phrase_.steps_[viewData_->currentPhrase_];

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

  // Get number of row to paste

  int height = clipboard_.height_;

  PhraseStep *base = viewData_->song_->phrase_.steps_[viewData_->currentPhrase_];

  for (int i = 0; i < clipboard_.width_; i++) {
    for (int j = 0; j < height; j++) {
      int r = (j + row_) % 16;
      switch (i + clipboard_.col_) {
        case colNote:
          base[r].note = clipboard_.steps_[j].note;
          break;
        case colInstrument:
          base[r].instrument = clipboard_.steps_[j].instrument;
          break;
        case colCmd1:
          base[r].cmd1 = clipboard_.steps_[j].cmd1;
          break;
        case colCmdVal1:
          base[r].param1 = clipboard_.steps_[j].param1;
          break;
        case colCmd2:
          base[r].cmd2 = clipboard_.steps_[j].cmd2;
          break;
        case colCmdVal2:
          base[r].param2 = clipboard_.steps_[j].param2;
          break;
        case colVolume:
          base[r].volume = clipboard_.steps_[j].volume;
          break;
      }
    }
  }
  int offset = (row_ + height) % 16 - row_;
  updateCursor(0x00, offset);
  isDirty_ = true;
}

inline void PhraseView::startAudition(bool startIfNotRunning) {
  // no instrument -> no audition
  if (getEffectiveInstrumentForRow(row_) == NO_INSTRUMENT) {
    return;
  }

  Player *player = Player::GetInstance();
  if (player->IsRunning()) {
    // now also update if in auditioning mode
    if (viewData_->playMode_ == PM_AUDITION) {
      player->Stop();
      player->OnStartButton(PM_AUDITION, viewData_->songX_, false, viewData_->chainRow_);
    }
  } else if (startIfNotRunning) {
    player->OnStartButton(PM_AUDITION, viewData_->songX_, false, viewData_->chainRow_);
  }
}

inline void PhraseView::stopAudition() {
  Player *player = Player::GetInstance();
  if (viewData_->playMode_ == PM_AUDITION) {
    player->Stop();
  }
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
  phraseDiv_ = viewData_->project_->FindVariable(Token::VarPhraseDivision)->GetInt();
  clipboard_.active_ = false;
  viewMode_ = VM_NORMAL;
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

bool PhraseView::navigationDown(uint16_t direction) {
  switch (direction) {
    case BM_LEFT:
      Navigate(VT_CHAIN, vtRevealFromLeft);
      return true;
    case BM_RIGHT:
      enterInstrumentUnderCursor();
      return true;
    case BM_DOWN:
      {
        // Go to table view
        PhraseStep &step = phrase_->steps_[viewData_->currentPhrase_][row_];
        Token cmd1 = Token::enum_type(step.cmd1);
        Token cmd2 = Token::enum_type(step.cmd2);
        if (cmd1 == Token::InstrumentCommandTable) {
          viewData_->currentTable_ = step.param1 & (TABLE_COUNT - 1);
        } else if (cmd2 == Token::InstrumentCommandTable) {
          viewData_->currentTable_ = step.param2 & (TABLE_COUNT - 1);
        }

        Navigate(VT_TABLE, vtRevealFromBottom);
        return true;
      }
    case BM_UP:
      // Go to groove view
      stopAudition();

      Navigate(VT_GROOVE, vtRevealFromTop);
      return true;
  }
  return false;
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
          unsigned char *c = &phrase_->steps_[viewData_->currentPhrase_][row_].instrument;
          *c = (unsigned char)next;
          lastInstr_ = next;
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
        if ((col_ == 3) &&
            Token::enum_type(phrase_->steps_[viewData_->currentPhrase_][row_].cmd1) == Token::InstrumentCommandTable) {
          TableHolder *th = TableHolder::GetInstance();
          uint16_t next = th->GetNext();
          if (next != NO_MORE_TABLE) {
            uint16_t *c = &phrase_->steps_[viewData_->currentPhrase_][row_].param1;
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
      unsigned char *c = &phrase_->steps_[viewData_->currentPhrase_][row_].instrument;
      if (*c != 0xFF) {
        uint16_t next = bank->Clone(*c);
        if (next != NO_MORE_INSTRUMENT) {
          *c = (unsigned char)next;
          lastInstr_ = next;
          isDirty_ = true;
        }
      }
    } else {
      if ((col_ == colCmdVal1) &&
          Token::enum_type(phrase_->steps_[viewData_->currentPhrase_][row_].cmd1) == Token::InstrumentCommandTable) {
        TableHolder *th = TableHolder::GetInstance();
        int current = phrase_->steps_[viewData_->currentPhrase_][row_].param1;
        if (current != -1) {
          th->Clone(current);
        }
      } else if ((col_ == colCmdVal2) && Token::enum_type(phrase_->steps_[viewData_->currentPhrase_][row_].cmd2) ==
                                             Token::InstrumentCommandTable) {
        TableHolder *th = TableHolder::GetInstance();
        uint16_t next = th->Clone(phrase_->steps_[viewData_->currentPhrase_][row_].param2);
        if (next != NO_MORE_TABLE) {
          isDirty_ = true;
          cmdEdit_.SetInt(next);
          Trace::Log("PHRASEVIEW", "Cloned table2 -> %04x", next);
        }
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

void PhraseView::enterInstrumentUnderCursor() {
  unsigned char *c = &phrase_->steps_[viewData_->currentPhrase_][row_].instrument;
  viewData_->currentInstrumentID_ = (*c != 0xFF) ? *c : lastInstr_;
  if (viewData_->currentInstrumentID_ != 0xFF) {
    Navigate(VT_INSTRUMENT, vtRevealFromRight);
  }
}

void PhraseView::processNormalButtonMask(uint16_t newButton, uint16_t mask) {
  Player *player = Player::GetInstance();

  if (IsChord(newButton, mask, BM_ENTER)) {
    // enter only (check if picker for commands is enabled)
    bool picker = Config::GetInstance()->FindVariable(Token::VarConfigCommandPicker)->GetInt();

    if (picker) {
      if (col_ == colCmd1 || col_ == colCmd2) {
        PhraseStep &step = phrase_->steps_[viewData_->currentPhrase_][row_];
        uint8_t cmd = (col_ == colCmd1) ? step.cmd1 : step.cmd2;

        // use lastCmd if we hit an empty command field
        if (cmd == Token::InstrumentCommandNone) {
          cmd = lastCmd_;
        }

        CommandView *cv = CommandView::Create(*this, Token::enum_type(cmd));
        DoModal(cv, ModalViewCallback::create<&SetCommandCallback>());
      }
    }

    // ENTER alone
    pasteLast();
    if ((col_ == colInstrument) || (col_ == colCmdVal1) || (col_ == colCmdVal2)) {
      viewMode_ = VM_NEW;
    }
    if (col_ == colNote || col_ == colInstrument || col_ == colVolume) {
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
  } else if (mask == BM_EDIT) {
    // EDIT Modifier
    switch (newButton) {
      case BM_LEFT:
        warpToNeighbour(-1);
        break;
      case BM_RIGHT:
        warpToNeighbour(1);
        break;
      case BM_UP:
        warpInChain(-1);
        break;
      case BM_DOWN:
        warpInChain(1);
        break;
    }
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
        player->OnStartButton(PM_PHRASE, viewData_->songX_, false, viewData_->chainRow_);
        break;
    }
  }
}

void PhraseView::processSelectionButtonMask(uint16_t newButton, uint16_t mask) {

  Player *player = Player::GetInstance();

  if (IsChord(newButton, mask, BM_EDIT)) {
    copySelection();
  } else if (IsPair(newButton, mask, BM_EDIT, BM_ALT)) {
    extendSelection();
  } else if (IsPair(newButton, mask, BM_NAV, BM_ALT)) {
    unMuteAll();
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
  } else if (mask == BM_NAV) {
    switch (newButton) {
      case BM_PLAY:
        player->OnStartButton(PM_PHRASE, viewData_->songX_, true, viewData_->chainRow_);
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
        player->OnStartButton(PM_PHRASE, viewData_->songX_, false, viewData_->chainRow_);
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
  PhraseStep *stepsBase = phrase_->steps_[viewData_->currentPhrase_];
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
  PhraseStep *stepsBase = phrase_->steps_[viewData_->currentPhrase_];
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
      DrawString(anchor.x_, y, "---");
    } else if (note == NOTE_OFF) {
      DrawString(anchor.x_, y, "off");
    } else {
      instrObj->noteDisplay(note, noteBuffer);
      DrawString(anchor.x_, y, noteBuffer);
    }

    if (selected && nextSelected) {
      DrawChar(anchor.x_ + 3, y, ' ');
    }

    // instrument

    selected = nextSelected;
    nextSelected = selRect.Contains({2, j});

    SetBackgroundColor(Theme::View::bg);
    setTextProps(colInstrument, j, Theme::Phrase::instrument(j % phraseDiv_ == 0), phraseLength);

    unsigned char d = stepsBase[j].instrument;

    if (d == NO_INSTRUMENT) {
      DrawString(anchor.x_ + 4, y, "--");
    } else {
      byteToHexString(d, buffer);
      DrawString(anchor.x_ + 4, y, buffer);
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
      DrawChar(anchor.x_ + 6, y, ' ');
    }

    // volume

    selected = nextSelected;
    nextSelected = selRect.Contains({3, j});

    uint8_t vol = stepsBase[j].volume;
    setTextProps(colVolume, j, Theme::Phrase::volume(hilit), phraseLength);
    DrawChar(anchor.x_ + 7, y, vol == 0xFF ? '-' : hexChars[vol & 0xF]);

    if (selected && nextSelected) {
      DrawChar(anchor.x_ + 8, y, ' ');
    }

    // command 1

    selected = nextSelected;
    nextSelected = selRect.Contains({4, j});

    Token command1 = Token::enum_type(stepsBase[j].cmd1);
    setTextProps(colCmd1, j, Theme::Phrase::command1(hilit), phraseLength);
    DrawString(anchor.x_ + 9, y, command1.c_str());
    if (j == row_ && (col_ == colCmd1 || col_ == colCmdVal1)) {
      helpLegendCommand = command1;
    }

    // command 1 param

    selected = nextSelected;
    nextSelected = selRect.Contains({5, j});

    setTextProps(colCmdVal1, j, Theme::Phrase::command1(hilit), phraseLength);
    wordToHexString(stepsBase[j].param1, buffer);
    DrawString(anchor.x_ + 12, y, buffer);

    if (selected && nextSelected) {
      DrawChar(anchor.x_ + 16, y, ' ');
    }

    // command 2

    selected = nextSelected;
    nextSelected = selRect.Contains({6, j});

    Token command2 = Token::enum_type(stepsBase[j].cmd2);
    setTextProps(colCmd2, j, Theme::Phrase::command2(hilit), phraseLength);
    DrawString(anchor.x_ + 17, y, command2.c_str());
    if (j == row_ && (col_ == colCmd2 || col_ == colCmdVal2)) {
      helpLegendCommand = command2;
    }

    // command 2 param

    setTextProps(colCmdVal2, j, Theme::Phrase::command2(hilit), phraseLength);
    wordToHexString(stepsBase[j].param2, buffer);
    DrawString(anchor.x_ + 20, y, buffer);
  }

  // draw the map in the bottom left corner

  drawMap();

  // Set info area draw mode based on what will be drawn

  if (helpLegendCommand != Token::InstrumentCommandNone) {
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

void PhraseView::OnPlayerUpdate(PlayerEventType eventType, unsigned int tick) {
  // Since this can be called from core1 via the Observer pattern,
  // we need to ensure we don't call any drawing functions directly
  // Instead of drawing directly, we'll just update our state and let
  // AnimationUpdate handle the actual drawing

  // Set the consolidated flag for UI updates
  needsUIUpdate_ = true;

  // Update the play position for use in AnimationUpdate
  Player *player = Player::GetInstance();
  if (player && player->GetSequencerMode() == SM_LIVE) {
    needsLiveIndicatorUpdate_ = true;
  }
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

    // Only update play position if player is running
    if (player->IsRunning()) {
      // Loop on all channels to see if one of them is playing current phrase
      for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
        if (player->IsChannelPlaying(i)) {
          if (viewData_->currentPlayPhrase_[i] == viewData_->currentPhrase_ && viewData_->playMode_ != PM_AUDITION) {
            pos.y_ = anchor.y_ + viewData_->phrasePlayPos_[i];

            SetBackgroundColor(Theme::View::bg);

            if (!player->IsChannelMuted(i)) {
              SetColor(Theme::Song::Playback::active);
              DrawString(pos.x_, pos.y_, char_indicator_position_s);
            } else {
              SetColor(Theme::Song::Playback::muted);
              DrawString(pos.x_, pos.y_, char_indicator_positionMuted_s);
            }

            lastPlayingPos_ = viewData_->phrasePlayPos_[i];
            break;
          }
        }
      }
    }

    // Draw live indicators if in live mode
    if (player->GetSequencerMode() == SM_LIVE) {
      pos = anchor;
      pos.x_ -= 1;
      SetColor(Theme::Song::Playback::live);

      for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
        if (player->GetQueueingMode(i) != QM_NONE) {
          // find the chain queued in channel
          unsigned char songPos = player->GetQueuePosition(i);
          unsigned char *chain = viewData_->song_->rows_[songPos].chains + i;
          if (*chain == viewData_->currentChain_) {
            const char *indicator = player->GetLiveIndicator(i);
            DrawString(pos.x_, pos.y_, indicator);
            break;
          }
        }
      }
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

    needsLiveIndicatorUpdate_ = false;

    // Reset the consolidated flag
    needsUIUpdate_ = false;
  }

  // Flush the window to ensure changes are displayed
}
