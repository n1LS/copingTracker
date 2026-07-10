/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "CommandView.h"

#include "Application/Instruments/CommandList.h"
#include "Application/AppWindow.h"

const int commandsPerRow = 5;

bool CommandView::inUse_ = false;
alignas(CommandView) static unsigned char CommandViewStorage[sizeof(CommandView)];
void *CommandView::storage_ = CommandViewStorage;

CommandView *CommandView::Create(View &view, FourCC command) {
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

CommandView::CommandView(View &view, FourCC command): ModalView(view) {
  SetCommand(command);
}

void CommandView::DrawView() {
  int top = 3;
  DrawWindow(2, top, 28, 16, "Command Selector");
  
  for (int cmd = 0; cmd < CommandList::CommandCount; cmd++) {
    bool active = cmd == index_;
    FourCC command = CommandList::AllCommands[cmd];

    SetColor(active ? Theme::Dialog::Button::fg(true) : Theme::Dialog::fg);
    SetBackgroundColor(active ? Theme::Dialog::Button::bg(true) : Theme::Dialog::bg);

    int x = 5 + (cmd % commandsPerRow) * 5;
    int y = top + 3 + (cmd / commandsPerRow);
    
    DrawString(x, y, command.c_str());

    if (active) {
      SetColor(Theme::Dialog::Button::bg(true));
      SetBackgroundColor(Theme::Dialog::bg);
    }
     
    DrawChar(x - 1, y, active ? CHAR(char_button_border_left_s) : ' ');
    DrawChar(x + 3, y, active ? CHAR(char_button_border_right_s) : ' ');
  }

  drawCommandLegend(3, top + 11, CommandList::AllCommands[index_]);
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

void CommandView::ProcessButtonMask(uint16_t mask, bool pressed) {
  if (!pressed && mask == 0) {
    // all keys up, return
    EndModal(GetCommmand().raw());
  }

  // directional navigation through the commands
  if (mask & BM_DIRECTIONAL) {
    ProcessSelection(mask);
  }
}

void CommandView::SetCommand(FourCC command) {
  for (int c = 0; c < CommandList::CommandCount; c++) {
    if (CommandList::AllCommands[c] == command) {
      index_ = c;
      break;
    }
  }
}

FourCC CommandView::GetCommmand() {
  return CommandList::AllCommands[index_];
}

void CommandView::OnPlayerUpdate(PlayerEventType, unsigned int currentTick) {
}