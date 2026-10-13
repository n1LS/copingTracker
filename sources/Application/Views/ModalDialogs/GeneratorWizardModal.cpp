/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "GeneratorWizardModal.h"
#include "Application/AppWindow.h"
#include "System/Console/nanoprintf.h"
#include "Foundation/Constants/SpecialCharacters.h"
#include <algorithm>
#include <string.h>
#include <new>

#define WINDOW_W 26
#define WINDOW_H 14
#define LABEL_X 2
#define VALUE_X 12
#define ROWS_Y 4
#define DENSITY_STEP 16
#define VALUE_W (WINDOW_W - VALUE_X - 4)

static const char *kTypeNames[GeneratorWizardModal::GT_COUNT] = {"Drum", "Melody", "Bassline", "Chords"};
static const char *kStyleNames[] = {"Straight", "Sparse", "Syncopated", "Broken", "Driving", "HalfTime"};
static const char *kFeelNames[] = {"Straight", "Triplet"};

static const uint8_t kStyleCount = sizeof(kStyleNames) / sizeof(kStyleNames[0]);
static const uint8_t kFeelCount = sizeof(kFeelNames) / sizeof(kFeelNames[0]);

bool GeneratorWizardModal::inUse_ = false;
alignas(GeneratorWizardModal) static unsigned char GeneratorWizardModalStorage[sizeof(GeneratorWizardModal)];
void *GeneratorWizardModal::storage_ = GeneratorWizardModalStorage;

GeneratorWizardModal *GeneratorWizardModal::Create(View &view) {
  if (inUse_) {
    auto *existing = reinterpret_cast<GeneratorWizardModal *>(storage_);
    existing->~GeneratorWizardModal();
    inUse_ = false;
  }
  inUse_ = true;
  return new (storage_) GeneratorWizardModal(view);
}

GeneratorWizardModal::GeneratorWizardModal(View &view) : ModalView(view) {
}

GeneratorWizardModal::~GeneratorWizardModal() {
}

void GeneratorWizardModal::Destroy() {
  this->~GeneratorWizardModal();
  inUse_ = false;
}

bool GeneratorWizardModal::isImplemented(GeneratorType type) {
  return type == GT_DRUM;
}

DrumGenerator::Settings GeneratorWizardModal::GetDrumSettings() const {
  DrumGenerator::Settings s{};
  s.style = static_cast<DrumGenerator::Style>(style_);
  s.feel = static_cast<DrumGenerator::Feel>(feel_);
  s.density = density_;
  s.variation = variation_;
  s.steps = DrumGenerator::kMaxSteps;
  s.phraseCount = 1;
  s.voices = DrumGenerator::VoiceCount::Mono;
  return s;
}

void GeneratorWizardModal::drawRow(int y, const char *label, const char *value, bool selected, bool enabled) {
  SetBackgroundColor(Theme::Dialog::Panel::bg);
  SetColor(Theme::Dialog::fg);
  DrawString(LABEL_X, y, label);

  drawField(VALUE_X, y, value, VALUE_W, selected && enabled);
}

// Instrument style field: square blocks when idle, rounded ends when focused
// (like UIField).
void GeneratorWizardModal::drawField(int x, int y, const char *text, int width, bool focused, char indicator) {
  const bool down = focused && enterDown_;
  char buf[WINDOW_W];
  int len = 0;
  while (len < width - 1 && text[len]) {
    buf[len] = text[len];
    len++;
  }
  while (len < width) {
    buf[len++] = ' ';
  }
  
  if (focused) {
    buf[len - 1] = indicator;
  }

  buf[len] = 0;

  SetColor(Theme::Dialog::Panel::Input::fg(focused));
  SetBackgroundColor(Theme::Dialog::Panel::Input::bg(focused));
  DrawString(x + 1, y, buf);

  SetColor(Theme::Dialog::Panel::Input::bg(focused));
  SetBackgroundColor(Theme::Dialog::Panel::bg);
  DrawChar(x, y, focused ? char_button_left(down) : CHAR(char_block_left_s));
  DrawChar(x + 1 + width, y, focused ? char_button_right(down) : CHAR(char_block_right_s));

  if (focused) {
    focusRect_ = GUIRect(x + left_, y + top_, width + 2, 1);
  }
}

void GeneratorWizardModal::DrawStep1() {
  SetBackgroundColor(Theme::Dialog::bg);
  SetColor(Theme::Dialog::fg);
  DrawString(LABEL_X, 3, "Select generator");

  DrawFilledBorder(1, 4, WINDOW_W - 2, 6, Theme::Dialog::Panel::bg, true);

  for (int i = 0; i < GT_COUNT; i++) {
    drawField(LABEL_X, ROWS_Y + 1 + i, kTypeNames[i], WINDOW_W - 6, row_ == i, CHAR(char_button_enter_s));
  }
  
  btn_ = BTN_LEFT;
  drawButtons(" Cancel ", nullptr);
}

