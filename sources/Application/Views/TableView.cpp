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

#include "TableView.h"
#include "Application/Instruments/CommandList.h"
#include "Application/Player/TablePlayback.h"
#include "Application/Utils/char.h"
#include "Application/Views/SampleEditorView.h"
#include "ViewData.h"
#include <nanoprintf.h>

namespace {
inline uint8_t encodeCommand(FourCC command) {
  return static_cast<uint8_t>(static_cast<char>(command));
}

inline uint8_t &getCmdRef(Table &table, int row, int col) {
  switch (col) {
    case 0:
      return table.steps_[row].cmd1;
    case 1:
      return table.steps_[row].cmd2;
    default:
      return table.steps_[row].cmd3;
  }
}

inline uint16_t &getParamRef(Table &table, int row, int col) {
  switch (col) {
    case 0:
      return table.steps_[row].param1;
    case 1:
      return table.steps_[row].param2;
    default:
      return table.steps_[row].param3;
  }
}
} // namespace

TableView::TableView(GUIWindow &w, ViewData *viewData)
    : ScreenView(w, viewData), cmdEdit_(FourCC::ActionEdit, 0), cmdEditPos_(0, 10),
      cmdEditField_(cmdEditPos_, cmdEdit_, 4, "%4.4X", 0, 0xFFFF, 16, true) {
  row_ = 0;
  col_ = 0;

  lastVol_ = 0;
  lastTick_ = 0;
  lastTsp_ = 0;
  lastCmd_ = FourCC::InstrumentCommandNone;
  lastParam_ = 0;

  clipboard_.active_ = false;
  clipboard_.width_ = 0;
  clipboard_.height_ = 0;
}

TableView::~TableView() {
}

void TableView::Reset() {
  row_ = 0;
  col_ = 0;
  lastVol_ = 0;
  lastTick_ = 0;
  lastTsp_ = 0;
  lastCmd_ = FourCC::InstrumentCommandNone;
  lastParam_ = 0;
  cmdEdit_.SetInt(0, false);

  clipboard_.active_ = false;
  clipboard_.width_ = 0;
  clipboard_.height_ = 0;
  clipboard_.col_ = 0;
  clipboard_.row_ = 0;
  for (int i = 0; i < 16; i++) {
    clipboard_.steps_[i] = {};
  }

  saveCol_ = 0;
  saveRow_ = 0;
  needsUIUpdate_ = false;
  needsPlayPositionUpdate_ = false;
  needsNotesUpdate_ = false;
  lastPosition_[0] = 0;
  lastPosition_[1] = 0;
  lastPosition_[2] = 0;
}

void TableView::OnFocus() {
  clipboard_.active_ = false;
  viewMode_ = VM_NORMAL;
  lastPosition_[0] = lastPosition_[1] = lastPosition_[2] = 0xFF;
  updateCursor(0, 0);
}

void TableView::cutPosition() {

  clipboard_.active_ = true;
  clipboard_.row_ = row_;
  clipboard_.col_ = col_;
  saveRow_ = row_;
  saveCol_ = col_;

  if ((col_ == 0) || (col_ == 2) || (col_ == 4))
    col_ += 1; // This way, A+B on note cuts
               // the instruments too and parameters get cut with commands
  cutSelection();
}

GUIRect TableView::getSelectionRect() {
  GUIRect r(clipboard_.col_, clipboard_.row_, col_, row_);
  r.Normalize();
  return r;
}

void TableView::fillClipboardData() {

  // Get Current normalized selection rect

  GUIRect selRect = getSelectionRect();

  // Get size & store in clipboard

  clipboard_.width_ = selRect.Width() + 1;
  clipboard_.height_ = selRect.Height() + 1;
  clipboard_.row_ = selRect.Top();
  clipboard_.col_ = selRect.Left();

  // Copy the data

  Table &table = TableHolder::GetInstance()->GetTable(viewData_->currentTable_);

  for (int i = 0; i < clipboard_.height_; i++) {
    clipboard_.steps_[i] = table.steps_[clipboard_.row_ + i];
  }
  updateCursor(0, 0);
}

