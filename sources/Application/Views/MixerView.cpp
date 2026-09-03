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

#include "MixerView.h"
#include "Application/Model/Mixer.h"
#include "Application/Utils/char.h"
#include "Application/Utils/mathutils.h"
#include "Application/Views/SampleEditorView.h"
#include "UIController.h"
#include <Application/AppWindow.h>
#include <string>

#define CHANNELS_X_OFFSET_ 3 // stride between each channel

MixerView::MixerView(GUIWindow &w, ViewData *viewData) : FieldView(w, viewData) {

  // Initialize the channel volume fields
  initChannelVolumeFields();
}

MixerView::~MixerView() {
}

void MixerView::Reset() {
  needsPlayTimeUpdate_ = false;
  needsNotesUpdate_ = false;
  ClearFocus();
  fieldList_.clear();
  channelVolumeFields_.clear();
  masterVolumeField_.clear();
  initChannelVolumeFields();
}

void MixerView::OnFocus() {
  // update selected field to match current cursor position
  if (viewData_->songX_ <= SONG_CHANNEL_COUNT) {
    if (viewData_->songX_ < SONG_CHANNEL_COUNT) {
      // Channel 0-7
      SetFocus((UIField *)&channelVolumeFields_.at(viewData_->songX_));
    } else {
      // Master channel
      SetFocus((UIField *)&masterVolumeField_.at(0));
    }
  }
}

void MixerView::SetFocus(UIField *field) {
  // Call parent implementation first
  FieldView::SetFocus(field);

  // Now update songX_ based on which field has focus
  if (!field)
    return;

  // Check if it's one of the channel volume fields
  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
    if (field == (UIField *)&channelVolumeFields_.at(i)) {
      viewData_->songX_ = i;
      return;
    }
  }

  // Check if it's the master volume field
  if (field == (UIField *)&masterVolumeField_.at(0)) {
    viewData_->songX_ = SONG_CHANNEL_COUNT;
  }
}

// keep track of currently selected channel
void MixerView::updateCursor(int dx, int dy) {
  int x = viewData_->songX_;
  x += dx;

  // Prevent wrapping by clamping values
  if (x < 0) {
    x = 0;
  }
  if (x > SONG_CHANNEL_COUNT) {
    x = SONG_CHANNEL_COUNT;
  }
  viewData_->songX_ = x;

  // Update field focus to match the selected channel
  if (x < SONG_CHANNEL_COUNT) {
    // Channel 0-7
    SetFocus(&channelVolumeFields_[x]);
  } else {
    // Master channel
    SetFocus(&masterVolumeField_[0]);
  }

  isDirty_ = true;
}

void MixerView::switchSoloMode() {
  UIController *controller = UIController::GetInstance();
  int currentChannel = viewData_->songX_;
  controller->SwitchSoloMode(currentChannel, currentChannel, (viewMode_ == VM_NORMAL));
  viewMode_ = (viewMode_ != VM_SOLOON) ? VM_SOLOON : VM_NORMAL;
  isDirty_ = true;
}

void MixerView::unMuteAll() {
  UIController *controller = UIController::GetInstance();
  controller->UnMuteAll();
  isDirty_ = true;
}

void MixerView::toggleMute() {

  UIController *controller = UIController::GetInstance();
  int currentChannel = viewData_->songX_;
  controller->ToggleMute(currentChannel, currentChannel);
  viewMode_ = (viewMode_ != VM_MUTEON) ? VM_MUTEON : VM_NORMAL;
  isDirty_ = true;
}

void MixerView::ProcessButtonMask(uint16_t mask, bool pressed) {
  if (!pressed) {
    if (viewMode_ == VM_MUTEON) {
      if (mask & BM_NAV) {
        toggleMute();
      }
    }
    if (viewMode_ == VM_SOLOON) {
      if (mask & BM_NAV) {
        switchSoloMode();
      }
    }
    FieldView::ProcessButtonMask(mask, pressed);
    // Force a full redraw of the mixer view
    SetDirty(true);
    return;
  }

  // Ignore up/down arrow keys when pressed by themselves in MixerView
  // We only want left/right to navigate between channels
  if (mask == BM_UP || mask == BM_DOWN) {
    return;
  }

  // Fieldview gets first go at the button event
  FieldView::ProcessButtonMask(mask, pressed);

  // Handle playback specific actions
  if (mask == BM_PLAY) {
    togglePlay();
  }
  // NAV back to Song view
  if (mask & BM_NAV) {
    if (mask & BM_UP) {
      Navigate(VT_SONG, vtRevealFromTop);
    } else if (mask & BM_RIGHT) {
      Navigate(VT_TABLE, vtRevealFromRight);
    }
  }

  if (mask == (BM_NAV & BM_ALT)) {
    unMuteAll();
  }

  // Handle mixer-specific actions for normal mode, but only if we don't have
  // field focus or if they're specific mixer actions that should override field
  // editing

  // EDIT+NAV is always for toggling mute
  if ((mask & BM_EDIT) && (mask & BM_NAV)) {
    toggleMute();
    return;
  }

  // ENTER+NAV is always for solo mode
  if ((mask & BM_ENTER) && (mask & BM_NAV)) {
    switchSoloMode();
    return;
  }

  viewMode_ = VM_NORMAL;
  // Force a full redraw of the mixer view when any button is pressed
  SetDirty(true);
  processNormalButtonMask(mask);
}

