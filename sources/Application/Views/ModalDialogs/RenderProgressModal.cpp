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

#include "RenderProgressModal.h"
#include "Application/AppWindow.h"
#include "Application/Player/Player.h"
#include "Application/Views/BaseClasses/View.h"
#include "Foundation/Constants/GraphicCharacters.h"
#include "Foundation/Constants/SpecialCharacters.h"
#include "Services/Audio/Audio.h"
#include "UIFramework/BasicDatas/GUIPoint.h"
#include <cstdint>
#include <new>
#include <stdio.h>
#include <nanoprintf.h>

static const char *messages[20] = {
  "Cooking" char_indicator_ellipsis_s,
  "Blending" char_indicator_ellipsis_s,
  "Converting" char_indicator_ellipsis_s,
  "Polishing" char_indicator_ellipsis_s,
  "Fine-tuning" char_indicator_ellipsis_s,
  "Balancing" char_indicator_ellipsis_s,
  "Processing" char_indicator_ellipsis_s,
  "Assembling" char_indicator_ellipsis_s,
  "Combining" char_indicator_ellipsis_s,
  "Mixing tracks" char_indicator_ellipsis_s,
  "Shaping sound" char_indicator_ellipsis_s,
  "Adding shine" char_indicator_ellipsis_s,
  "One more pass" char_indicator_ellipsis_s,
  "Making magic" char_indicator_ellipsis_s,
  "Building mix" char_indicator_ellipsis_s,
  "Exporting" char_indicator_ellipsis_s,
  "Wrapping up" char_indicator_ellipsis_s,
  "Finalizing" char_indicator_ellipsis_s,
  "Almost there" char_indicator_ellipsis_s,
  "Finishing up" char_indicator_ellipsis_s,
};

bool RenderProgressModal::inUse_ = false;
alignas(RenderProgressModal) static unsigned char RenderProgressModalStorage[sizeof(RenderProgressModal)];
void *RenderProgressModal::storage_ = RenderProgressModalStorage;

RenderProgressModal *RenderProgressModal::Create(View &view, const char *title, const char *message,
                                                 ProgressDisplayMode progressDisplayMode) {
  if (inUse_) {
    auto *existing = reinterpret_cast<RenderProgressModal *>(storage_);
    existing->~RenderProgressModal();
    inUse_ = false;
  }
  inUse_ = true;
  return new (storage_) RenderProgressModal(view, title, message, progressDisplayMode);
}

RenderProgressModal::RenderProgressModal(View &view, const char *title, const char *message,
                                         ProgressDisplayMode progressDisplayMode)
    : ModalView(view), title_(title), message_(message), totalSamples_(0.0f),
      progressDisplayMode_(progressDisplayMode) {
}

RenderProgressModal::~RenderProgressModal() {
}

void RenderProgressModal::Destroy() {
  this->~RenderProgressModal();
  inUse_ = false;
}

void RenderProgressModal::DrawView() {
  // Calculate window size
  // SetWindow(dialogWidth_,  dialogHeight_);
  const int x = (SCREEN_WIDTH - dialogWidth_) / 2;
  const int y = (SCREEN_HEIGHT - dialogHeight_) / 2;

  DrawWindow(x, y, dialogWidth_, dialogHeight_, title_.c_str());

  // draw progress
  DrawFilledBorder(x + 1, y + 6, dialogWidth_ - 2, 3, BLACK, true);
}

void RenderProgressModal::OnPlayerUpdate(PlayerEventType eventType, unsigned int currentTick) {
  (void)eventType;
  (void)currentTick;
}

void RenderProgressModal::OnFocus() {
}

void RenderProgressModal::ProcessButtonMask(uint16_t mask, bool pressed) {
    if (!pressed) {
        return;
    }

  if (mask & BM_ENTER) {
    // If player is still running, stop it first
    Player *player = Player::GetInstance();
    if (player && player->IsRunning()) {
      player->Stop();
    }
    // Always allow closing the modal, whether rendering is complete or not
    EndModal(0);
    return; // Return early to prevent setting dirty flag unnecessarily
  }
    
  // Only set dirty if we didn't handle the button press
  isDirty_ = true;
}

