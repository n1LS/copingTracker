/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#include "RecordView.h"
#include "Application/Model/Config.h"
#include "Application/Persistency/PersistenceConstants.h"
#include "Application/Views/SampleEditorView.h"
#include "System/Console/Trace.h"
#include "System/FileSystem/FileSystem.h"
#include "System/System/System.h"
#include "UIController.h"
#include "ViewData.h"

#include "Adapters/picoTracker/audio/record.h"

static constexpr uint32_t kMaxRecordDurationMs = 30000;

// Initialize static member
ViewType RecordView::sourceViewType_ = VT_SONG;

RecordView::RecordView(GUIWindow &w, ViewData *data) : FieldView(w, data) {

  GUIPoint position = GetAnchor();

  auto config = Config::GetInstance();

  // Initialize recording state
  uiRecordingActive_ = false;
  uiSavingActive_ = false;
  autoSwitchPending_ = false;
  recordingStartTime_ = 0;
  recordingDuration_ = 0;

  // Audio source selection field (Line In = 1, Mic = 2)
  Variable *v = config->FindVariable(FourCC::VarRecordSource);
  intVarField_.emplace_back(position, *v, "Audio source: %s", 1, 2, 1, 1);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));
  (*intVarField_.rbegin()).AddObserver(*this);

  position.y_ += 1;
  v = config->FindVariable(FourCC::VarRecordLineGain);
  intVarField_.emplace_back(position, *v, "Line gain: %d dB", LINEIN_GAIN_MINDB, LINEIN_GAIN_MAXDB, 1, 2);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));
  (*intVarField_.rbegin()).AddObserver(*this);

  position.y_ += 1;
  v = config->FindVariable(FourCC::VarRecordMicGain);
  intVarField_.emplace_back(position, *v, "Mic gain: %d dB", MIC_GAIN_MINDB, MIC_GAIN_MAXDB, 1, 2);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));
  (*intVarField_.rbegin()).AddObserver(*this);
}

RecordView::~RecordView() {
}

void RecordView::Reset() {
  uiRecordingActive_ = false;
  uiSavingActive_ = false;
  autoSwitchPending_ = false;
  recordingStartTime_ = 0;
  recordingDuration_ = 0;
}

// Static method to set the source view type before opening SampleEditorView
void RecordView::SetSourceViewType(ViewType vt) {
  sourceViewType_ = vt;
}

void RecordView::ProcessButtonMask(unsigned short mask, bool pressed) {

  if (!pressed) {
    return;
  }

  if (uiSavingActive_) {
    // Ignore input while saving to keep stop/save flow deterministic.
    return;
  }

  // While actively recording, lock out all parameter edits/navigation except:
  // - PLAY: stop recording (existing save/switch flow)
  // - NAV-LEFT: leave immediately and stop recording in background
  if (uiRecordingActive_) {
    if ((mask & EPBM_NAV) && (mask & EPBM_LEFT)) {
      uiRecordingActive_ = false;
      uiSavingActive_ = false;
      autoSwitchPending_ = false;

      Navigate(sourceViewType_);
      return;
    }

    if (mask & EPBM_PLAY) {
      stopAndSwitchToEditor();
      return;
    }

    return;
  }

  if (mask & EPBM_NAV) {
    if (mask & EPBM_LEFT) {
      Navigate(sourceViewType_);
      StopMonitoring();
      return;
    }
  }

  // Handle PLAY button for start/stop recording
  if (mask & EPBM_PLAY) {
    record();
    isDirty_ = true;
    return;
  }

  // Let FieldView handle other button presses (navigation, etc.)
  FieldView::ProcessButtonMask(mask, pressed);
}