/*******************************************************************************
 processNormalButtonMask:
        process button mask in the case there is no
        selection active
******************************************************************************/

// This method is no longer needed as we're using FieldView's field navigation
void MixerView::processNormalButtonMask(unsigned int mask) {
  // Handle mixer-specific actions
  if (mask & BM_EDIT) {
    if (mask & BM_NAV) {
      toggleMute();
    }
  } else if (mask & BM_ENTER) {
    if (mask & BM_NAV) {
      switchSoloMode();
    }
  } else if (mask & BM_NAV) {
    if (mask & BM_UP) {
      Navigate(VT_SONG, vtRevealFromTop);
    }
    if (mask & BM_ALT) {
      unMuteAll();
    }
  } else {
    if (mask & BM_LEFT) {
      updateCursor(-1, 0);
    }
    if (mask & BM_RIGHT) {
      updateCursor(1, 0);
    }
  }
}

/*******************************************************************************
 processSelectionButtonMask:
        process button mask in the case there is a
        selection active
******************************************************************************/
void MixerView::processSelectionButtonMask(unsigned int mask) {

  if (mask & BM_EDIT) {

  } else {
    if (mask & BM_ENTER) {
      if (mask & BM_NAV) {
        switchSoloMode();
      }
    } else {
      if (mask & BM_NAV) {
        if (mask & BM_PLAY) {
          onStop();
        }
        if (mask & BM_ALT) {
          unMuteAll();
        }
      } else {
        // No modifier
        if (mask & BM_PLAY) {
          onStart();
        }
      }
    }
  }
}

void MixerView::initChannelVolumeFields() {
  Project *project = viewData_ ? viewData_->project_ : nullptr;

  if (!project)
    return;

  // Position for volume fields - below VU meters
  GUIPoint position = GetAnchor();
  position.y_ += VU_METER_HEIGHT + 1; // Position below VU meters

  // Get Token codes for channel volumes
  Token channelVolumeTokens[SONG_CHANNEL_COUNT] = {
      Token::VarChannel1Volume, Token::VarChannel2Volume, Token::VarChannel3Volume, Token::VarChannel4Volume,
      Token::VarChannel5Volume, Token::VarChannel6Volume, Token::VarChannel7Volume, Token::VarChannel8Volume};

  // Clear any existing fields
  channelVolumeFields_.clear();

  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
    // Create position for this channel's volume field
    GUIPoint fieldPos = position;
    fieldPos.x_ = position.x_ + (i * CHANNELS_X_OFFSET_);

    // Find the variable for this channel's volume
    Variable *v = project->FindVariable(channelVolumeTokens[i]);
    if (v) {
      // Create a 2-digit field (00-99) for the channel volume
      // NOTE: 99 is considered "unity" gain
      // Format: %2.2d = 2-digit decimal number with leading zeros
      // Use xOffset=1 and yOffset=5 for small/large increments
      channelVolumeFields_.emplace_back(fieldPos, *v, "%2.2d", 0, 99, 1, 5);

      // Add the field to the fieldList_ for proper field navigation
      fieldList_.insert(fieldList_.end(), &(*channelVolumeFields_.rbegin()));
    }
  }

  // Add master volume field to the right of channel volumes
  GUIPoint masterPos = position;
  // Position to the right of channel volumes
  masterPos.x_ += (SONG_CHANNEL_COUNT * CHANNELS_X_OFFSET_);

  Variable *v = project->FindVariable(Token::VarMasterVolume);
  if (v) {
    masterVolumeField_.emplace_back(masterPos, *v, "%2.2d", 0, 99, 1, 5);
    fieldList_.insert(fieldList_.end(), &(*masterVolumeField_.begin()));
  }

  // Set focus to the first field if we have any fields
  if (!fieldList_.empty()) {
    SetFocus(*fieldList_.begin());
  }
}