void RenderProgressModal::AnimationUpdate() {
  // This runs on core0 (UI thread). Keep updating progress every UI tick
  // while rendering so the dialog stays responsive even if PET_UPDATE events
  // are sparse during stems.
  Player *player = Player::GetInstance();
  const bool isRunning = player && player->IsRunning();

  if (isRunning) {
    renderStarted_ = true;
    if (progressDisplayMode_ == ProgressDisplayMode::pdmElapsedTime) {
      totalSamples_ = player->GetPlayTime() * Audio::GetInstance()->GetSampleRate();
    }
    // calculate the percentage progress of the song we have rendered
  bool hasActiveRow = false;
  const int currentRow = getCurrentRenderedSongRow(&hasActiveRow);
  if (hasActiveRow) {
    if (!startSongRowCaptured_) {
      startSongRow_ = currentRow;
      startSongRowCaptured_ = true;
      initializeSongProgressTracking();
    }
    if (progressChannel_ >= 0) {
      const int renderedUnits = calculateChannelRenderedUnits(progressChannel_, startSongRow_);
      if (renderedUnits > renderedUnits_) {
        renderedUnits_ = renderedUnits;
      }

    percentDone_ = calculateSongRenderPercent();
  }
    }
  } else if (renderStarted_ && !renderComplete_) {
    renderComplete_ = true;
      percentDone_ = 100;
    message_ = "Render complete!";
  }

  const int x = (SCREEN_WIDTH - dialogWidth_) / 2;
  const int y = (SCREEN_HEIGHT - dialogHeight_) / 2;

  // draw progress
  drawRenderProgress(x + 2, y + 7);

  // action button
  DrawButton(x + 6, y + 10, renderComplete_ ? "  OK  " : "Cancel", true, Theme::Dialog::bg);
    focusRect_ = GUIRect(x + 6, y + 10, 8, 1);

    // Draw message
    SetColor(Theme::Dialog::fg);
    SetBackgroundColor(Theme::Dialog::bg);
    char buf[16];
    if (renderComplete_) {
      npf_snprintf(buf, sizeof(buf), "%s", "Rendering done. ");
    } else {
      memset(buf, ' ', 16);
      npf_snprintf(buf, sizeof(buf), "%-16.16s%", messages[percentDone_ / 5]);
    }
    DrawString(x + 2, y + 4, buf);
}

void RenderProgressModal::drawRenderProgress(int x, int y) {
  animationFrame_++;

  char buf[16];
  SetColor(WHITE);
  SetBackgroundColor(BLACK);
  horizontal_bar_graph_10(buf, percentDone_);
  DrawString(x, y, buf);

  if (progressDisplayMode_ == ProgressDisplayMode::pdmPercentage) {
    sprintf(buf, "  %3d%%", percentDone_);
  } else {
    // Calculate time in seconds from total samples
    uint8_t seconds = static_cast<uint8_t>(totalSamples_ / Audio::GetInstance()->GetSampleRate());
    uint8_t minutes = seconds / 60;
    seconds %= 60;
    sprintf(buf, " %02d:%02d", minutes, seconds);
  }
  
  DrawString(x + 10, y, buf);
}

int RenderProgressModal::getCurrentRenderedSongRow(bool *hasActive) const {
  if (hasActive != nullptr) {
    *hasActive = false;
  }
  if (viewData_ == nullptr) {
    return 0;
  }

  int currentRow = 0;
  bool foundActive = false;
  for (int channel = 0; channel < SONG_CHANNEL_COUNT; channel++) {
    if (viewData_->currentPlayPhrase_[channel] == EMPTY_SONG_VALUE) {
      continue;
    }
    const int row = viewData_->songPlayPos_[channel];
    if (!foundActive || row > currentRow) {
      currentRow = row;
      foundActive = true;
    }
  }

  if (hasActive != nullptr) {
    *hasActive = foundActive;
  }

  if (currentRow < 0) {
    currentRow = 0;
  } else if (currentRow >= SONG_ROW_COUNT) {
    currentRow = SONG_ROW_COUNT - 1;
  }
  return currentRow;
}

int RenderProgressModal::getChainPhraseCount(int songRow, int channel) const {
  if (viewData_ == nullptr || viewData_->song_ == nullptr) {
    return 0;
  }
  if (songRow < 0 || songRow >= SONG_ROW_COUNT || channel < 0 || channel >= SONG_CHANNEL_COUNT) {
    return 0;
  }

  const Song *song = viewData_->song_;
  const unsigned char chain = song->rows_[songRow].chains[channel];
  if (chain == EMPTY_SONG_VALUE) {
    return 0;
  }

  int phraseCount = 0;
  for (int i = 0; i < PHRASES_PER_CHAIN; i++) {
    if (song->chain_.steps_[chain][i].phrase == EMPTY_SONG_VALUE) {
      break;
    }
    phraseCount++;
  }
  return phraseCount;
}

