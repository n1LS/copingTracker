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

#include "View.h"
#include "Application/AppWindow.h"
#include "Application/Player/Player.h"
#include "Application/Utils/HelpLegend.h"
#include "Application/Utils/char.h"
#include "Application/Utils/mathutils.h"
#include "Application/Views/RecordView.h"
#include "Application/Views/SampleEditorView.h"
#include "Foundation/Constants/SpecialCharacters.h"
#include "ModalView.h"
#include "System/Console/Trace.h"
#include <UIFramework/SimpleBaseClasses/EventManager.h>
#include <nanoprintf.h>

bool View::initPrivate_ = false;

int View::margin_ = 0;
int View::songRowCount_ = 16;

BatteryState View::batteryState_ = {
    .percentage = 0,
    .voltage_mv = 0,
    .temperature_c = 0,
    .charging = false,
};

BatteryState View::latestBatteryState_ = {
    .percentage = 0,
    .voltage_mv = 0,
    .temperature_c = 0,
    .charging = false,
};

uint32_t View::lastBatteryDisplayFrame_ = 0;
bool View::batteryDisplayInitialized_ = false;

View::View(GUIWindow &w, ViewData *viewData)
    : w_(w), viewData_(viewData), needsRedraw_(false), isVisible_(true), vuMeterCount_(0), viewMode_(VM_NORMAL),
      isDirty_(true), viewType_(VT_SONG), hasFocus_(false), powerButtonPressed_(false), powerButtonHoldCount_(0) {
  if (!initPrivate_) {
    View::margin_ = 0;
    songRowCount_ = 16;
    initPrivate_ = true;
  }
  mask_ = 0;
  locked_ = false;
  modalView_ = 0;
  modalViewCallback_ = ModalViewCallback();
  // Initialize VU meter tracking variables
  for (int i = 0; i < SONG_CHANNEL_COUNT + 1; i++) {
    prevLeftVU_[i] = 0;
    prevRightVU_[i] = 0;
  }
}
GUIPoint View::GetAnchor() {
  // Original code had a dynamic anchor point dending on song count, but
  // changing the song count didn't work anyway given that there are many places
  // where it was statically defined as 8. Other screens also don't fit with a
  // dynamic anchor point
  return GUIPoint(5, 3);
}

GUIPoint View::GetTitlePosition() {
  return GUIPoint(0, 0);
}

bool View::Lock() {
  if (locked_)
    return false;
  locked_ = true;
  return true;
}

void View::WaitForObject() {
  while (locked_) {
  };
}

void View::Unlock() {
  locked_ = false;
}

void View::drawMap() {
  GUIPoint anchor = GetAnchor();
  GUIPoint pos(View::margin_, anchor.y_ + View::songRowCount_ + 1);

  // draw entire map
  SetColor(cNormal);
  SetBackgroundColor(cBackground);
  
  char buffer[5];
  // row1
  DrawString(pos.x_, pos.y_, "D   ");
  pos.y_++;
  // row2
  DrawString(pos.x_, pos.y_, "P G ");
  pos.y_++;
  // row3
  DrawString(pos.x_, pos.y_, "SCPI");
  pos.y_++;
  // row4
  DrawString(pos.x_, pos.y_, "M TT");

  // draw current screen on map
  SetColor(cBackground);
  SetBackgroundColor(cHighlight2);
  pos.y_ = anchor.y_ + View::songRowCount_ + 1;
  switch (viewType_) {
  case VT_CHAIN:
    pos.x_ += 1;
    pos.y_ += 2;
    DrawString(pos.x_, pos.y_, "C");
    break;
  case VT_PHRASE:
    pos.x_ += 2;
    pos.y_ += 2;
    DrawString(pos.x_, pos.y_, "P");
    break;
  case VT_DEVICE:
    DrawString(pos.x_, pos.y_, "D");
    break;
  case VT_PROJECT:
    pos.y_ += 1;
    DrawString(pos.x_, pos.y_, "P");
    break;
  case VT_INSTRUMENT:
    pos.x_ += 3;
    pos.y_ += 2;
    DrawString(pos.x_, pos.y_, "I");
    break;
  case VT_TABLE: // under phrase
    pos.x_ += 2;
    pos.y_ += 3;
    DrawString(pos.x_, pos.y_, "T");
    break;
  case VT_TABLE2: // under instrument
    pos.x_ += 3;
    pos.y_ += 3;
    DrawString(pos.x_, pos.y_, "T");
    break;
  case VT_GROOVE:
    pos.x_ += 2;
    pos.y_ += 1;
    DrawString(pos.x_, pos.y_, "G");
    break;
  case VT_MIXER:
    pos.y_ += 3;
    DrawString(pos.x_, pos.y_, "M");
    break;
  default: // VT_SONG
    pos.y_ += 2;
    DrawString(pos.x_, pos.y_, "S");
  }
}

