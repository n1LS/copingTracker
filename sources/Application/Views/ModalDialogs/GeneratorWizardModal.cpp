/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "GeneratorWizardModal.h"
#include "Application/AppWindow.h"
#include "Application/Instruments/InstrumentBank.h"
#include "Foundation/Constants/GraphicCharacters.h"
#include "Foundation/Constants/SpecialCharacters.h"
#include "System/Console/nanoprintf.h"
#include <algorithm>
#include <new>
#include <string.h>

#define WINDOW_W 26
#define WINDOW_H 14
#define LABEL_X 2
#define VALUE_X 12
#define ROWS_Y 4
#define DENSITY_STEP 16
#define VISIBLE_ROWS 4
#define VALUE_W (WINDOW_W - VALUE_X - 4)

static const char *kTypeNames[GeneratorWizardModal::GT_COUNT] = {"Drum", "Melody", "Bassline", "Chords"};
static const char *kTargetNames[] = {"Drum Instr.", "LSDJ Kit", "Sliced Smpl", "Sample GM"};
static const char *kStyleNames[] = {"Straight", "Sparse", "Syncopated", "Broken", "Driving", "HalfTime"};
static const char *kFeelNames[] = {"Straight", "Triplet"};

static const uint8_t kStyleCount = sizeof(kStyleNames) / sizeof(kStyleNames[0]);
static const uint8_t kFeelCount = sizeof(kFeelNames) / sizeof(kFeelNames[0]);

bool GeneratorWizardModal::inUse_ = false;
alignas(GeneratorWizardModal) static unsigned char GeneratorWizardModalStorage[sizeof(GeneratorWizardModal)];
void *GeneratorWizardModal::storage_ = GeneratorWizardModalStorage;

GeneratorWizardModal *GeneratorWizardModal::Create(View &view, InstrumentBank *bank) {
  if (inUse_) {
    auto *existing = reinterpret_cast<GeneratorWizardModal *>(storage_);
    existing->~GeneratorWizardModal();
    inUse_ = false;
  }
  inUse_ = true;
  return new (storage_) GeneratorWizardModal(view, bank);
}

GeneratorWizardModal::GeneratorWizardModal(View &view, InstrumentBank *bank) : ModalView(view), bank_(bank) {
}

GeneratorWizardModal::~GeneratorWizardModal() {
}

void GeneratorWizardModal::Destroy() {
  this->~GeneratorWizardModal();
  inUse_ = false;
}

InstrumentType GeneratorWizardModal::instrumentType() const {
  switch (static_cast<DrumGenerator::Target>(target_)) {
    case DrumGenerator::Target::DrumInstrument:
      return IT_DRUM;
    case DrumGenerator::Target::LSDJKit:
      return IT_LSDJKIT;
    default: // sliced sample and GM set are both sample instruments
      return IT_SAMPLE;
  }
}

// Sample instruments are told apart by their setup: sliced ones have slices
// to play back, GM set ones have a GM instrument selected.
bool GeneratorWizardModal::matchesTarget(I_Instrument *instrument) const {
  if (instrument->GetType() != instrumentType()) {
    return false;
  }

  const auto target = static_cast<DrumGenerator::Target>(target_);
  if (target == DrumGenerator::Target::SlicedSample) {
    return static_cast<SampleInstrument *>(instrument)->HasSlicesForPlayback();
  }
  if (target == DrumGenerator::Target::SampleGMSet) {
    Variable *gm = instrument->FindVariable(Token::SampleInstrumentGMInstrument);
    return gm && gm->GetInt() != NO_GM_INSTRUMENT;
  }
  return true;
}

void GeneratorWizardModal::buildInstrumentList() {
  entryCount_ = 0;
  creating_ = false;

  for (int i = 0; i < MAX_INSTRUMENT_COUNT; i++) {
    if (matchesTarget(bank_->GetInstrument(i))) {
      entries_[entryCount_++] = i;
    }
  }

  if (entryCount_ == 0) {
    const uint16_t slot = bank_->GetNextFreeInstrumentSlotId();
    if (slot != NO_MORE_INSTRUMENT) {
      entries_[entryCount_++] = slot;
      creating_ = true;
    }
  }
}