void GeneratorWizardModal::DrawStep2() {
  SetBackgroundColor(Theme::Dialog::bg);
  SetColor(Theme::Dialog::fg);
  DrawString(LABEL_X, 3, "Configure generator");

  DrawFilledBorder(1, 4, WINDOW_W - 2, 6, Theme::Dialog::Panel::bg, true);

  char buf[8];
  drawRow(ROWS_Y + 1 + DR_STYLE, "Style", kStyleNames[style_], row_ == DR_STYLE);
  drawRow(ROWS_Y + 1 + DR_FEEL, "Feel", kFeelNames[feel_], row_ == DR_FEEL);
  npf_snprintf(buf, sizeof(buf), "%d", density_);
  drawRow(ROWS_Y + 1 + DR_DENSITY, "Density", buf, row_ == DR_DENSITY);
  npf_snprintf(buf, sizeof(buf), "%d", variation_);
  drawRow(ROWS_Y + 1 + DR_VARIATION, "Variation", buf, row_ == DR_VARIATION);
  drawButtons(" Back ", " Generate ");
}

void GeneratorWizardModal::DrawView() {
  SetBackgroundColor(Theme::Dialog::bg);

  left_ = (SCREEN_WIDTH - WINDOW_W) / 2;
  top_ = (SCREEN_HEIGHT - WINDOW_H) / 2;

  DrawWindow(0, 0, WINDOW_W, WINDOW_H, step_ == STEP_TYPE ? "PhraseGen (1/2)" : "PhraseGen (2/2)");

  if (step_ == STEP_TYPE) {
    DrawStep1();
  } else {
    DrawStep2();
  }
}

// Secondary button (Cancel / Back) on the left, primary (Next / Generate) on
// the right.
void GeneratorWizardModal::drawButtons(const char *left, const char *right) {
  const int y = WINDOW_H - 3;
  const bool onButtons = isButtonRow();
    const bool leftSel = onButtons && btn_ == BTN_LEFT;
    const bool rightSel = onButtons && btn_ == BTN_RIGHT;

    if (left) {
        int len = strlen(left);
        DrawButton(LABEL_X, y, left, leftSel, leftSel && enterDown_, Theme::Dialog::bg);
        if (leftSel) {
            focusRect_ = GUIRect(LABEL_X + left_, y + top_, len + 2, 1);
        }
   }

  if (right) {
      int len = strlen(right);
      int x = WINDOW_W - len - 4;
    DrawButton(x, y, right, rightSel, rightSel && enterDown_, Theme::Dialog::bg);
    if (rightSel) {
      focusRect_ = GUIRect(x + left_, y + top_, len + 2, 1);
    }
  }
}

void GeneratorWizardModal::changeValue(int delta) {
  if (step_ != STEP_DRUM) {
    return;
  }
  switch (row_) {
  case DR_STYLE:
    style_ = (style_ + kStyleCount + delta) % kStyleCount;
    break;
  case DR_FEEL:
    feel_ = (feel_ + kFeelCount + delta) % kFeelCount;
    break;
  case DR_DENSITY:
    density_ = std::clamp(density_ + delta * DENSITY_STEP, 0, 255);
    break;
  case DR_VARIATION:
    variation_ = std::clamp(variation_ + delta * DENSITY_STEP, 0, 255);
    break;
  }
}

void GeneratorWizardModal::buttonStep1() {
  if (row_ < 4) {
    switch (row_) {
      case 0:
        step_ = STEP_DRUM;
        row_ = 0;
            btn_ = BTN_RIGHT;
        break;
    }
  } else {
    EndModal(GWM_CANCEL);
  }
}

void GeneratorWizardModal::buttonStep2() {
  if (btn_ == BTN_LEFT) {
    step_ = STEP_TYPE;
    row_ = 0;
  } else {
    EndModal(GWM_ACCEPT);
  }
}

void GeneratorWizardModal::ProcessButtonMask(uint16_t mask, bool pressed) {
  const uint16_t previous = lastMask_;
  lastMask_ = mask;

  if (!pressed) {
    // actions fire on ENTER release, so the release doesn't leak into
    // whatever the modal hands control to next
    if (enterDown_ && (previous & ~mask) == BM_ENTER) {
      enterDown_ = false;
      isDirty_ = true;
      
      if (step_ == STEP_TYPE) {
        buttonStep1();
      } else {
        if (row_ == 4) {
          buttonStep2();
        }
      }
    } else if (enterDown_ && !(mask & BM_ENTER)) {
      enterDown_ = false;
      isDirty_ = true;
    }
    return;
  }

  const uint8_t rowCount = (step_ == STEP_TYPE) ? GT_COUNT : DR_COUNT;
  // rows 0..rowCount-1 are the fields, rowCount is the button row
  const uint8_t focusCount = rowCount + 1;

  if (mask == BM_ENTER) {
    enterDown_ = true;
  }
  
  if (enterDown_) {
    if (mask & BM_LEFT || mask & BM_RIGHT) {
      const int delta = (mask & BM_LEFT) ? -1 : 1;
      changeValue(delta);
    } 
  } else {
    if (mask == BM_UP || mask == BM_DOWN) {
      row_ = (row_ + focusCount + (mask == BM_UP ? -1 : 1)) % focusCount;
    } else if  (mask & BM_LEFT || mask & BM_RIGHT) {
      btn_ = (btn_ == BTN_LEFT) ? BTN_RIGHT : BTN_LEFT;
    }
  }

  isDirty_ = true;
}
