/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "CommandView.h"

#include "Application/AppWindow.h"
#include "Application/Instruments/CommandList.h"

const int commandsPerRow = 6;

bool CommandView::inUse_ = false;
alignas(CommandView) static unsigned char CommandViewStorage[sizeof(CommandView)];
void *CommandView::storage_ = CommandViewStorage;

CommandView *CommandView::Create(View &view, Token command) {
  if (inUse_) {
    auto *existing = reinterpret_cast<CommandView *>(storage_);
    existing->~CommandView();
    inUse_ = false;
  }
  inUse_ = true;
  return new (storage_) CommandView(view, command);
}

CommandView::~CommandView() {
}

void CommandView::Destroy() {
  this->~CommandView();
  inUse_ = false;
}

void CommandView::OnFocus() {
}

CommandView::CommandView(View &view, Token command) : ModalView(view) {
  SetCommand(command);
}

void CommandView::DrawView() {
  int top = 2;
  DrawWindow(1, top, 30, 18, "Command Selector");

  DrawFilledBorder(2, top + 3, 28, 8, Theme::Dialog::Panel::sub, true);

  for (int cmd = 0; cmd < CommandList::CommandCount; cmd++) {
    bool active = cmd == index_;
    Token command = CommandList::AllCommands[cmd];

    int x = 4 + (cmd % commandsPerRow) * 4;
    int y = top + 4 + (cmd / commandsPerRow);
    
    if (active) {
      SetColor(Theme::Dialog::Button::bg(true));
      SetBackgroundColor(Theme::Dialog::Panel::sub);
      DrawChar(x - 1, y, CHAR(char_button_border_left_s));
      DrawChar(x + 3, y, CHAR(char_button_border_right_s));

      SetColor(Theme::Dialog::Button::fg(true));
      SetBackgroundColor(Theme::Dialog::Button::bg(true));
    } else {
      SetColor(Theme::View::fg);
      SetBackgroundColor(Theme::Dialog::Panel::sub);
    }

    DrawString(x, y, command.c_str());
  }

  DrawFilledBorder(2, top + 11, 28, 6, Theme::Dialog::Panel::bg, true);
  drawCommandLegend(3, top + 12, CommandList::AllCommands[index_]);
}

void CommandView::ProcessSelection(uint16_t mask) {
  if (mask & BM_RIGHT) {
    index_ = (index_ + 1) % CommandList::CommandCount;
  } else if (mask & BM_LEFT) {
    index_ = (index_ + CommandList::CommandCount - 1) % CommandList::CommandCount;
  } else if (mask & BM_DOWN) {
    index_ = (index_ + commandsPerRow) % CommandList::CommandCount;
  } else if (mask & BM_UP) {
    index_ = (index_ + CommandList::CommandCount - commandsPerRow) % CommandList::CommandCount;
  }

  SetDirty(true);
}

void CommandView::ButtonUp(uint16_t button, uint16_t mask) {
  if (mask == 0) {
    // all keys up, return
    EndModal(GetCommmand().raw());
  }
}

void CommandView::ButtonDown(uint16_t newButton, uint16_t mask) {
  // directional navigation through the commands. The picker is opened while
  // ENTER is held, so other held buttons are deliberately ignored here.
  if (newButton & BM_DIRECTIONAL) {
    ProcessSelection(newButton);
  } else if (newButton == BM_EDIT) { // ENTER shows the modal -> + EDIT means delete
    EndModal(Token::InstrumentCommandNone);
  }
}

void CommandView::SetCommand(Token command) {
  for (int c = 0; c < CommandList::CommandCount; c++) {
    if (CommandList::AllCommands[c] == command) {
      index_ = c;
      break;
    }
  }
}

Token CommandView::GetCommmand() {
  return CommandList::AllCommands[index_];
}

void CommandView::OnPlayerUpdate(PlayerEventType, unsigned int currentTick) {
}