// Keeps the focused entry inside the visible window of the list.
void GeneratorWizardModal::updateScroll() {
  if (row_ >= entryCount_) {
    return; // button row
  }
  if (row_ < scroll_) {
    scroll_ = row_;
  } else if (row_ >= scroll_ + VISIBLE_ROWS) {
    scroll_ = row_ - (VISIBLE_ROWS - 1);
  }
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
  s.target = GetTarget();
  s.instrument = instrumentId_;
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

  button_ = BTN_LEFT;
  drawButtons(" Cancel ", nullptr);
}

// Right aligned progress bar in the title bar. step is the zero based current
// step, so the bar is full on the last one.
void GeneratorWizardModal::drawProgress(uint8_t step, uint8_t steps) {
  char bar[7];
  horizontal_bar_graph_6(bar, (uint8_t)((step + 1) * 60 / steps));

  SetBackgroundColor(Theme::Dialog::Title::bg);
  SetColor(Theme::Dialog::Title::fg);
  DrawString(WINDOW_W - 1 - 6, 1, bar, fGraphic);
}

void GeneratorWizardModal::DrawStepTarget() {
  SetBackgroundColor(Theme::Dialog::bg);
  SetColor(Theme::Dialog::fg);
  DrawString(LABEL_X, 3, "Select target");

  DrawFilledBorder(1, 4, WINDOW_W - 2, 6, Theme::Dialog::Panel::bg, true);

  for (int i = 0; i < kTargetCount; i++) {
    drawField(LABEL_X, ROWS_Y + 1 + i, kTargetNames[i], WINDOW_W - 6, row_ == i, CHAR(char_button_enter_s));
  }

  button_ = BTN_LEFT;
  drawButtons(" Back ", nullptr);
}

void GeneratorWizardModal::DrawStepInstrument() {
  SetBackgroundColor(Theme::Dialog::bg);
  SetColor(Theme::Dialog::fg);
  DrawString(LABEL_X, 3, creating_ ? "Create instrument" : "Select instrument");

  DrawFilledBorder(1, 4, WINDOW_W - 2, 6, Theme::Dialog::Panel::bg, true);

  if (entryCount_ == 0) {
    SetBackgroundColor(Theme::Dialog::Panel::bg);
    SetColor(Theme::Dialog::fg);
    DrawString(LABEL_X, ROWS_Y + 1, "No free instrument");
  }

  for (int i = 0; i < VISIBLE_ROWS && scroll_ + i < entryCount_; i++) {
    const uint8_t id = entries_[scroll_ + i];
    char label[WINDOW_W];
    if (creating_) {
      npf_snprintf(label, sizeof(label), "%02X new %s", id, InstrumentTypeNames[instrumentType()].full);
    } else {
      npf_snprintf(label, sizeof(label), "%02X %s", id, bank_->GetInstrument(id)->GetDisplayName().c_str());
    }
    drawField(LABEL_X, ROWS_Y + 1 + i, label, WINDOW_W - 6, row_ == scroll_ + i, CHAR(char_button_enter_s));
  }

  button_ = BTN_LEFT;
  drawButtons(" Back ", nullptr);
}

void GeneratorWizardModal::DrawStep2() {
  SetBackgroundColor(Theme::Dialog::bg);
  SetColor(Theme::Dialog::fg);
  DrawString(LABEL_X, 3, "Configure generator");

  DrawFilledBorder(1, 4, WINDOW_W - 2, 6, Theme::Dialog::Panel::bg, true);

  char buf[8];
  drawRow(ROWS_Y + 1 + DR_STYLE, "Style", kStyleNames[style_], row_ == DR_STYLE);
  drawRow(ROWS_Y + 1 + DR_FEEL, "Feel", kFeelNames[feel_], row_ == DR_FEEL);

  npf_snprintf(buf, sizeof(buf), "%02X", density_);
  drawRow(ROWS_Y + 1 + DR_DENSITY, "Density", buf, row_ == DR_DENSITY);

  npf_snprintf(buf, sizeof(buf), "%02X", variation_);
  drawRow(ROWS_Y + 1 + DR_VARIATION, "Variation", buf, row_ == DR_VARIATION);
  drawButtons(" Back ", " Generate ");
}

