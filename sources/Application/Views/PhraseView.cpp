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
#include "Application/Views/ModalDialogs/MessageBox.h"
#include "Application/Views/SampleEditorView.h"
#include "System/Console/Trace.h"
#include "UIController.h"
#include "ViewData.h"
#include <Application/AppWindow.h>
#include <cstdint>
#include <etl/string.h>
#include <nanoprintf.h>
#include <stdlib.h>

int16_t PhraseView::offsets_[2][4] = {-1, 1, 12, -12, -1, 1, 16, -16};

PhraseView::PhraseView(GUIWindow &w, ViewData *viewData)
    : ScreenView(w, viewData), cmdEdit_(FourCC::ActionEdit, 0), cmdEditPos_(0, 10),
      cmdEditField_(cmdEditPos_, cmdEdit_, 4, "%4.4X", 0, 0xFFFF, 16, true) {
  phrase_ = &(viewData_->song_->phrase_);
  lastPlayingPos_ = 0;
  row_ = 0;
  viewData->phraseCurPos_ = 0;
  col_ = colNote;
  lastNote_ = NOTE_C3;
  lastInstr_ = 0;
  lastCmd_ = FourCC::InstrumentCommandNone;
  lastParam_ = 0;
  lastVolume_ = 0xFF;

  clipboard_.active_ = false;
  clipboard_.width_ = 0;
  clipboard_.height_ = 0;

  for (int i = 0; i < 16; i++) {
    clipboard_.steps_[i] = {NO_NOTE, 0, 0, 0, 0, 0, 0xFF};
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
  lastCmd_ = FourCC::InstrumentCommandNone;
  lastParam_ = 0;
  lastVolume_ = 0xFF;
  viewData_->phraseCurPos_ = 0;

  clipboard_.active_ = false;
  clipboard_.width_ = 0;
  clipboard_.height_ = 0;
  clipboard_.col_ = 0;
  clipboard_.row_ = 0;
  for (int i = 0; i < 16; i++) {
    clipboard_.steps_[i] = {0xFF, 0, 0, 0, 0, 0, 0xFF};
  }

  saveCol_ = colNote;
  saveRow_ = 0;
  needsUIUpdate_ = false;
  needsLiveIndicatorUpdate_ = false;
}

bool PhraseView::getEffectiveInstrumentForRow(int row, uint8_t &instrumentId) const {
  if (!phrase_ || row < 0) {
    return false;
  }

  for (int i = row; i >= 0; --i) {
    unsigned char instr = phrase_->steps_[viewData_->currentPhrase_][i].instrument;
    if (instr != 0xFF) {
      instrumentId = instr;
      return true;
    }
  }
  return false;
}

void PhraseView::updateCursor(int dx, int dy) {
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
      p.x_ += 12; // TODO: pick from constant array
      p.y_ += row_;
      cmdEditField_.SetPosition(p);
      cmdEdit_.SetInt(phrase_->steps_[viewData_->currentPhrase_][row_].param1);
      break;
    case colCmdVal2:
      p.x_ += 20; // TODO: pick from constant array
      p.y_ += row_;
      cmdEditField_.SetPosition(p);
      cmdEdit_.SetInt(phrase_->steps_[viewData_->currentPhrase_][row_].param2);
      break;
    default:
      break;
  }

  viewData_->phraseCurPos_ = row_;
  isDirty_ = true;
}