void MixerView::DrawView() {
  Clear();

  // Draw title

  Player *player = Player::GetInstance();
  Project *project = player->GetProject(); // Use Player's GetProject method
  DrawTitle(player->GetSequencerMode() == SM_SONG ? "Song" : "Live");

  // Now draw busses
  // we start at the bottom of the VU meter and draw it growing upwards
  GUIPoint anchor = GetAnchor();
  GUIPoint pos = anchor;
  pos.y_ += VU_METER_HEIGHT - 1; // -1 to align with song grid

  // Draw all fields (channel volume fields)
  FieldView::Redraw();

  // Draw mute indicators below the volume values
  pos.y_ = anchor.y_ + VU_METER_HEIGHT + 2; // Position below volume fields
  pos.x_ = GetTitlePosition().x_;

  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
    char state[2];
    state[0] = player->IsChannelMuted(i) ? 'M' : '-';
    state[1] = '\0';

    bool highlighted = (i == viewData_->songX_);
    SetBackgroundColor(Theme::View::bg);
    SetColor(Theme::View::fg);

    if (highlighted) {
      SwapColors();
    }
    DrawString(pos.x_, pos.y_, state);
    pos.x_ += CHANNELS_X_OFFSET_;
  }

  drawMap();
  drawNotes();
  drawMasterVuMeter(player);

  // Draw master volume label
  GUIPoint labelPos = GetAnchor();
  // Align with master volume control
  labelPos.x_ += (SONG_CHANNEL_COUNT * CHANNELS_X_OFFSET_);
  labelPos.y_ = SCREEN_HEIGHT - 3; // Position below the volume control

  bool active = (viewData_->songX_ == SONG_CHANNEL_COUNT);

  SetBackgroundColor(Theme::Notes::bg(active));
  SetColor(Theme::Notes::fg(active));
  DrawString(labelPos.x_, labelPos.y_, "MB");
  DrawString(labelPos.x_, labelPos.y_ + 1, "  ");
  DrawString(labelPos.x_, labelPos.y_ + 2, "  ");

  if (player->IsRunning()) {
    OnPlayerUpdate(PET_UPDATE);
  };
}

void MixerView::OnPlayerUpdate(PlayerEventType eventType, unsigned int tick) {
  // Since this can be called from core1 via the Observer pattern,
  // we need to ensure we don't try directly calling draw functions here!

  // Instead of drawing directly, we'll just update our state and let
  // AnimationUpdate handle the actual drawing
  Player *player = Player::GetInstance();

  if (eventType != PET_STOP) {
    // Flag that play time needs to be updated
    needsPlayTimeUpdate_ = true;
  }

  needsNotesUpdate_ = true;
}

void MixerView::AnimationUpdate() {
  // First call the parent class implementation to draw the battery gauge
  ScreenView::AnimationUpdate();

  // Get the player safely
  Player *player = Player::GetInstance();

  // Only process updates below if we're fully initialized
  if (!viewData_ || !player) {
    // Just flush the battery gauge and return
    return;
  }

  // Always update VU meters, whether the sequencer is running or not
  // This ensures we see VU meter updates from MIDI input even when not playing
  etl::array<stereosample, SONG_CHANNEL_COUNT> *levels = player->GetMixerLevels();
  if (levels) {
    drawChannelVUMeters(levels, player);
    drawMasterVuMeter(player);
  }

  // Handle any pending updates from OnPlayerUpdate
  // This ensures all UI drawing happens in the same thread (core0)
  if (needsPlayTimeUpdate_) {
    GUIPoint pos = GetAnchor();
    // explicitly position timer directly below the battery gauge
    pos.x_ = 27;
    pos.y_ = 1;
    drawPlayTime(player, pos);
    needsPlayTimeUpdate_ = false;
  }

  if (needsNotesUpdate_) {
    drawNotes();
    needsNotesUpdate_ = false;
  }

  // Flush the window to ensure changes are displayed
}

void MixerView::drawChannelVUMeters(etl::array<stereosample, SONG_CHANNEL_COUNT> *levels, Player *player,
                                    bool forceRedraw) {
  // Quick optimization: If not forcing redraw, check if any levels have changed
  // This saves CPU cycles by avoiding unnecessary drawing operations
  if (!forceRedraw) {
    bool anyChanges = false;
    for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
      // Convert amplitude to bar levels
      int32_t leftBars, rightBars;
      amplitudeToBars(levels->at(i), &leftBars, &rightBars);

      // Check if this channel's levels have changed
      if (leftBars != prevLeftVU_[i + 1] || rightBars != prevRightVU_[i + 1]) {
        anyChanges = true;
        break;
      }
    }

    // If no changes, return early
    if (!anyChanges) {
      return;
    }
  }

  // we start at the bottom of the VU meter and draw it growing upwards
  GUIPoint pos = GetAnchor();
  pos.y_ += VU_METER_HEIGHT - 1; // -1 to align with song grid

  // draw vu meter for each bus
  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
    int32_t leftBars = 0;
    int32_t rightBars = 0;

    // if channel is muted just use default 0 values for bars
    if (!player->IsChannelMuted(i)) {
      // Convert amplitude to bar levels
      amplitudeToBars(levels->at(i), &leftBars, &rightBars);
    }

    // Use index i+1 for channel VU meters (index 0 is reserved for master)
    drawVUMeter(leftBars, rightBars, pos, i + 1, forceRedraw);
    pos.x_ += CHANNELS_X_OFFSET_;
  }
}

void MixerView::togglePlay() {
  Player *player = Player::GetInstance();
  player->OnStartButton(PM_CHAIN, viewData_->songX_, true, viewData_->chainRow_);
}
