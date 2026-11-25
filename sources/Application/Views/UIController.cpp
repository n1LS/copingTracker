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

#include "UIController.h"

#include "Application/Player/Player.h"
#include "Application/Views/BaseClasses/View.h"

UIController::UIController() {};

void UIController::Init(Project *project, ViewData *viewData) {
  viewData_ = viewData;
  project_ = project;
}

void UIController::Reset() {
  viewData_ = 0;
  project_ = 0;
  armed_ = Armed::None;
  soloActive_ = false;
}

void UIController::UnMuteAll() {

  Player *player = Player::GetInstance();
  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
    player->SetChannelMute(i, false);
  };
  // nothing is left to give back
  soloActive_ = false;
  armed_ = Armed::None;
}

void UIController::toggleMute(int from, int to) {

  Player *player = Player::GetInstance();
  for (int i = from; i < to + 1; i++) {
    bool muted = player->IsChannelMuted(i);
    player->SetChannelMute(i, !muted);
  };
}

// the channels from..to play, the others are muted
void UIController::solo(int from, int to) {
  Player *player = Player::GetInstance();
  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
    soloMask_[i] = player->IsChannelMuted(i);
    player->SetChannelMute(i, (i < from) || (i > to));
  };
  soloActive_ = true;
}

void UIController::endSolo() {
  Player *player = Player::GetInstance();
  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
    player->SetChannelMute(i, soloMask_[i]);
  };
  soloActive_ = false;
}

void UIController::BeginMute(int from, int to) {
  toggleMute(from, to);
  armed_ = Armed::Mute;
  armedFrom_ = from;
  armedTo_ = to;
}

void UIController::BeginSolo(int from, int to) {
  if (soloActive_) {
    // a solo that was kept: this is the way out
    endSolo();
    armed_ = Armed::None;
    return;
  }
  solo(from, to);
  armed_ = Armed::Solo;
}

void UIController::OnButtonUp(uint16_t button, uint16_t mask) {
  if (armed_ == Armed::None) {
    return;
  }
  if (button == BM_NAV) {
    // NAV first: it stays
    armed_ = Armed::None;
    return;
  }
  const uint16_t chordButton = (armed_ == Armed::Mute) ? BM_EDIT : BM_ENTER;
  if (button == chordButton && (mask & BM_NAV)) {
    if (armed_ == Armed::Mute) {
      toggleMute(armedFrom_, armedTo_);
    } else {
      endSolo();
    }
    armed_ = Armed::None;
  }
}