void View::drawNotes() {
  GUIPoint anchor = GetAnchor();
  int initialX = View::margin_ + 5;
  int initialY = anchor.y_ + View::songRowCount_ + 2;
  GUIPoint pos(initialX, initialY);

  Player *player = Player::GetInstance();

  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
    bool highlighted = (i == viewData_->songX_);
    SetBackgroundColor(highlighted ? cHighlight2 : cHighlight1);
    SetColor(cBackground);
    
    if (player->IsRunning() && viewData_->playMode_ != PM_AUDITION) {
      uint8_t sliceIndex = 0;
      if (player->GetPlayedSliceIndex(i, sliceIndex)) {
        DrawString(pos.x_, pos.y_, "SL");
        pos.y_++;
        char sliceBuffer[3];
        sliceBuffer[0] = static_cast<char>('0' + (sliceIndex / 10));
        sliceBuffer[1] = static_cast<char>('0' + (sliceIndex % 10));
        sliceBuffer[2] = '\0';
        DrawString(pos.x_, pos.y_, sliceBuffer);
        pos.y_++;
        DrawString(pos.x_, pos.y_, player->GetPlayedInstrument(i)); // draw instrument number
      } else {
        DrawString(pos.x_, pos.y_, player->GetPlayedNote(i)); // row for the note values
        pos.y_++;
        DrawString(pos.x_, pos.y_, player->GetPlayedOctive(i)); // row for the octive values
        pos.y_++;
        DrawString(pos.x_, pos.y_, player->GetPlayedInstrument(i)); // draw instrument number
      }
    } else {
      DrawString(pos.x_, pos.y_, "  "); // row for the note
                                               // values
      pos.y_++;
      DrawString(pos.x_, pos.y_, "  "); // row for the octive values
      pos.y_++;
      DrawString(pos.x_, pos.y_, "  "); // draw instrument number
    }
    pos.y_ = initialY;
    pos.x_ += 3;
  }
}

void View::drawMasterVuMeter(Player *player, bool forceRedraw, uint8_t xoffset) {
  stereosample playerLevel = player->GetMasterLevel();

  // Convert amplitude to bar levels
  int32_t leftBars, rightBars;
  amplitudeToBars(playerLevel, &leftBars, &rightBars);

  // we start at the bottom of the VU meter and draw it growing upwards
  GUIPoint pos = GetAnchor();
  pos.x_ += xoffset;
  pos.y_ += VU_METER_HEIGHT - 1; // -1 to align with song grid

  // Use index 0 for the master VU meter
  drawVUMeter(leftBars, rightBars, pos, 0, forceRedraw);
}

void View::drawVUMeter(int32_t leftBars, int32_t rightBars, GUIPoint pos, int vuIndex,
                       bool forceRedraw) {

  // Clamp the values to the maximum height
  leftBars = std::min<int32_t>(leftBars, VU_METER_MAX);
  rightBars = std::min<int32_t>(rightBars, VU_METER_MAX);

  // Add inertia effect by limiting the rate of change
  // Maximum step change allowed per update
  const int maxStepChange = 20;
  const int fallStepChange = 10;

  // For rising levels (current > previous), allow faster response
  if (leftBars > prevLeftVU_[vuIndex]) {
    // If the difference is greater than maxStepChange, limit it
    if (leftBars - prevLeftVU_[vuIndex] > maxStepChange) {
      leftBars = prevLeftVU_[vuIndex] + maxStepChange;
    }
  }
  // For falling levels (current < previous), add more inertia for a slower fall
  else if (leftBars < prevLeftVU_[vuIndex]) {
    // Use a smaller step for falling levels to create more inertia
    if (prevLeftVU_[vuIndex] - leftBars > fallStepChange) {
      leftBars = prevLeftVU_[vuIndex] - fallStepChange;
    }
  }

  // Same for right channel
  if (rightBars > prevRightVU_[vuIndex]) {
    if (rightBars - prevRightVU_[vuIndex] > maxStepChange) {
      rightBars = prevRightVU_[vuIndex] + maxStepChange;
    }
  } else if (rightBars < prevRightVU_[vuIndex]) {
    if (prevRightVU_[vuIndex] - rightBars > fallStepChange) {
      rightBars = prevRightVU_[vuIndex] - fallStepChange;
    }
  }

  // Left channel: Handle level changes
  bool leftChanged = (leftBars != prevLeftVU_[vuIndex]);
  bool rightChanged = (rightBars != prevRightVU_[vuIndex]);

  if (forceRedraw || leftChanged || rightChanged) {
    // If forcing redraw or level changed, redraw the entire meter

    // Then draw the active cells with inversion enabled

    for (int i = 0; i < VU_METER_HEIGHT; i++) {
      // Set appropriate color based on level
      if (i == VU_METER_CLIP_LEVEL) {
        SetColor(cError);
      } else if (i > VU_METER_WARN_LEVEL) {
        SetColor(cWarn);
      } else {
        SetColor(cInfo);
      }

      // draw left channel if changed
      if (leftChanged) {
        DrawString(pos.x_, pos.y_ - i, char_bargraph_s(leftBars - 10 * i));
      }

      // draw right channel if changed
      if (rightChanged) {
        DrawString(pos.x_ + 1, pos.y_ - i, char_bargraph_s(rightBars - 10 * i));
      }
    }
  }

  // Store the current values for next time
  prevLeftVU_[vuIndex] = leftBars;
  prevRightVU_[vuIndex] = rightBars;
}