void RecordView::DrawView() {
  Clear();

  // Draw title
  
  DrawTitle("Record");

  // Draw recording status and time
  GUIPoint pos = GetAnchor();
  pos.y_ += 4;

  SetColor(Theme::View::fg);

  if (uiSavingActive_) {
    SetColor(Theme::View::error);
    DrawString(pos.x_, pos.y_, "SAVING");
    SetColor(Theme::View::fg);
  } else if (uiRecordingActive_) {
    SetColor(Theme::View::error);
    DrawString(pos.x_, pos.y_, "[REC]");
    SetColor(Theme::View::fg);
  } else {
    DrawString(pos.x_, pos.y_, "[---]");
  }

  // Draw time display
  if (uiRecordingActive_ || uiSavingActive_) {
    SetColor(Theme::View::error);
  }
  pos.x_ += 7;
  if (uiSavingActive_) {
    char percentBuffer[8];
    uint8_t percent = GetSavingProgressPercent();
    snprintf(percentBuffer, sizeof(percentBuffer), "%3u%%", percent);
    DrawString(pos.x_, pos.y_, percentBuffer);
  } else {
    char timeBuffer[16];
    formatTime(recordingDuration_, timeBuffer, sizeof(timeBuffer));
    DrawString(pos.x_, pos.y_, timeBuffer);
  }

  // Draw instructions
  pos.y_ += 2;
  pos.x_ = GetAnchor().x_;
  SetColor(Theme::View::fg);
  const char *instruction = uiSavingActive_ ? "" : uiRecordingActive_ ? "PRESS PLAY TO STOP" : "PRESS PLAY TO RECORD";
  DrawString(pos.x_, pos.y_, instruction);

  // Draw fields
  FieldView::Redraw();

  SetColor(Theme::View::fg);
}

void RecordView::OnFocus() {
  isDirty_ = true;
  recordingDuration_ = 0;

  auto config = Config::GetInstance();

  updateRecordingSource();
  StartMonitoring();
}

void RecordView::Update(Observable &o, I_ObservableData *data) {
  if (!hasFocus_) {
    return;
  }

  uintptr_t fourcc = (uintptr_t)data;

  auto config = Config::GetInstance();

  switch (fourcc) {
    case FourCC::VarRecordSource:
      StopMonitoring();
      updateRecordingSource();
      StartMonitoring();
      break;
  }
}

void RecordView::AnimationUpdate() {
  // First call the parent class implementation to draw the battery gauge, power
  // off etc
  ScreenView::AnimationUpdate();

  if (uiRecordingActive_) {
    // Update recording duration
    uint32_t currentTime = System::GetInstance()->Millis();
    recordingDuration_ = (currentTime - recordingStartTime_);
    if (recordingDuration_ >= kMaxRecordDurationMs) {
      stopAndSwitchToEditor();
      return;
    }
    isDirty_ = true;
    DrawView();
  }
  if (uiSavingActive_) {
    isDirty_ = true;
    DrawView();
  }

  // Get player instance safely
  Player *player = Player::GetInstance();
  // Only process updates if we're fully initialized
  if (!player) {
    return;
  }
}

void RecordView::record() {
  Trace::Log("RECORD", "Recording not yet supported on pico");
}

void RecordView::stop() {
  Trace::Log("RECORD", "Recording not yet supported on pico");
}

void RecordView::stopAndSwitchToEditor() {
}

void RecordView::formatTime(uint32_t milliseconds, char *buffer, size_t bufferSize) {
  uint32_t seconds = milliseconds / 1000;
  uint32_t minutes = seconds / 60;
  seconds = seconds % 60;

  snprintf(buffer, bufferSize, "%02d:%02d", (int)minutes, (int)seconds);
}

void RecordView::updateRecordingSource() {
  auto config = Config::GetInstance();
  auto source = config->FindVariable(FourCC::VarRecordSource)->GetInt();
  SetInputSource((RecordSource)source);
}

void RecordView::OnFocusLost() {
  Config *config = Config::GetInstance();
  if (!config->Save()) {
    Trace::Error("RECORDVIEW", "Failed to save record setting on focus lost");
    return;
  }
  Trace::Log("RECORDVIEW", "Saved record setting on focus lost");
}