int RenderProgressModal::calculateChannelTotalRenderUnits(int channel, int startSongRow) const {
  if (startSongRow < 0 || startSongRow >= SONG_ROW_COUNT) {
    return 0;
  }

  int totalUnits = 0;
  for (int row = startSongRow; row < SONG_ROW_COUNT; row++) {
    const int phraseCount = getChainPhraseCount(row, channel);
    if (phraseCount <= 0) {
      break;
    }
    totalUnits += phraseCount * STEPS_PER_PHRASE;

    if (row + 1 >= SONG_ROW_COUNT || getChainPhraseCount(row + 1, channel) <= 0) {
      break;
    }
  }

  return totalUnits;
}

int RenderProgressModal::calculateChannelRenderedUnits(int channel, int startSongRow) const {
  if (startSongRow < 0 || startSongRow >= SONG_ROW_COUNT || viewData_ == nullptr) {
    return 0;
  }
  if (channel < 0 || channel >= SONG_CHANNEL_COUNT) {
    return 0;
  }

  int currentSongRow = viewData_->songPlayPos_[channel];
  if (currentSongRow < startSongRow) {
    return 0;
  }

  int renderedUnits = 0;
  for (int row = startSongRow; row < currentSongRow && row < SONG_ROW_COUNT; row++) {
    const int phraseCount = getChainPhraseCount(row, channel);
    if (phraseCount <= 0) {
      return renderedUnits;
    }
    renderedUnits += phraseCount * STEPS_PER_PHRASE;
  }

  if (currentSongRow >= SONG_ROW_COUNT) {
    return totalRenderUnits_;
  }

  const int currentPhraseCount = getChainPhraseCount(currentSongRow, channel);
  if (currentPhraseCount <= 0) {
    return renderedUnits;
  }

  int chainPos = viewData_->chainPlayPos_[channel];
  if (chainPos < 0) {
    chainPos = 0;
  } else if (chainPos >= currentPhraseCount) {
    chainPos = currentPhraseCount - 1;
  }

  int phrasePos = viewData_->phrasePlayPos_[channel];
  if (phrasePos < 0) {
    phrasePos = 0;
  } else if (phrasePos >= STEPS_PER_PHRASE) {
    phrasePos = STEPS_PER_PHRASE - 1;
  }

  renderedUnits += chainPos * STEPS_PER_PHRASE + phrasePos;
  if (renderedUnits > totalRenderUnits_) {
    renderedUnits = totalRenderUnits_;
  }
  return renderedUnits;
}

void RenderProgressModal::initializeSongProgressTracking() {
  progressChannel_ = -1;
  totalRenderUnits_ = 1;
  renderedUnits_ = 0;

  int bestTotalUnits = 0;
  for (int channel = 0; channel < SONG_CHANNEL_COUNT; channel++) {
    const int totalUnits = calculateChannelTotalRenderUnits(channel, startSongRow_);
    if (totalUnits <= 0) {
      continue;
    }
    if (progressChannel_ < 0 || totalUnits < bestTotalUnits) {
      progressChannel_ = channel;
      bestTotalUnits = totalUnits;
    }
  }

  if (progressChannel_ >= 0 && bestTotalUnits > 0) {
    totalRenderUnits_ = bestTotalUnits;
  }
}

int RenderProgressModal::calculateSongRenderPercent() const {
  if (renderComplete_) {
    return 100;
  }
  if (!renderStarted_ || !startSongRowCaptured_ || progressChannel_ < 0) {
    return 0;
  }

  int totalUnits = totalRenderUnits_;
  if (totalUnits <= 0) {
    totalUnits = 1;
  }

  int renderedUnits = renderedUnits_;
  if (renderedUnits < 0) {
    renderedUnits = 0;
  } else if (renderedUnits > totalUnits) {
    renderedUnits = totalUnits;
  }

  int percent = (renderedUnits * 100) / totalUnits;
  if (percent > 99) {
    percent = 99;
  }

  return percent;
}