void View::drawPlayTime(Player *player, GUIPoint pos) {
  char strbuffer[10];

  SetBackgroundColor(cBackground);
  SetColor(cNormal);
  int time = int(player->GetPlayTime());
  int mi = time / 60;
  int se = time - mi * 60;
  npf_snprintf(strbuffer, sizeof(strbuffer), "%2.2d:%2.2d", mi, se);
  DrawString(pos.x_, pos.y_, strbuffer);
}

void View::DoModal(ModalView *view, ModalViewCallback cb) {
  modalView_ = view;
  modalView_->OnFocus();
  modalViewCallback_ = cb;
  isDirty_ = true;
}

void View::DismissModal() {
  if (modalView_ && modalView_->IsFinished()) {
    ModalView *finishedModal = modalView_;
    const uint32_t finishedModalId = finishedModal->GetInstanceId();
    ModalViewCallback callback = modalViewCallback_;

    // Clear current modal first so callback can safely open another modal.
    modalView_ = nullptr;
    modalViewCallback_ = ModalViewCallback();

    if (callback) {
      callback(*this, *finishedModal);
    }

    // Callback may have already replaced this instance in shared storage.
    if (finishedModal->GetInstanceId() == finishedModalId) {
      finishedModal->Destroy();
    }

    isDirty_ = true;
  }
}

void View::Redraw() {
  if (modalView_) {
    if (isDirty_) {
      DrawView();
    }
    modalView_->Redraw();
  } else {
    DrawView();
  }
  isDirty_ = false;
}

void View::SetDirty(bool isDirty) {
  isDirty_ = isDirty;
}

void View::ProcessButton(unsigned short mask, bool pressed) {
  if (!pressed) {
    powerButtonPressed_ = false;
  } else if (mask & EPBM_POWER) {
    powerButtonPressed_ = pressed;
  }

  // Normal button processing
  if (modalView_) {
    modalView_->ProcessButton(mask, pressed);
    // checks if modal is done and if it is disposes of it:
    DismissModal();
  } else {
    ProcessButtonMask(mask, pressed);
  }

  if (isDirty_)
    ((AppWindow &)w_).SetDirty();
}

void View::Clear() {
  ((AppWindow &)w_).Clear();
}

void View::ForceClear() {
  ((AppWindow &)w_).Clear(true);
}

void View::SwapColors() {
  ((AppWindow &)w_).SwapColors();
}

void View::SetColor(Color cd) {
  ((AppWindow &)w_).SetColor(cd);
}

void View::SetBackgroundColor(Color cd) {
  ((AppWindow &)w_).SetBackgroundColor(cd);
}

void View::ClearTextRect(int x, int y, int w, int h) {
  GUIRect rect(x, y, (x + w), (y + h));
  w_.ClearTextRect(rect);
}

void View::DrawString(int x, int y, const char *text) {
  GUIPoint pos(x, y);
  w_.DrawString(text, pos);
}

void View::DrawChar(int x, int y, const char character) {
  GUIPoint pos(x, y);
  w_.DrawChar(character, pos);
}

