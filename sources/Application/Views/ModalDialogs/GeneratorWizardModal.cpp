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
#include "System/Console/nanoprintf.h"
#include <algorithm>
#include <new>

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

GeneratorWizardModal::GeneratorWizardModal(View &view, InstrumentBank *bank) : WizardModal(view), bank_(bank) {
}

GeneratorWizardModal::~GeneratorWizardModal() {
}

void GeneratorWizardModal::Destroy() {
  this->~GeneratorWizardModal();
  inUse_ = false;
}

// --- data --------------------------------------------------------------------

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

// --- WizardModal hooks -------------------------------------------------------

const char *GeneratorWizardModal::GetTitle() {
  return "PhraseGen";
}

uint8_t GeneratorWizardModal::NumSteps() const {
  return NUM_STEPS;
}

WizardModal::StepInfo GeneratorWizardModal::GetStepInfo(uint8_t step) {
  switch (step) {
    case STEP_TYPE:
      return {"Select generator"};
    case STEP_TARGET:
      return {"Select target"};
    case STEP_INSTRUMENT:
      return {creating_ ? "Create instrument" : "Select instrument"};
    default:
      return {"Configure generator", nullptr, "Generate"};
  }
}

uint8_t GeneratorWizardModal::RowCount(uint8_t step) {
  switch (step) {
    case STEP_TYPE:
      return GT_COUNT;
    case STEP_TARGET:
      return kTargetCount;
    case STEP_INSTRUMENT:
      return entryCount_;
    default:
      return DR_COUNT;
  }
}

void GeneratorWizardModal::OnEnterStep(uint8_t step, bool forward) {
  switch (step) {
    case STEP_TYPE:
      row_ = type_;
      break;
    case STEP_TARGET:
      row_ = target_;
      break;
    case STEP_INSTRUMENT:
      if (forward) {
        buildInstrumentList();
      } else {
        // coming back: focus the instrument picked before
        for (int i = 0; i < entryCount_; i++) {
          if (entries_[i] == instrumentId_) {
            row_ = i;
          }
        }
      }
      break;
    default:
      break;
  }
}

void GeneratorWizardModal::OnRowFocused(uint8_t step, uint8_t row) {
  if (step == STEP_TARGET) {
    target_ = row; // the choice follows the highlight
  }
}

void GeneratorWizardModal::OnValueChange(uint8_t step, uint8_t row, int delta) {
  if (step != STEP_DRUM) {
    return;
  }

  // the choice rows only step by one, whatever the arrow's delta
  const int dir = delta > 0 ? 1 : -1;

  switch (row) {
    case DR_STYLE:
      style_ = (style_ + kStyleCount + dir) % kStyleCount;
      break;
    case DR_FEEL:
      feel_ = (feel_ + kFeelCount + dir) % kFeelCount;
      break;
    case DR_DENSITY:
      density_ = std::clamp(density_ + delta, 0, 255);
      break;
    case DR_VARIATION:
      variation_ = std::clamp(variation_ + delta, 0, 255);
      break;
  }
}

bool GeneratorWizardModal::OnNext(uint8_t step, uint8_t row) {
  switch (step) {
    case STEP_TYPE:
      if (!isImplemented(static_cast<GeneratorType>(row))) {
        return false;
      }
      type_ = static_cast<GeneratorType>(row);
      return true;
    case STEP_TARGET:
      target_ = row;
      return true;
    case STEP_INSTRUMENT:
      if (row >= entryCount_) {
        return false; // nothing to pick, e.g. no free instrument
      }
      instrumentId_ = entries_[row];
      return true;
    default:
      return true;
  }
}

bool GeneratorWizardModal::OnAccept() {
  // a new instrument is set up only now, so backing out leaves the bank alone
  if (creating_) {
    return bank_->AssignInstrumentToSlot(instrumentType(), instrumentId_) == InstrumentAssignResult::Success;
  }
  return true;
}

// --- drawing -----------------------------------------------------------------

void GeneratorWizardModal::DrawStep(uint8_t step) {
  switch (step) {
    case STEP_TYPE:
      for (int i = 0; i < GT_COUNT; i++) {
        drawListItem(i, kTypeNames[i], row_ == i);
      }
      break;
    case STEP_TARGET:
      for (int i = 0; i < kTargetCount; i++) {
        drawListItem(i, kTargetNames[i], row_ == i);
      }
      break;
    case STEP_INSTRUMENT:
      drawInstrumentStep();
      break;
    default:
      drawDrumStep();
      break;
  }
}

void GeneratorWizardModal::drawInstrumentStep() {
  if (entryCount_ == 0) {
    drawPanelText(0, "No free instrument");
  }

  for (int i = 0; i < kVisibleRows && scroll_ + i < entryCount_; i++) {
    const uint8_t id = entries_[scroll_ + i];
    char label[SCREEN_WIDTH];
    if (creating_) {
      npf_snprintf(label, sizeof(label), "%02X new %s", id, InstrumentTypeNames[instrumentType()].full);
    } else {
      npf_snprintf(label, sizeof(label), "%02X %s", id, bank_->GetInstrument(id)->GetDisplayName().c_str());
    }
    drawListItem(i, label, row_ == scroll_ + i);
  }
}

void GeneratorWizardModal::drawDrumStep() {
  char buf[8];
  drawLabeledField(DR_STYLE, "Style", kStyleNames[style_], row_ == DR_STYLE);
  drawLabeledField(DR_FEEL, "Feel", kFeelNames[feel_], row_ == DR_FEEL);

  npf_snprintf(buf, sizeof(buf), "%02X", density_);
  drawLabeledField(DR_DENSITY, "Density", buf, row_ == DR_DENSITY);

  npf_snprintf(buf, sizeof(buf), "%02X", variation_);
  drawLabeledField(DR_VARIATION, "Variation", buf, row_ == DR_VARIATION);
}