void PhraseView::updateCursorValue(ViewUpdateDirection direction, int xOffset, int yOffset) {

  unsigned char *c = 0;
  unsigned char limit = 0;

  bool wrap = false;

  switch (col_ + xOffset) {
    case colNote:
      c = &phrase_->steps_[viewData_->currentPhrase_][row_ + yOffset].note;
      limit = HIGHEST_NOTE;
      wrap = true;
      break;

    case colInstrument:
      c = &phrase_->steps_[viewData_->currentPhrase_][row_ + yOffset].instrument;
      limit = MAX_INSTRUMENT_COUNT - 1;
      wrap = true;
      break;

    case colVolume:
      {
        uint8_t &vol = phrase_->steps_[viewData_->currentPhrase_][row_ + yOffset].volume;
        if (direction == VUD_UP) {
          vol = 0x0F;
        } else if (direction == VUD_DOWN) {
          vol = 0xFF;
        } else if (direction == VUD_LEFT) {
          if (vol == 0) {
            // below 0 is no value
            vol = NO_VOLUME;
          } else if (vol != NO_VOLUME) {
            vol--;
          }
        } else if (direction == VUD_RIGHT) {
          if (vol == NO_VOLUME) {
            vol = 0;
          } else {
            vol = std::min(0x0F, vol + 1);
          }
        }
        lastVolume_ = vol;
        break;
      }

    case colCmd1:
      {
        PhraseStep &step = phrase_->steps_[viewData_->currentPhrase_][row_ + yOffset];
        FourCC cc = FourCC::enum_type(step.cmd1);
        switch (direction) {
          case VUD_RIGHT:
            cc = CommandList::GetNext(cc);
            break;
          case VUD_UP:
            cc = CommandList::GetNextAlpha(cc);
            break;
          case VUD_LEFT:
            cc = CommandList::GetPrev(cc);
            break;
          case VUD_DOWN:
            cc = CommandList::GetPrevAlpha(cc);
            break;
        }
        step.cmd1 = static_cast<uint8_t>(static_cast<char>(cc));
        lastCmd_ = cc;
        break;
      }

    case colCmdVal1:
      {
        switch (direction) {
          case VUD_RIGHT:
            cmdEditField_.ProcessArrow(BM_RIGHT);
            break;
          case VUD_UP:
            cmdEditField_.ProcessArrow(BM_UP);
            break;
          case VUD_LEFT:
            cmdEditField_.ProcessArrow(BM_LEFT);
            break;
          case VUD_DOWN:
            cmdEditField_.ProcessArrow(BM_DOWN);
            break;
        }
        PhraseStep &step = phrase_->steps_[viewData_->currentPhrase_][row_ + yOffset];
        FourCC currentCmd = FourCC::enum_type(step.cmd1);
        uint8_t paramValue = cmdEdit_.GetInt();
        paramValue = CommandList::RangeLimitCommandParam(currentCmd, paramValue);
        cmdEdit_.SetInt(paramValue);
        step.param1 = paramValue;
        lastParam_ = paramValue;
        break;
      }
    case colCmd2:
      {
        PhraseStep &step = phrase_->steps_[viewData_->currentPhrase_][row_ + yOffset];
        FourCC cc = FourCC::enum_type(step.cmd2);
        switch (direction) {
          case VUD_RIGHT:
            cc = CommandList::GetNext(cc);
            break;
          case VUD_UP:
            cc = CommandList::GetNextAlpha(cc);
            break;
          case VUD_LEFT:
            cc = CommandList::GetPrev(cc);
            break;
          case VUD_DOWN:
            cc = CommandList::GetPrevAlpha(cc);
            break;
        }
        step.cmd2 = static_cast<uint8_t>(static_cast<char>(cc));
        lastCmd_ = cc;
        break;
      }
    case colCmdVal2:
      {
        switch (direction) {
          case VUD_RIGHT:
            cmdEditField_.ProcessArrow(BM_RIGHT);
            break;
          case VUD_UP:
            cmdEditField_.ProcessArrow(BM_UP);
            break;
          case VUD_LEFT:
            cmdEditField_.ProcessArrow(BM_LEFT);
            break;
          case VUD_DOWN:
            cmdEditField_.ProcessArrow(BM_DOWN);
            break;
        }
        PhraseStep &step = phrase_->steps_[viewData_->currentPhrase_][row_ + yOffset];
        FourCC currentCmd = FourCC::enum_type(step.cmd2);
        uint8_t paramValue = cmdEdit_.GetInt();
        paramValue = CommandList::RangeLimitCommandParam(currentCmd, paramValue);
        cmdEdit_.SetInt(paramValue);
        step.param2 = paramValue;
        lastParam_ = paramValue;
        break;
      }
  }

  if ((c) && (*c != NO_NOTE)) {
    int offset = offsets_[col_ + xOffset][direction];

    // when changing notes from note off, always start from C3
    if (*c == NOTE_OFF) {
      *c = NOTE_C3;
      offset = 0;
    }

    // if note column apply the set scale or slice range
    if (col_ + xOffset == 0) {
      uint8_t instrId = 0;
      InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
      SampleInstrument *sliceInstr = nullptr;
      if (bank && getEffectiveInstrumentForRow(row_ + yOffset, instrId)) {
        I_Instrument *instr = bank->GetInstrument(instrId);
        if (instr && instr->GetType() == IT_SAMPLE) {
          sliceInstr = static_cast<SampleInstrument *>(instr);
        }
      }

      uint8_t sliceFirst = 0;
      uint8_t sliceLast = 0;
      if (sliceInstr && sliceInstr->GetSliceNoteRange(sliceFirst, sliceLast)) {
        int newNote = *c + offset;
        if (newNote < sliceFirst) {
          newNote = sliceFirst;
        } else if (newNote > sliceLast) {
          newNote = sliceLast;
        }
        *c = static_cast<unsigned char>(newNote);
      } else {
        // Add/remove from offset to match selected scale
        int scale = viewData_->project_->GetScale();
        int scaleRoot = viewData_->project_->GetScaleRoot();

        // Calculate the new note with the offset
        int newNote = *c + offset;

        // Check if the note is in the scale (adjusted for root)
        // For root = 0, (newNote + 12 - 0) % 12 simplifies to newNote % 12
        while (newNote >= 0 && !scaleSteps[scale][(newNote + 12 - scaleRoot) % 12]) {
          offset > 0 ? offset++ : offset--;
          newNote = *c + offset;
        }
        updateData(c, offset, limit, wrap);
      }
    } else {
      updateData(c, offset, limit, wrap);
    }

    switch (col_ + xOffset) {
      case colNote:
        lastNote_ = *c;
        // Need to restart audition to update it with the new note
        startAudition(false);
        break;

      case colInstrument:
        lastInstr_ = *c;
        break;

        // TODO: store last volume as well?

      default:
        break;
    }
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
        FourCC cmd1 = FourCC::enum_type(step.cmd1);
        if (cmd1 == FourCC::InstrumentCommandNone) {
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
        FourCC cmd2 = FourCC::enum_type(step.cmd2);
        if (cmd2 == FourCC::InstrumentCommandNone) {
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
  int newPos = viewData_->songX_ + offset;
  if ((newPos > -1) && (newPos < SONG_CHANNEL_COUNT)) {
    // Go to neighbout song channel
    viewData_->songX_ = newPos;
    unsigned char *c = viewData_->GetCurrentSongPointer();
    // is there a chain ?
    unsigned char oldChain = viewData_->currentChain_;
    if (*c != 0xFF) {
      // go to chain
      viewData_->currentChain_ = *c;
      // get phrase at location
      unsigned char *p = viewData_->GetCurrentChainPointer();
      // is there a phrase ?
      if (*p != 0xFF) {
        viewData_->currentPhrase_ = *p;
        updateCursor(0, 0);
        isDirty_ = true;
      } else { // restore chain & song
        viewData_->currentChain_ = oldChain;
        viewData_->songX_ -= offset;
      }
    } else { // restore song
      viewData_->songX_ -= offset;
    }
  }
}

/*******************************************************************************
 getSelectionRect:
        gets the normalized rectangle of the current
        selection. Valid only while selection is drawn
******************************************************************************/

GUIRect PhraseView::getSelectionRect() {
  GUIRect r(clipboard_.col_, clipboard_.row_, col_, row_);
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

void PhraseView::fillClipboardData() {

  // Get Current normalized selection rect

  GUIRect selRect = getSelectionRect();

  // Get size & store in clipboard

  clipboard_.width_ = selRect.Width() + 1;
  clipboard_.height_ = selRect.Height() + 1;
  clipboard_.row_ = selRect.Top();
  clipboard_.col_ = selRect.Left();

  // Copy the data

  PhraseStep *base = viewData_->song_->phrase_.steps_[viewData_->currentPhrase_];

  for (int i = 0; i < clipboard_.height_; i++) {
    clipboard_.steps_[i] = base[clipboard_.row_ + i];
  };
  updateCursor(0, 0);
}

void PhraseView::updateSelectionValue(ViewUpdateDirection direction) {
  saveRow_ = row_;
  saveCol_ = col_;

  GUIRect r = getSelectionRect();
  col_ = (PhraseColumn)r.Left();
  row_ = (PhraseColumn)r.Top();

  for (int i = 0; i <= r.Width(); i++) {
    for (int j = 0; j <= r.Height(); j++) {
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
  if (rect.Left() > 0 || rect.Right() < 5) {
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
/*******************************************************************************
 copySelection:
        copies data in the current selection to the
        clipboard & end selection process
******************************************************************************/

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

/*******************************************************************************
 cut:  copies data in the current selection to the
       clipboard, clear selection content & end selection
       process
******************************************************************************/

void PhraseView::cutSelection() {

  // Keep up with row,col of selection because
  // fillClipboardData will trash it

  fillClipboardData();

  // Loop over selection col, row & clear data inside it

  static const uint8_t kNone = static_cast<uint8_t>(static_cast<char>(FourCC::InstrumentCommandNone));
  PhraseStep *base = viewData_->song_->phrase_.steps_[viewData_->currentPhrase_];

  for (int i = 0; i < clipboard_.width_; i++) {
    for (int j = 0; j < clipboard_.height_; j++) {
      int r = j + clipboard_.row_;
      switch (i + clipboard_.col_) {
        case colNote:
          base[r].note = 0xFF;
          break;
        case colInstrument:
          base[r].instrument = 0xFF;
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
          base[r].volume = 0xFF;
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

/*******************************************************************************
 pasteClipboard:
        copies data in the clipboard to the current step
******************************************************************************/

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
  clipboard_.active_ = false;
  viewMode_ = VM_NORMAL;
  updateCursor(0, 0);
}

void PhraseView::ProcessButtonMask(uint16_t mask, bool pressed) {
  if (!pressed) {
    // ENTER might now no longer be pressed so first check if we were in
    // audition mode and if its not then stop auditioning, stopAudition does
    // both those things
    if (!(mask & BM_ENTER)) {
      stopAudition();
    }

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
      // If note or Instrument, we request a new instr
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
          MessageBox *mb = MessageBox::Create(*this, "No more instruments!", MBBF_OK);
          DoModal(mb);
          return;
        }
        mask &= (0xFFFF - BM_ENTER);
      } else {
        if ((col_ == 3) && FourCC::enum_type(phrase_->steps_[viewData_->currentPhrase_][row_].cmd1) ==
                               FourCC::InstrumentCommandTable) {
          TableHolder *th = TableHolder::GetInstance();
          uint16_t next = th->GetNext();
          if (next != NO_MORE_TABLE) {
            uint16_t *c = &phrase_->steps_[viewData_->currentPhrase_][row_].param1;
            *c = next;
            isDirty_ = true;
            mask &= (0xFFFF - BM_ENTER);
            cmdEdit_.SetInt(next);
          }
        }
      };
    }
  }

  if ((viewMode_ == VM_CLONE) && !((mask & BM_ENTER) && (mask & BM_ALT))) {
    viewMode_ = VM_SELECTION;
  }

  if ((mask == (BM_ALT | BM_EDIT | BM_ENTER)) || ((viewMode_ == VM_CLONE) && (mask & BM_ENTER) && (mask & BM_ALT))) {
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
          FourCC::enum_type(phrase_->steps_[viewData_->currentPhrase_][row_].cmd1) == FourCC::InstrumentCommandTable) {
        TableHolder *th = TableHolder::GetInstance();
        int current = phrase_->steps_[viewData_->currentPhrase_][row_].param1;
        if (current != -1) {
          uint16_t next = th->Clone(current);
          if (next != NO_MORE_TABLE) {
            uint16_t *c = &phrase_->steps_[viewData_->currentPhrase_][row_].param1;
          }
        }
      }
      if ((col_ == colCmdVal2) &&
          FourCC::enum_type(phrase_->steps_[viewData_->currentPhrase_][row_].cmd2) == FourCC::InstrumentCommandTable) {
        TableHolder *th = TableHolder::GetInstance();
        uint16_t next = th->Clone(phrase_->steps_[viewData_->currentPhrase_][row_].param2);
        if (next != NO_MORE_TABLE) {
          uint16_t *c = &phrase_->steps_[viewData_->currentPhrase_][row_].param2;
          *c = next;
          isDirty_ = true;
          cmdEdit_.SetInt(next);
          Trace::Log("PHRASEVIEW", "Cloned table2 -> %04x", next);
        }
      }
    };
    viewMode_ = VM_NORMAL;
    clipboard_.active_ = false;
    return;
  }

  if (viewMode_ == VM_SELECTION) {
    if (!clipboard_.active_) {
      clipboard_.active_ = true;
      clipboard_.col_ = col_;
      clipboard_.row_ = row_;
      saveCol_ = col_;
      saveRow_ = row_;
    }
    processSelectionButtonMask(mask);
  } else {
    viewMode_ = VM_NORMAL;
    processNormalButtonMask(mask);
  };
}

void PhraseView::processNormalButtonMask(uint16_t mask) {
  Player *player = Player::GetInstance();

  if (mask & BM_EDIT) {
    // EDIT Modifier
    if (mask & BM_LEFT)
      warpToNeighbour(-1);
    if (mask & BM_RIGHT)
      warpToNeighbour(1);
    if (mask & BM_UP)
      warpInChain(-1);
    if (mask & BM_DOWN)
      warpInChain(1);
    if (mask & BM_ENTER) {
      cutPosition();
    }
    if (mask & BM_ALT) {
      viewMode_ = VM_CLONE;
    }
    if (mask & BM_NAV)
      toggleMute();
  } else if (mask & BM_ENTER) {
    // ENTER Modifer
    if (mask & BM_DOWN)
      updateCursorValue(VUD_DOWN);
    if (mask & BM_UP)
      updateCursorValue(VUD_UP);
    if (mask & BM_LEFT)
      updateCursorValue(VUD_LEFT);
    if (mask & BM_RIGHT)
      updateCursorValue(VUD_RIGHT);
    if (mask & BM_ALT)
      pasteClipboard();
    if (mask & BM_NAV)
      switchSoloMode();
    if (mask == BM_ENTER) {
      pasteLast();
      if ((col_ == colInstrument) || (col_ == colCmdVal1) || (col_ == colCmdVal2)) {
        viewMode_ = VM_NEW;
      } else {
        // for note, volume and instrument:
        // Start auditionq, note stopping audition happens in
        // processButtonMask on key up
        stopAudition();
        startAudition(true);
      }
    }
  } else if (mask & BM_NAV) {
    // NAV Modifier
    if (mask & BM_LEFT) {
      Navigate(VT_CHAIN);
    } else if (mask & BM_RIGHT) {
      unsigned char *c = &phrase_->steps_[viewData_->currentPhrase_][row_].instrument;
      if (*c != 0xFF) {
        viewData_->currentInstrumentID_ = *c;
      } else {
        viewData_->currentInstrumentID_ = lastInstr_;
      }
      if (viewData_->currentInstrumentID_ != 0xFF) {
        Navigate(VT_INSTRUMENT);
      }
    } else if (mask & BM_DOWN) {
      // Go to table view
      {
        PhraseStep &step = phrase_->steps_[viewData_->currentPhrase_][row_];
        FourCC cmd1 = FourCC::enum_type(step.cmd1);
        FourCC cmd2 = FourCC::enum_type(step.cmd2);
        if (cmd1 == FourCC::InstrumentCommandTable) {
          viewData_->currentTable_ = step.param1 & (TABLE_COUNT - 1);
        } else if (cmd2 == FourCC::InstrumentCommandTable) {
          viewData_->currentTable_ = step.param2 & (TABLE_COUNT - 1);
        }
      }

      Navigate(VT_TABLE);
    } else if (mask & BM_UP) {
      // Go to groove view
      stopAudition();

      Navigate(VT_GROOVE);
    }

    if (mask & BM_PLAY) {
      player->OnStartButton(PM_PHRASE, viewData_->songX_, true, viewData_->chainRow_);
    }
    if (mask & BM_ALT)
      unMuteAll();

  } else if (mask & BM_ALT) {
    // ALT Modifier
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
      player->OnStartButton(PM_PHRASE, viewData_->songX_, false, viewData_->chainRow_);
    }
  }
}

void PhraseView::processSelectionButtonMask(uint16_t mask) {

  Player *player = Player::GetInstance();

  // B modifier

  if (mask & BM_EDIT) {
    if (mask & BM_ALT) {
      extendSelection();
    } else {
      copySelection();
    }
  } else {

    // A Modifer

    if (mask & BM_ENTER) {

      if (mask & BM_DOWN)
        updateSelectionValue(VUD_DOWN);
      if (mask & BM_UP)
        updateSelectionValue(VUD_UP);
      if (mask & BM_LEFT)
        updateSelectionValue(VUD_LEFT);
      if (mask & BM_RIGHT)
        updateSelectionValue(VUD_RIGHT);

      if (mask & BM_ALT)
        cutSelection();
      if (mask & BM_NAV)
        switchSoloMode();
    } else {

      // R Modifier

      if (mask & BM_NAV) {
        if (mask & BM_LEFT) {
          Navigate(VT_CHAIN);
        }
        if (mask & BM_RIGHT) {
          unsigned char *c = &phrase_->steps_[viewData_->currentPhrase_][row_].instrument;
          if (*c != 0xFF) {
            viewData_->currentInstrumentID_ = *c;
          } else {
            viewData_->currentInstrumentID_ = lastInstr_;
          }
          Navigate(VT_INSTRUMENT);
        }
        if (mask & BM_PLAY) {
          player->OnStartButton(PM_PHRASE, viewData_->songX_, true, viewData_->chainRow_);
        }
        if (mask & BM_ALT)
          unMuteAll();

      } else {
        // L Modifier
        if (mask & BM_ALT) {

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
            player->OnStartButton(PM_PHRASE, viewData_->songX_, false, viewData_->chainRow_);
          }
        }
      }
    }
  }
}

void PhraseView::setTextProps(int col, int row, Color textColor = Theme::View::fg) {
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

  SetColor(highlighted ? Theme::View::bg : textColor);
  SetBackgroundColor(highlighted ? textColor : Theme::View::bg);
}

void PhraseView::DrawView() {
  Clear();

  // Draw title

  DrawTitle("Phrase %2.2X", viewData_->currentPhrase_);

  // Compute song grid location

  GUIPoint pos = GetAnchor();

  // Draw section header

  SetColor(Theme::View::inactive);
  SetBackgroundColor(Theme::View::bg);
  DrawString(pos.x_, pos.y_ - 1, "Nte In V Cmd1Val Cmd2Val");

  // Display row numbers

  drawRowNumbers(pos.x_ - 3, pos.y_, 0, 16);

  // Display notes

  PhraseStep *stepsBase = phrase_->steps_[viewData_->currentPhrase_];
  unsigned char lastInstr = NO_INSTRUMENT;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();

  char buffer[6];
  buffer[3] = 0;
  for (int j = 0; j < 16; j++) {
    unsigned char d = stepsBase[j].note;
    unsigned char instr = stepsBase[j].instrument;
    if (instr != NO_INSTRUMENT) {
      lastInstr = instr;
    }

    unsigned char effectiveInstr = lastInstr;
    setTextProps(colNote, j, Theme::Phrase::note(j % ALT_ROW_NUMBER == 0));

    if (d == NO_NOTE) {
      DrawString(pos.x_, pos.y_, "---");
    } else if (d == NOTE_OFF) {
      DrawString(pos.x_, pos.y_, "off");
    } else {
      bool showSlice = false;
      bool invalidSlice = false;
      uint8_t sliceIndex = 0;
      if (effectiveInstr != 0xFF && bank) {
        I_Instrument *instrObj = bank->GetInstrument(effectiveInstr);
        if (instrObj && instrObj->GetType() == IT_SAMPLE) {
          SampleInstrument *sampleInstr = static_cast<SampleInstrument *>(instrObj);
          if (sampleInstr->HasSlicesForPlayback()) {
            if (sampleInstr->ShouldDisplaySliceForNote(d)) {
              showSlice = true;
              sliceIndex = static_cast<uint8_t>(d - SampleInstrument::SliceNoteBase);
            } else {
              invalidSlice = true;
            }
          }
        }
      }
      if (showSlice) {
        npf_snprintf(buffer, sizeof(buffer), "S%02u", static_cast<unsigned>(sliceIndex));
      } else if (invalidSlice) {
        npf_snprintf(buffer, sizeof(buffer), "S**");
      } else {
        noteToString(d, buffer);
      }
      DrawString(pos.x_, pos.y_, buffer);
    }
    pos.y_++;
  }

  // Draw instruments

  pos = GetAnchor();
  pos.x_ += 4;

  for (int j = 0; j < 16; j++) {
    SetBackgroundColor(Theme::View::bg);
    setTextProps(colInstrument, j, Theme::Phrase::instrument(j % ALT_ROW_NUMBER == 0));

    unsigned char d = stepsBase[j].instrument;

    if (d == NO_INSTRUMENT) {
      DrawString(pos.x_, pos.y_, "--");
    } else {
      byteToHexString(d, buffer);
      DrawString(pos.x_, pos.y_, buffer);
      // todo: move outside of the loop
      if (j == row_) {
        npf_snprintf(buffer, sizeof(buffer), "%2.2X:", d);
        etl::string<SCREEN_WIDTH - BATTERY_GAUGE_WIDTH> instrLine = buffer;
        GUIPoint location = GetTitlePosition();
        I_Instrument *instr = viewData_->project_->GetInstrumentBank()->GetInstrument(d);
        instrLine += instr->GetDisplayName();

        SetBackgroundColor(Theme::View::Title::bg);
        SetColor(Theme::View::Title::fg);
        DrawString(location.x_ + 10, location.y_, instrLine.c_str());
      }
    }
    pos.y_++;
  }

  // Draw volume

  pos = GetAnchor();
  pos.x_ += 7;

  for (int j = 0; j < 16; j++) {
    uint8_t vol = stepsBase[j].volume;
    setTextProps(colVolume, j, Theme::Phrase::volume(j % ALT_ROW_NUMBER == 0));
    DrawChar(pos.x_, pos.y_, vol == 0xFF ? '-' : hexChars[vol & 0xF]);
    pos.y_++;
  }

  // Draw command 1

  pos = GetAnchor();
  pos.x_ += 9;

  for (int j = 0; j < 16; j++) {
    FourCC command = FourCC::enum_type(stepsBase[j].cmd1);
    setTextProps(colCmd1, j, Theme::Phrase::command1(j % ALT_ROW_NUMBER == 0));
    DrawString(pos.x_, pos.y_, command.c_str());
    pos.y_++;
    if (j == row_ && (col_ == colCmd1 || col_ == colCmdVal1)) {
      drawHelpLegend(command);
    }
  }

  // Draw commands params 1

  pos = GetAnchor();
  pos.x_ += 12;

  buffer[5] = 0;

  for (int j = 0; j < 16; j++) {
    uint8_t p = stepsBase[j].param1;
    setTextProps(colCmdVal1, j, Theme::Phrase::command1(j % ALT_ROW_NUMBER == 0));
    wordToHexString(p, buffer);
    DrawString(pos.x_, pos.y_, buffer);
    pos.y_++;
  }

  // Draw commands 2

  pos = GetAnchor();
  pos.x_ += 17;

  for (int j = 0; j < 16; j++) {
    FourCC command = FourCC::enum_type(stepsBase[j].cmd2);
    setTextProps(colCmd2, j, Theme::Phrase::command2(j % ALT_ROW_NUMBER == 0));
    DrawString(pos.x_, pos.y_, command.c_str());
    pos.y_++;
    if (j == row_ && (col_ == colCmd2 || col_ == colCmdVal2)) {
      drawHelpLegend(command);
    }
  }

  // Draw commands params

  pos = GetAnchor();
  pos.x_ += 20;

  buffer[5] = 0;

  for (int j = 0; j < 16; j++) {
    uint8_t p = stepsBase[j].param2;
    setTextProps(colCmdVal2, j, Theme::Phrase::command2(j % ALT_ROW_NUMBER == 0));
    wordToHexString(p, buffer);
    DrawString(pos.x_, pos.y_, buffer);
    pos.y_++;
  }

  drawMap();
  drawNotes();

  if (Player::GetInstance()->IsRunning()) {
    OnPlayerUpdate(PET_UPDATE);
  }

  if ((viewMode_ != VM_SELECTION) && ((col_ == colCmdVal1) || (col_ == colCmdVal1))) {
    cmdEditField_.SetFocus();
    cmdEditField_.Draw(w_);
  }
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

  // Get player instance safely
  Player *player = Player::GetInstance();

  // Only process updates if we're fully initialized
  if (!viewData_ || !player) {
    return;
  }

  // Always update VU meter even if other parts of UI dont need updating
  drawMasterVuMeter(player, false, 25);

  // Handle any pending updates from OnPlayerUpdate using the consolidated flag
  // This ensures all UI drawing happens on the "main" thread (core0)
  if (needsUIUpdate_) {
    // Draw notes
    drawNotes();

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

    // Create a memory barrier to ensure proper synchronization between cores
    createMemoryBarrier();

    needsLiveIndicatorUpdate_ = false;

    // Reset the consolidated flag
    needsUIUpdate_ = false;
  }

  // Flush the window to ensure changes are displayed
}