void View::DrawRect(GUIRect &r, Color color) {
  w_.SetCurrentRectColor(AppWindow::GetGUIColor(color));
  w_.DrawRect(r);
}

void View::drawBattery() {
  SetBackgroundColor(cBackground);

  const uint32_t frameCounter = AppWindow::GetAnimationFrameCounter();
  const bool sampleNow = (frameCounter % PICO_CLOCK_HZ) == 0;

  // Sample the battery once per second.
  if (sampleNow) {
    System *sys = System::GetInstance();
    sys->GetBatteryState(latestBatteryState_);
    if (!batteryDisplayInitialized_) {
      batteryState_ = latestBatteryState_;
      lastBatteryDisplayFrame_ = frameCounter;
      batteryDisplayInitialized_ = true;
    } else {
      if (latestBatteryState_.charging != batteryState_.charging) {
        // Immediately display change in charging state
        batteryState_ = latestBatteryState_;
        lastBatteryDisplayFrame_ = frameCounter;
      } else {
        constexpr uint32_t kBatteryDisplayUpdateFrames = PICO_CLOCK_HZ * 120;
        if ((frameCounter - lastBatteryDisplayFrame_) >= kBatteryDisplayUpdateFrames) {
          // While discharging, update the display at most every 120 seconds.
          batteryState_ = latestBatteryState_;
          lastBatteryDisplayFrame_ = frameCounter;
        }
      }
    }
  }

  GUIPoint battpos = GetAnchor();
  battpos.y_ = 0;

#if BATTERY_LEVEL_AS_PERCENTAGE
  uint8_t batteryPercent = batteryState_.percentage;
  if (batteryPercent > 100) {
    batteryPercent = 100;
  }

  Color batteryColor = CD_NORMAL;
  if (batteryPercent <= 5) {
    batteryColor = cError;
  } else if (batteryPercent < 20) {
    batteryColor = cWarn;
  } else if (batteryState_.charging) {
    batteryColor = cInfo;
  }

  SetColor(batteryColor);

  // Keep percentage branch compact: 3 digits + right battery cap.
  char batteryText[4];
  npf_snprintf(batteryText, sizeof(batteryText), "%3u", batteryPercent);

  constexpr int kBatteryWidgetWidth = 4; // 3 text chars + right-side symbol
  int startX = SCREEN_WIDTH - kBatteryWidgetWidth;
  ClearTextRect(startX, battpos._y, kBatteryWidgetWidth, 1);

  DrawString(startX, battpos._y, batteryText);
  const char *rightSymbol = batteryState_.charging ? char_symbol_charging_s : char_battery_right_s;
  DrawString(startX + 3, battpos._y, rightSymbol);
#else
  // use define to choose between drawing battery percentage or battery level as
  // bars
  SetColor(cNormal);
  const char *battText = nullptr;

  if (batteryState_.charging) {
    SetColor(cAccent);
    battText = string_battery_charging;
  } else {
    if (batteryState_.percentage > 90) {
      battText = string_battery_100_percent;
    } else if (batteryState_.percentage > 65) {
      battText = string_battery_75_percent;
    } else if (batteryState_.percentage > 40) {
      battText = string_battery_50_percent;
    } else if (batteryState_.percentage > 35) {
      battText = string_battery_25_percent;
    } else if (batteryState_.percentage > 10) {
      SetColor(cWarn);
      battText = string_battery_0_percent;
    } else {
      SetColor(cError);
      battText = string_battery_0_percent;
    }
  }

  int battLen = (battText != nullptr) ? static_cast<int>(strlen(battText)) : 0;
  constexpr int kBattWidth = 6; // "[100%]" is the widest we render
  int startX = SCREEN_WIDTH - kBattWidth;
  ClearTextRect(startX, battpos.y_, kBattWidth, 1);
  battpos.x_ = startX + (kBattWidth - battLen); // we want to right align the batt widget
  DrawString(battpos.x_, battpos.y_, battText);
#endif
}

