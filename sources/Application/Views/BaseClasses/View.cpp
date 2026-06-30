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
      isDirty_(true), viewType_(VT_SONG), hasFocus_(false) {
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
  SetColor(Theme::View::Map::fg(false));
  SetBackgroundColor(Theme::View::Map::bg(false));

  char buffer[5];
  // row1
  DrawString(pos.x_, pos.y_, "D   ");
  // row2
  DrawString(pos.x_, pos.y_ + 1, "P" char_dotted_horizontal_s "G ");
  // row3
  DrawString(pos.x_, pos.y_ + 2, "SCPI");
  // row4
  DrawString(pos.x_, pos.y_ + 3, "M" char_dotted_horizontal_s "TT");

  // dotted fast forward symbols
  SetColor(Theme::View::inactive);
  DrawString(pos.x_ + 1, pos.y_ + 1, char_dotted_horizontal_s);
  DrawString(pos.x_ + 1, pos.y_ + 3, char_dotted_horizontal_s);

  // draw current screen on map
  SetColor(Theme::View::Map::fg(true));
  SetBackgroundColor(Theme::View::Map::bg(true));

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

void View::drawRegularNote(const GUIPoint &pos, uint8_t channel) {
  Player *player = Player::GetInstance();

  int note = player->GetChannelNote(channel);
  uint8_t instrument = player->GetPlayedInstrument(channel);

  char buf[3];

  if (note == NOTE_OFF) {
    strcpy(buf, "of");
  } else if (note == NO_NOTE) {
    strcpy(buf, "  ");
  } else {
    strcpy(buf, noteNames[note % 12]);
  }

  DrawString(pos.x_, pos.y_ + 0, buf); // row for the note values

  // draw octave

  if (note != NO_NOTE) {
    npf_snprintf(buf, sizeof(buf), " %X", note / 12);
  } else {
    strcpy(buf, "  ");
  }
  DrawString(pos.x_, pos.y_ + 1, buf); // row for the octive values

  // draw instrument

  if (instrument == NO_INSTRUMENT || note == NO_NOTE) {
    strcpy(buf, "  ");
  } else {
    byteToHexString(instrument, buf);
  }

  // draw instrument number
  DrawString(pos.x_, pos.y_ + 2, "  ");
}

void View::drawNotes() {
  GUIPoint anchor = GetAnchor();
  int initialX = View::margin_ + 5;
  int initialY = anchor.y_ + View::songRowCount_ + 2;
  GUIPoint pos(initialX, initialY);

  Player *player = Player::GetInstance();

  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
    bool highlighted = (i == viewData_->songX_);
    SetBackgroundColor(Theme::Notes::bg(highlighted));
    SetColor(Theme::Notes::fg(highlighted));

    if (player->IsRunning() && viewData_->playMode_ != PM_AUDITION) {
      uint8_t sliceIndex = 0;
      if (player->GetPlayedSliceIndex(i, sliceIndex)) {
        DrawString(pos.x_, pos.y_, "SL");
        pos.y_++;
        char buf[3];
        buf[0] = static_cast<char>('0' + (sliceIndex / 10));
        buf[1] = static_cast<char>('0' + (sliceIndex % 10));
        buf[2] = '\0';
        DrawString(pos.x_, pos.y_, buf);
        pos.y_++;

        uint8_t instrument = player->GetPlayedInstrument(i);
        if (instrument == NO_INSTRUMENT) {
          strcpy(buf, "--");
        } else {
          byteToHexString(instrument, buf);
        }
        DrawString(pos.x_, pos.y_, buf); // draw instrument number
      } else {
        drawRegularNote(pos, i);
      }
    } else {
      drawRegularNote(pos, i);
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

void View::drawVUMeter(int32_t leftBars, int32_t rightBars, GUIPoint pos, int vuIndex, bool forceRedraw) {
  SetBackgroundColor(Theme::View::bg);

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
        SetColor(Theme::VU::clip);
      } else if (i > VU_METER_WARN_LEVEL) {
        SetColor(Theme::VU::warn);
      } else {
        SetColor(Theme::VU::normal);
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

  SetBackgroundColor(Theme::View::bg);
  SetColor(Theme::View::fg);
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

void View::ProcessButton(uint16_t mask, bool pressed) {
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
  w_.Clear();
}

void View::SwapColors() {
  ((AppWindow &)w_).SwapColors();
}

void View::SetColor(Color cd) {
  w_.SetColor(cd);
}

void View::SetBackgroundColor(Color cd) {
  w_.SetBackgroundColor(cd);
}

void View::ClearTextRect(int x, int y, int w, int h) {
  GUIRect rect(x, y, (x + w), (y + h));
  w_.ClearTextRect(rect);
}

void View::DrawString(int x, int y, const char *text) {
  GUIPoint pos(x, y);
  w_.DrawString(text, pos);
}

void View::DrawChar(int x, int y, const char character, bool transparent) {
  GUIPoint pos(x, y);
  w_.DrawChar(character, pos, transparent);
}

void View::DrawRect(GUIRect &r, Color color) {
  w_.SetCurrentRectColor(color);
  w_.DrawRect(r);
}

void View::drawBattery() {
  SetBackgroundColor(Theme::View::Title::bg);

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

  GUIPoint battpos = GUIPoint(SCREEN_WIDTH - 4, 0); // battery gauge is consistently 4 chars

#if BATTERY_LEVEL_AS_PERCENTAGE
  uint8_t batteryPercent = std::max(batteryState_.percentage, 99);

  Color batteryColor = CD_NORMAL;
  if (batteryPercent <= 5) {
    batteryColor = Theme::View::error;
  } else if (batteryPercent < 20) {
    batteryColor = Theme::View::war;
  } else if (batteryState_.charging) {
    batteryColor = Theme::View::fg;
  }

  SetColor(batteryColor);

  // Keep percentage branch compact: 3 digits + right battery cap.
  char batteryText[5];
  char endCap = batteryState_.charging ? char_symbol_charging_s : char_battery_right_s;
  npf_snprintf(batteryText, sizeof(batteryText), char_battery_left_s "%2u%c", batteryPercent, endCap);
  DrawString(battpos.x_, battpos.y_, , batteryText);
#else
  // use define to choose between drawing battery percentage or battery level as
  // bars
  SetColor(Theme::View::fg);
  const char *battText = nullptr;

  if (batteryState_.charging) {
    SetColor(Theme::View::charging);
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
      SetColor(Theme::View::warning);
      battText = string_battery_0_percent;
    } else {
      SetColor(Theme::View::error);
      battText = string_battery_0_percent;
    }
  }

  int battLen = static_cast<int>(strlen(battText));
  constexpr int kBattWidth = 4;
  int startX = SCREEN_WIDTH - kBattWidth;
  battpos.x_ = startX + (kBattWidth - battLen);
  DrawString(battpos.x_, battpos.y_, battText);
#endif
}

void View::DrawBorder(int32_t x, int32_t y, int32_t width, int32_t height, bool thick = false) {
  const char *thickChars = char_border_double_topLeft_s char_border_double_topRight_s char_border_double_bottomLeft_s
      char_border_double_bottomRight_s char_border_double_horizontal_s char_border_double_vertical_s;
  const char *thinChars = char_border_double_topLeft_s char_border_double_topRight_s char_border_double_bottomLeft_s
      char_border_double_bottomRight_s char_border_double_horizontal_s char_border_double_vertical_s;

  const char *chars = thick ? thickChars : thinChars;

  DrawChar(x, y, chars[0]);
  DrawChar(x + width - 1, y, chars[1]);
  DrawChar(x, y + height - 1, chars[2]);
  DrawChar(x + width - 1, y + height - 1, chars[3]);

  // horizontal borders
  for (int32_t i = x + 1; i < x + width - 1; i++) {
    DrawChar(i, y, chars[4]);
    DrawChar(i, y + height - 1, chars[4]);
  }

  // left and right borders
  for (int32_t j = y + 1; j < y + height - 1; j++) {
    DrawChar(x, j, chars[5]);
    DrawChar(x + width - 1, j, chars[5]);
  }
}

int View::DrawButton(int x, int y, const char *title, bool selected) {
  int len = strlen(title);

  SetBackgroundColor(Theme::View::bg);
  SetColor(Theme::View::Button::bg(selected));
  DrawString(x, y, char_button_border_left_s);

  SetBackgroundColor(Theme::View::Button::bg(selected));
  SetColor(Theme::View::Button::fg(selected));
  DrawString(x + 1, y, title);

  SetBackgroundColor(Theme::View::bg);
  SetColor(Theme::View::Button::bg(selected));
  DrawString(x + 1 + len, y, char_button_border_right_s);

  return len + 2;
}

int View::DrawTab(int x, int y, const char *title, bool selected) {
  int len = strlen(title);

  SetBackgroundColor(Theme::View::bg);
  SetColor(Theme::View::Tab::bg(selected));
  DrawString(x, y, char_tab_border_left_s);

  SetBackgroundColor(Theme::View::Tab::bg(selected));
  SetColor(Theme::View::Tab::fg(selected));
  DrawString(x + 1, y, title);

  SetBackgroundColor(Theme::View::bg);
  SetColor(Theme::View::Tab::bg(selected));
  DrawString(x + 1 + len, y, char_tab_border_right_s);

  return len + 2;
}

void View::DrawFilledBorder(int32_t x, int32_t y, int32_t width, int32_t height, Color fill, bool half = false) {
  // corners, top, right, bottom, left
  const char *halfChars =
      char_filledHalfBorder_topLeft_s char_filledHalfBorder_topRight_s char_filledHalfBorder_bottomLeft_s
          char_filledHalfBorder_bottomRight_s char_block_bottom_s char_block_left_s char_block_top_s char_block_right_s;
  const char *fullChars = char_filledBorder_topLeft_s char_filledBorder_topRight_s char_filledBorder_bottomLeft_s
      char_filledBorder_bottomRight_s char_block_full_s char_block_full_s char_block_full_s char_block_full_s;

  const char *chars = half ? halfChars : fullChars;

  SetColor(fill);

  // corners
  DrawChar(x, y, chars[0], true);
  DrawChar(x + width - 1, y, chars[1], true);
  DrawChar(x, y + height - 1, chars[2], true);
  DrawChar(x + width - 1, y + height - 1, chars[3], true);

  // horizontal borders
  for (int32_t i = x + 1; i < x + width - 1; i++) {
    DrawChar(i, y, chars[4], true);
    DrawChar(i, y + height - 1, chars[6], true);
  }

  // left and right borders
  for (int32_t j = y + 1; j < y + height - 1; j++) {
    DrawChar(x, j, chars[5], true);
    DrawChar(x + width - 1, j, chars[7], true);
  }

  // fill
  SetBackgroundColor(fill);

  for (int i = x + 1; i < x + width - 1; i++) {
    for (int j = y + 1; j < y + height - 1; j++) {
      DrawChar(i, j, ' ');
    }
  }
}

void View::drawScrollBar(uint16_t x, uint16_t y, uint16_t height, uint16_t index, uint16_t total) {
  if (total <= height) {
    return; // no scrollbar needed
  }

  SetColor(Theme::View::scrollbar);

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

  HelpLegend help = getHelpLegend(command);

  // highlight the letters that are the symbol for the command
  SetBackgroundColor(Theme::View::bg);

  int y = SCREEN_HEIGHT - 4;

  bool preColon = true;
  for (size_t x = 0; x < strlen(help.line1); x++) {
    unsigned char c = help.line1[x];

    if (c == ':') {
      preColon = false;
    }

    SetColor(Theme::View::help(preColon && c >= 'A' && c <= 'Z'));
    DrawChar(5 + x, y, help.line1[x]);
  }

  SetColor(Theme::View::help(false));
  DrawString(5, y + 1, help.line2);
  DrawString(5, y + 2, help.line3);
  DrawString(5, y + 3, help.line4);
}

void View::DrawTitle(const char *format, ...) {
  GUIPoint pos = GetTitlePosition();

  SetBackgroundColor(Theme::View::Title::bg);
  SetColor(Theme::View::Title::fg);

  constexpr size_t maxLength = SCREEN_WIDTH - BATTERY_GAUGE_WIDTH;

  va_list val;
  va_start(val, format);
  static char buffer[maxLength + 1];
  npf_vsnprintf(buffer, sizeof(buffer), format, val);
  va_end(val);

  // make sure we extend the length to fill the entire screen -  battery gauge
  size_t len = strlen(buffer);

  if (len <= maxLength) {
    memset(buffer + len, ' ', 28 - len);
    buffer[maxLength] = '\0';
  } else {
    buffer[maxLength] = '\0';
  }

  DrawString(pos.x_, pos.y_, buffer);
}

void View::drawRowNumbers(int x, int y, int start, int numRows) {
  SetBackgroundColor(Theme::View::bg);

  char row[3];

  for (int j = 0; j < numRows; j++) {
    SetColor(Theme::View::index((j + start) % ALT_ROW_NUMBER == 0));
    byteToHexString(j, row);
    DrawString(x, y + j, row);
  }
}