void GeneratorWizardModal::DrawView() {
  SetBackgroundColor(Theme::Dialog::bg);

  left_ = (SCREEN_WIDTH - WINDOW_W) / 2;
  top_ = (SCREEN_HEIGHT - WINDOW_H) / 2;

  DrawWindow(0, 0, WINDOW_W, WINDOW_H, "PhraseGen");
  drawProgress(step_, NUM_STEPS);

  switch (step_) {
    case STEP_TYPE:
      DrawStep1();
      break;
    case STEP_TARGET:
      DrawStepTarget();
      break;
    case STEP_INSTRUMENT:
      DrawStepInstrument();
      break;
    default:
      DrawStep2();
      break;
  }
}

// Secondary button (Cancel / Back) on the left, primary (Next / Generate) on
// the right.
void GeneratorWizardModal::drawButtons(const char *left, const char *right) {
  const int y = WINDOW_H - 3;
  const bool onButtons = isButtonRow();
  const bool leftSel = onButtons && button_ == BTN_LEFT;
  const bool rightSel = onButtons && button_ == BTN_RIGHT;

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
      density_ = std::clamp(density_ + delta, 0, 255);
      break;
    case DR_VARIATION:
      variation_ = std::clamp(variation_ + delta, 0, 255);
      break;
  }
}

void GeneratorWizardModal::buttonStep1() {
  if (row_ < 4) {
    switch (row_) {
      case 0:
        step_ = STEP_TARGET;
        row_ = target_;
        button_ = BTN_RIGHT;
        break;
    }
  } else {
    EndModal(GWM_CANCEL);
  }
}

void GeneratorWizardModal::buttonStepTarget() {
  if (isButtonRow()) {
    step_ = STEP_TYPE;
    row_ = type_;
    return;
  }
  // ENTER on a target row continues with that target
  target_ = row_;
  buildInstrumentList();
  step_ = STEP_INSTRUMENT;
  row_ = 0;
  scroll_ = 0;
  button_ = BTN_LEFT;
}

void GeneratorWizardModal::buttonStepInstrument() {
  if (isButtonRow()) {
    step_ = STEP_TARGET;
    row_ = target_;
    return;
  }
  instrumentId_ = entries_[row_];
  step_ = STEP_DRUM;
  row_ = 0;
  button_ = BTN_RIGHT;
}

void GeneratorWizardModal::buttonStep2() {
  if (button_ == BTN_LEFT) {
    step_ = STEP_INSTRUMENT;
    row_ = 0;
    for (int i = 0; i < entryCount_; i++) {
      if (entries_[i] == instrumentId_) {
        row_ = i;
      }
    }
    scroll_ = 0;
    updateScroll();
  } else {
    // a new instrument is set up only now, so backing out leaves the bank alone
    if (creating_ &&
        bank_->AssignInstrumentToSlot(instrumentType(), instrumentId_) != InstrumentAssignResult::Success) {
      EndModal(GWM_CANCEL);
      return;
    }
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
      } else if (step_ == STEP_TARGET) {
        buttonStepTarget();
      } else if (step_ == STEP_INSTRUMENT) {
        buttonStepInstrument();
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

  // rows 0..rowCount()-1 are the fields, rowCount() is the button row
  const uint8_t focusCount = rowCount() + 1;

  if (mask == BM_ENTER) {
    enterDown_ = true;
  }

  if (enterDown_) {
    if (mask & BM_DIRECTIONAL) {
      if (mask & BM_RIGHT) {
        changeValue(1);
      } else if (mask & BM_LEFT) {
        changeValue(-1);
      } else {
        changeValue((mask & BM_DOWN) ? -16 : 16);
      }
    }
  } else {
    if (mask == BM_UP || mask == BM_DOWN) {
      row_ = (row_ + focusCount + (mask == BM_UP ? -1 : 1)) % focusCount;
      if (step_ == STEP_TARGET && !isButtonRow()) {
        target_ = row_; // Next uses whatever was highlighted last
      }
      updateScroll();
    } else if (isButtonRow() && (mask & BM_LEFT || mask & BM_RIGHT)) {
      button_ = (button_ == BTN_LEFT) ? BTN_RIGHT : BTN_LEFT;
    }
  }

  isDirty_ = true;
}