void TableView::extendSelection() {
  GUIRect rect = getSelectionRect();
  if (rect.Left() > 0 || rect.Right() < 6) {
    if (col_ < clipboard_.col_) {
      col_ = 0;
      clipboard_.col_ = 6;
    } else {
      col_ = 6;
      clipboard_.col_ = 0;
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

void TableView::copySelection() {

  // Keep up with row,col of selection because
  // fillClipboardData will trash it

  fillClipboardData();

  clipboard_.active_ = false;
  viewMode_ = VM_NORMAL;
  row_ = saveRow_;
  col_ = saveCol_;

  isDirty_ = true;
}

void TableView::cutSelection() {

  // Keep up with row,col of selection because
  // fillClipboardData will trash it

  fillClipboardData();

  // Loop over selection col, row & clear data inside it

  Table &table = TableHolder::GetInstance()->GetTable(viewData_->currentTable_);

  for (int i = 0; i < clipboard_.width_; i++) {
    for (int j = 0; j < clipboard_.height_; j++) {
      const int row = j + clipboard_.row_;
      switch (i + clipboard_.col_) {
        case 0:
          table.steps_[row].cmd1 = encodeCommand(FourCC::InstrumentCommandNone);
          break;
        case 1:
          table.steps_[row].param1 = 0x0000;
          break;
        case 2:
          table.steps_[row].cmd2 = encodeCommand(FourCC::InstrumentCommandNone);
          break;
        case 3:
          table.steps_[row].param2 = 0x0000;
          break;
        case 4:
          table.steps_[row].cmd3 = encodeCommand(FourCC::InstrumentCommandNone);
          break;
        case 5:
          table.steps_[row].param3 = 0x0000;
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

void TableView::pasteClipboard() {

  // Get number of row to paste

  int height = clipboard_.height_;
  /*    if (row_+height>16) {
          height=16-row_ ;
      }
    */
  Table &table = TableHolder::GetInstance()->GetTable(viewData_->currentTable_);

  for (int i = 0; i < clipboard_.width_; i++) {
    for (int j = 0; j < height; j++) {
      const int row = (j + row_) % 16;
      switch (i + clipboard_.col_) {
        case 0:
          table.steps_[row].cmd1 = clipboard_.steps_[j].cmd1;
          break;
        case 1:
          table.steps_[row].param1 = clipboard_.steps_[j].param1;
          break;
        case 2:
          table.steps_[row].cmd2 = clipboard_.steps_[j].cmd2;
          break;
        case 3:
          table.steps_[row].param2 = clipboard_.steps_[j].param2;
          break;
        case 4:
          table.steps_[row].cmd3 = clipboard_.steps_[j].cmd3;
          break;
        case 5:
          table.steps_[row].param3 = clipboard_.steps_[j].param3;
          break;
      }
    }
  }
  int offset = (row_ + height) % 16 - row_;
  updateCursor(0x00, offset);
  isDirty_ = true;
}

void TableView::updateCursor(int dx, int dy) {
  col_ += dx;
  row_ += dy;
  if (col_ > 5) {
    col_ = 5;
  }
  if (col_ < 0) {
    col_ = 0;
  }
  if (row_ > 15) {
    row_ = 15;
  }
  if (row_ < 0) {
    row_ = 0;
  }
  Table &table = TableHolder::GetInstance()->GetTable(viewData_->currentTable_);

  GUIPoint p = GetAnchor();

  switch (col_) {
    case 1:
      p.x_ += 3;
      p.y_ += row_;
      cmdEditField_.SetPosition(p);
      cmdEdit_.SetInt(table.steps_[row_].param1);
      break;
    case 3:
      p.x_ += 11;
      p.y_ += row_;
      cmdEditField_.SetPosition(p);
      cmdEdit_.SetInt(table.steps_[row_].param2);
      break;
    case 5:
      p.x_ += 19;
      p.y_ += row_;
      cmdEditField_.SetPosition(p);
      cmdEdit_.SetInt(table.steps_[row_].param3);
      break;
  };

  isDirty_ = true;
}

void TableView::warpToNeighbour(int dir) {

  int current = viewData_->currentTable_ + dir;

  if (current >= TABLE_COUNT) {
    current -= TABLE_COUNT;
  }
  if (current < 0) {
    current += TABLE_COUNT;
  }
  viewData_->currentTable_ = current;
  updateCursor(0, 0);
  isDirty_ = true;
}

void TableView::updateCursorValue(int offset) {
  unsigned char *c = 0;
  unsigned char limit = 0;
  bool wrap = false;
  FourCC *cc;

  Table &table = TableHolder::GetInstance()->GetTable(viewData_->currentTable_);

  switch (col_) {
    case 0:
    case 2:
    case 4:
      {
        const int commandColumn = col_ / 2;
        FourCC command = table.getCmd(row_, commandColumn);
        switch (offset) {
          case 0x01:
            command = CommandList::GetNext(command);
            if (command == FourCC::InstrumentCommandTable) {
              command = CommandList::GetNext(command);
            }
            break;
          case 0x10:
            command = CommandList::GetNextAlpha(command);
            if (command == FourCC::InstrumentCommandTable) {
              command = CommandList::GetNextAlpha(command);
            }
            break;
          case -0x01:
            command = CommandList::GetPrev(command);
            if (command == FourCC::InstrumentCommandTable) {
              command = CommandList::GetPrev(command);
            }
            break;
          case -0x10:
            command = CommandList::GetPrevAlpha(command);
            if (command == FourCC::InstrumentCommandTable) {
              command = CommandList::GetPrevAlpha(command);
            }
            break;
        }
        getCmdRef(table, row_, commandColumn) = encodeCommand(command);
        lastCmd_ = command;
        break;
      }

    case 1:
    case 3:
    case 5:
      {
        switch (offset) {
          case 0x01:
            cmdEditField_.ProcessArrow(BM_RIGHT);
            break;
          case 0x10:
            cmdEditField_.ProcessArrow(BM_UP);
            break;
          case -0x01:
            cmdEditField_.ProcessArrow(BM_LEFT);
            break;
          case -0x10:
            cmdEditField_.ProcessArrow(BM_DOWN);
            break;
        }
        const int commandColumn = col_ / 2;
        FourCC currentCmd = table.getCmd(row_, commandColumn);
        uint16_t paramValue = cmdEdit_.GetInt();
        paramValue = CommandList::RangeLimitCommandParam(currentCmd, paramValue);
        cmdEdit_.SetInt(paramValue);
        getParamRef(table, row_, commandColumn) = paramValue;
        lastParam_ = paramValue;
        break;
      }
  }
  isDirty_ = true;
}

void TableView::pasteLast() {
  Table &table = TableHolder::GetInstance()->GetTable(viewData_->currentTable_);

  switch (col_) {
    case 0:
    case 2:
    case 4:
      {
        const int commandColumn = col_ / 2;
        uint8_t &command = getCmdRef(table, row_, commandColumn);
        if (command == encodeCommand(FourCC::InstrumentCommandNone)) {
          command = static_cast<uint8_t>(lastCmd_);
          isDirty_ = true;
        } else {
          lastCmd_ = table.getCmd(row_, commandColumn);
        }
        break;
      }

    case 1:
    case 3:
    case 5:
      break;
  }
}

void TableView::ProcessButtonMask(uint16_t mask, bool pressed) {

  if (!pressed) {
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
    processNormalButtonMask(mask);
  };
}

void TableView::processNormalButtonMask(uint16_t mask) {

  Player *player = Player::GetInstance();

  if (mask & BM_EDIT) {
    if (mask & BM_LEFT)
      warpToNeighbour(-1);
    if (mask & BM_RIGHT)
      warpToNeighbour(+1);
    if (mask & BM_DOWN)
      warpToNeighbour(-16);
    if (mask & BM_UP)
      warpToNeighbour(16);
    if (mask & BM_ENTER)
      cutPosition();
    if (mask & BM_ALT)
      viewMode_ = VM_SELECTION;
  } else if (mask & BM_ENTER) {
    // ENTER modifier
    if (mask & BM_DOWN)
      updateCursorValue(-0x10);
    else if (mask & BM_UP)
      updateCursorValue(0x10);
    else if (mask & BM_LEFT)
      updateCursorValue(-0x01);
    else if (mask & BM_RIGHT)
      updateCursorValue(0x01);
    else if (mask == BM_ENTER)
      pasteLast();
    else if (mask & BM_ALT)
      pasteClipboard();
  } else if (mask & BM_NAV) {
    // NAV Modifier
    if (mask & BM_UP) {
      Navigate((viewType_ == VT_TABLE ? VT_PHRASE : VT_INSTRUMENT));
    } else if (mask & BM_LEFT) {
      Navigate(viewType_ == VT_TABLE ? VT_MIXER : VT_TABLE);
    } else if (mask & BM_RIGHT) {
      if (viewType_ == VT_TABLE) {
        Navigate(VT_TABLE2);
      }
    }
    if (mask & BM_PLAY) {
      player->OnStartButton(PM_PHRASE, viewData_->songX_, true, viewData_->chainRow_);
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
      player->OnStartButton(PM_PHRASE, viewData_->songX_, false, viewData_->chainRow_);
    }
  }
}

void TableView::processSelectionButtonMask(uint16_t mask) {

  Player *player = Player::GetInstance();

  if (mask & BM_EDIT) {
    if (mask & BM_ALT) {
      extendSelection();
    } else {
      copySelection();
    }
  } else {

    // A Modifer

    if (mask & BM_ENTER) {
      if (mask & BM_ALT)
        cutSelection();
      //		if (mask&BM_R) switchSoloMode() ;
    } else {

      // R Modifier

      if (mask & BM_NAV) {
        if (mask & BM_UP) {
          Navigate(VT_PHRASE);
        }
        if (mask & BM_PLAY) {
          player->OnStartButton(PM_PHRASE, viewData_->songX_, true, viewData_->chainRow_);
        }
        /*			if (mask&BM_L) unMuteAll() ;
         */
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

void TableView::setTextProps(int row, int col, Color color = Theme::View::fg) {
  bool highlighted = false;

  if (clipboard_.active_) {
    GUIRect selRect = getSelectionRect();
    if ((row >= selRect.Left()) && (row <= selRect.Right()) && (col >= selRect.Top()) && (col <= selRect.Bottom())) {
      highlighted = true;
    }
  } else {
    if ((col_ == row) && (row_ == col)) {
      highlighted = true;
    }
  }

  SetColor(highlighted ? Theme::View::bg : color);
  SetBackgroundColor(highlighted ? color : Theme::View::bg);
}

void TableView::DrawView() {
  Clear();

  // Draw title

  Table &table = TableHolder::GetInstance()->GetTable(viewData_->currentTable_);
  DrawTitle("Table %2.2X", viewData_->currentTable_);

  // Compute song grid location

  GUIPoint anchor = GetAnchor();

  // Draw section header

  SetColor(Theme::View::inactive);
  SetBackgroundColor(Theme::View::bg);
  DrawString(anchor.x_, anchor.y_ - 1, "Cmd1Val Cmd2Val Cmd3Val");

  // Display row numbers
  drawRowNumbers(anchor.x_ - 3, anchor.y_, 0, 16);

  // Draw command 1
  GUIPoint pos = anchor;

  for (int j = 0; j < 16; j++) {
    FourCC command = table.getCmd(j, 0);
    setTextProps(0, j, Theme::Phrase::command1(j % ALT_ROW_NUMBER == 0));
    DrawString(pos.x_, pos.y_, command.c_str());
    pos.y_++;
    if (j == row_ && (col_ == 0 || col_ == 1)) {
      drawHelpLegend(command);
    }
  }

  // Draw commands params 1

  pos = anchor;
  pos.x_ += 3;

  char buffer[6];
  buffer[5] = 0;

  for (int j = 0; j < 16; j++) {
    uint16_t p = table.getParam(j, 0);
    setTextProps(1, j, Theme::Phrase::command1(j % ALT_ROW_NUMBER == 0));
    byteToHexString(p, buffer);
    DrawString(pos.x_, pos.y_, buffer);
    pos.y_++;
  }

  // Draw commands 2

  pos = anchor;
  pos.x_ += 8;

  for (int j = 0; j < 16; j++) {
    FourCC command = table.getCmd(j, 1);
    setTextProps(2, j, Theme::Phrase::command2(j % ALT_ROW_NUMBER == 0));
    DrawString(pos.x_, pos.y_, command.c_str());
    pos.y_++;
    if (j == row_ && (col_ == 2 || col_ == 3)) {
      drawHelpLegend(command);
    }
  }

  // Draw commands params

  pos = anchor;
  pos.x_ += 11;

  buffer[5] = 0;

  for (int j = 0; j < 16; j++) {
    uint16_t p = table.getParam(j, 1);
    setTextProps(3, j, Theme::Phrase::command2(j % ALT_ROW_NUMBER == 0));
    byteToHexString(p, buffer);
    DrawString(pos.x_, pos.y_, buffer);
    pos.y_++;
  }

  // Draw command 3

  pos = anchor;
  pos.x_ += 16;

  for (int j = 0; j < 16; j++) {
    FourCC command = table.getCmd(j, 2);
    setTextProps(4, j, Theme::Phrase::command3(j % ALT_ROW_NUMBER == 0));
    DrawString(pos.x_, pos.y_, command.c_str());
    pos.y_++;
    if (j == row_ && (col_ == 4 || col_ == 5)) {
      drawHelpLegend(command);
    }
  }

  // Draw commands params 3

  pos = anchor;
  pos.x_ += 19;

  buffer[5] = 0;

  for (int j = 0; j < 16; j++) {
    uint16_t p = table.getParam(j, 2);
    setTextProps(5, j, Theme::Phrase::command3(j % ALT_ROW_NUMBER == 0));
    byteToHexString(p, buffer);
    DrawString(pos.x_, pos.y_, buffer);
    pos.y_++;
  }

  if ((viewMode_ != VM_SELECTION) && ((col_ == 1) || (col_ == 3) || (col_ == 5))) {
    cmdEditField_.SetFocus();
    cmdEditField_.Draw(w_);
  };

  drawMap();
  drawNotes();

  Player *player = Player::GetInstance();

  if (player->IsRunning()) {
    OnPlayerUpdate(PET_UPDATE);
  };
}

void TableView::OnPlayerUpdate(PlayerEventType eventType, unsigned int tick) {
  // Since this can be called from core1 via the Observer pattern,
  // we just need to set the update flag and let AnimationUpdate
  // handle the actual drawing on the main thread
  needsUIUpdate_ = true;
  needsNotesUpdate_ = true;
  needsPlayPositionUpdate_ = true;

  // Create a memory barrier to ensure changes are visible across cores
  createMemoryBarrier();
}

void TableView::AnimationUpdate() {
  // First call the parent class implementation to draw the battery gauge
  ScreenView::AnimationUpdate();

  // Get player instance safely
  Player *player = Player::GetInstance();
  TableHolder *th = TableHolder::GetInstance();

  // Only process updates if we're fully initialized
  if (!viewData_ || !player || !th) {
    return;
  }

  // Handle any pending updates from OnPlayerUpdate using the consolidated flag
  // This ensures all UI drawing happens on the "main" thread (core0)
  if (needsUIUpdate_) {

    // Draw notes
    drawNotes();

    // Get anchor position for drawing
    GUIPoint anchor = GetAnchor();
    GUIPoint pos = anchor;

    // Clear all cursor columns first (positions 0, 9, 18 from anchor)
    SetBackgroundColor(Theme::View::bg);

    for (int i = 0; i < 3; i++) {
      pos.x_ = anchor.x_ - 1 + (i * 9);
      for (int row = 0; row < 16; row++) {
        pos.y_ = anchor.y_ + row;
        DrawString(pos.x_, pos.y_, " ");
      }
    }

    // Only update play position if player is running
    if (player->IsRunning()) {
      // Get current channel
      int channel = viewData_->songX_;
      TablePlayback &tpb = TablePlayback::GetTablePlayback(channel);
      TablePlayback &atp = TablePlayback::GetAutomationPlayback(channel);
      Table &viewTable = th->GetTable(viewData_->currentTable_);

      if (viewData_->playMode_ != PM_AUDITION) {
        SetBackgroundColor(Theme::View::bg);
        SetColor(Theme::Song::Playback::active);

        if (tpb.GetTable() == &viewTable) {
          for (int i = 0; i < 3; i++) {
            int yPos = tpb.GetPlaybackPosition(i);
            if (yPos >= 0 && yPos < 16) {
              pos.x_ = anchor.x_ - 1 + (i * 9);
              pos.y_ = anchor.y_ + yPos;
              DrawString(pos.x_, pos.y_, char_indicator_position_s);
            }
          }
        }
        if (atp.GetTable() == &viewTable) {
          for (int i = 0; i < 3; i++) {
            int yPos = atp.GetPlaybackPosition(i);
            if (yPos >= 0 && yPos < 16) {
              pos.x_ = anchor.x_ - 1 + (i * 9);
              pos.y_ = anchor.y_ + yPos;
              DrawString(pos.x_, pos.y_, char_indicator_position_s);
            }
          }
        }
      }
    }

    // Create a memory barrier to ensure proper synchronization between cores
    createMemoryBarrier();

    // Reset the consolidated flag
    needsUIUpdate_ = false;
  }

  // Flush the window to ensure changes are displayed
}