// Draw power button UI overlay
void View::drawPowerButtonUI() {
  // Only process and draw UI when power button is pressed
  if (powerButtonPressed_) {
    char countdownMessage[SCREEN_WIDTH];
    powerButtonHoldCount_++;

    int remainingSeconds = 3 - (powerButtonHoldCount_ / PICO_CLOCK_HZ);
    if (remainingSeconds < 0) {
      remainingSeconds = 0;
    }

    snprintf(countdownMessage, sizeof(countdownMessage), "Hold for shutdown (%d sec)", remainingSeconds);

    if (remainingSeconds == 0) {
      Trace::Debug("Power button held for threshold time, Powerdown!");

      System::GetInstance()->PowerDown();
    }

    // Calculate center position for the message
    GUIPoint pos = GetAnchor();
    uint16_t mesgLen = strlen(countdownMessage);
    pos.x_ = (SCREEN_WIDTH - mesgLen) / 2;
    pos.y_ = SCREEN_HEIGHT / 2 - 1;

    // Draw a background box
    SetBackgroundColor(cBackground);
    for (int y = pos.y_ - 1; y <= pos.y_ + 1; y++) {
      for (int x = pos.x_ - 1; x <= (uint16_t)(pos.x_ + mesgLen + 1); x++) {
        DrawString(x, y, " ");
      }
    }

    // Draw the message
    SetColor(cEmphasis);
    DrawString(pos.x_, pos.y_, countdownMessage);
  } else if (powerButtonHoldCount_ > 0) {
    // Reset hold counter when button is released
    powerButtonHoldCount_ = 0;

    // Force immediate redraw by calling DrawView directly
    // This will redraw the entire screen with the correct content
    DrawView();

    Trace::Debug("Power button released! View redrawn.");
  }
}

void View::switchToRecordView() {
  // recording view only not yet supported on pico
  return;

  // if (!Player::GetInstance()->IsRunning()) {
  //   RecordView::SetSourceViewType(viewType_);
  //   SampleEditorView::SetSourceViewType(viewType_);
  //   ViewType vt = VT_RECORD;
  //   ViewEvent ve(VET_SWITCH_VIEW, &vt);
  //   SetChanged();
  //   NotifyObservers(&ve);
  // }
}

void View::DrawBorder(int32_t x, int32_t y, int32_t width, int32_t height) {
  
  DrawChar(x, y, GLYPH(char_border_single_topLeft_s));
  DrawChar(x + width- 1, y, GLYPH(char_border_single_topRight_s));
  DrawChar(x, y + height - 1, GLYPH(char_border_single_bottomLeft_s));
  DrawChar(x + width - 1, y + height - 1, GLYPH(char_border_single_bottomRight_s));

  // horizontal borders
  for (int32_t i = x + 1; i < x + width - 1; i++) {
    DrawChar(i, y, GLYPH(char_border_single_horizontal_s));
    DrawChar(i, y + height - 1, GLYPH(char_border_single_horizontal_s));
  }
  
  // left and right borders
  for (int32_t j = y + 1; j < y + height - 1; j++) {
    DrawChar(x, j, GLYPH(char_border_single_vertical_s));
    DrawChar(x + width - 1, j, GLYPH(char_border_single_vertical_s));
  }
}

void View::drawScrollBar(uint16_t x, uint16_t y, uint16_t height, uint16_t index, uint16_t total) {
  if (total <= height) {
    return; // no scrollbar needed
  }

  SetColor(cNormal);

  // Thumb size represents the ratio of visible items to total items
  uint16_t thumbSize = std::max(1, (height * height) / total);

  // Thumb position: map topIndex (0 to maxScroll) onto available scrollbar
  // space
  uint16_t maxScroll = total - height;
  uint16_t availableSpace = height - thumbSize;
  uint16_t thumbPos = (index * availableSpace) / maxScroll;

  for (int dy = 0; dy < height; dy++) {
    bool thumb = (dy >= thumbPos) && (dy < thumbPos + thumbSize);
    const char *str = thumb ? char_block_full_s : char_border_single_vertical_s;
    DrawString(x, y + dy, str);
  }
}

void View::drawHelpLegend(FourCC command) {
  if (command == FourCC::InstrumentCommandNone) {
    // no command -> no help text
    return;
  }

  char **helpLegend = getHelpLegend(command);
  char line[SCREEN_WIDTH]; // -1 for 1char space start of line
  strcpy(line, helpLegend[0]);
  
  // highlight the letters that are the symbol for the command
  SetBackgroundColor(cBackground);

  bool preColon = true;
  for (size_t x = 0; x < strlen(line); x++) {
    unsigned char c = line[x];

    if (c == ':') {
      preColon = false;
    }

    if (preColon && c >= 'A' && c <= 'Z') {
      SetColor(cHighlight1);
    } else {
      SetColor(cNormal);
    }

    DrawChar(x, 0, line[x]);
  }
  
  memset(line, ' ', 32);
  if (helpLegend[1] != NULL) {
    strcpy(line, helpLegend[1]);
    DrawString(0, 1, line);
  }
}
