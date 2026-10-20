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
#include "Application/Model/Scale.h"
#include "Foundation/Constants/SpecialCharacters.h"
#include "System/Console/nanoprintf.h"
#include <algorithm>
#include <new>
#include <string.h>

static const char *kTypeNames[GeneratorWizardModal::GT_COUNT] = {"Drum", "Melody", "Bassline", "Chords"};
static const char *kTargetNames[] = {"Drum Instr.", "LSDJ Kit", "Sliced Smpl", "Sample GM"};
static const char *kStyleNames[] = {"Straight", "Sparse", "Syncopated", "Broken", "Driving", "HalfTime"};
static const char *kFeelNames[] = {"Straight", "Triplet"};

static const char *kBassStyleNames[] = {"Funky", "Octave", "Pulse"};
static const char *kKeyNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
static const char *kScaleNames[] = {"Minor", "Major", "Pent Minor", "Pent Major", "Dorian", "Blues"};

static const int kChordWidth = 9;
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
  for (auto &phrase : chords_) {
    for (auto &slot : phrase) {
      slot = BasslineGenerator::kCarry;
    }
  }
  chords_[0][0] = 0; // first chord is the tonic
}

void GeneratorWizardModal::SetPhraseCount(uint8_t count) {
  phraseCount_ = std::clamp<int>(count, 1, BasslineGenerator::kMaxPhrases);
}

void GeneratorWizardModal::PresetKeyAndScale(uint8_t root, int scale) {
  using BasslineGenerator::Scale;
  // project scale name -> generator scale
  static const struct {
    const char *name;
    Scale scale;
  } kMap[] = {
      {"Aeolian mode (minor)", Scale::Minor},
      {"Ionian mode (major)", Scale::Major},
      {"Minor pentatonic", Scale::PentMinor},
      {"Major pentatonic", Scale::PentMajor},
      {"Dorian", Scale::Dorian},
      {"Blues", Scale::Blues},
  };

  if (root < 12) {
    bassKey_ = root;
  }
  if (scale < 0 || scale >= numScales) {
    return;
  }
  for (const auto &m : kMap) {
    if (strcmp(scaleNames[scale], m.name) == 0) {
      bassScale_ = static_cast<uint8_t>(m.scale);
    }
  }
}

GeneratorWizardModal::Step GeneratorWizardModal::kind(uint8_t step) const {
  static const Step drum[] = {STEP_TYPE, STEP_TARGET, STEP_INSTRUMENT, STEP_DRUM};
  static const Step bass[] = {STEP_TYPE, STEP_INSTRUMENT, STEP_BASS, STEP_CHORDS};
  return (type_ == GT_BASSLINE ? bass : drum)[step];
}

GeneratorWizardModal::~GeneratorWizardModal() {
}

void GeneratorWizardModal::Destroy() {
  this->~GeneratorWizardModal();
  inUse_ = false;
}

// --- data --------------------------------------------------------------------

InstrumentType GeneratorWizardModal::instrumentType() const {
  if (type_ == GT_BASSLINE) {
    return IT_CHIPTUNE; // what a new instrument is created as
  }
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
  if (type_ == GT_BASSLINE) {
    return instrument->GetType() == IT_CHIPTUNE || instrument->GetType() == IT_STACK ||
           instrument->GetType() == IT_SAMPLE;
  }
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
  return type == GT_DRUM || type == GT_BASSLINE;
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

BasslineGenerator::Settings GeneratorWizardModal::GetBasslineSettings() const {
  BasslineGenerator::Settings s{};
  s.style = static_cast<BasslineGenerator::Style>(bassStyle_);
  s.key = bassKey_;
  s.scale = static_cast<BasslineGenerator::Scale>(bassScale_);
  s.phraseCount = phraseCount_;
  s.density = bassDensity_;
  s.variation = bassVariation_;
  s.movement = bassMovement_;
  s.instrument = instrumentId_;
  for (int p = 0; p < BasslineGenerator::kMaxPhrases; p++) {
    for (int q = 0; q < BasslineGenerator::kChordSlots; q++) {
      s.chords[p][q] = chords_[p][q];
    }
  }
  return s;
}

// --- WizardModal hooks -------------------------------------------------------

const char *GeneratorWizardModal::GetTitle() {
  return "PhraseGen";
}

uint8_t GeneratorWizardModal::NumSteps() const {
  return 4;
}

WizardModal::StepInfo GeneratorWizardModal::GetStepInfo(uint8_t step) {
  switch (kind(step)) {
    case STEP_TYPE:
      return {"Select generator"};
    case STEP_TARGET:
      return {"Select target"};
    case STEP_INSTRUMENT:
      return {creating_ ? "Create instrument" : "Select instrument"};
    case STEP_BASS:
      return {"Configure bassline", nullptr, "Next"};
    case STEP_CHORDS:
      return {"Chords (per quarter)", nullptr, "Generate"};
    default:
      return {"Configure generator", nullptr, "Generate"};
  }
}

uint8_t GeneratorWizardModal::RowCount(uint8_t step) {
  switch (kind(step)) {
    case STEP_TYPE:
      return GT_COUNT;
    case STEP_TARGET:
      return kTargetCount;
    case STEP_INSTRUMENT:
      return entryCount_;
    case STEP_BASS:
      return BR_COUNT;
    case STEP_CHORDS:
      return phraseCount_ * BasslineGenerator::kChordSlots;
    default:
      return DR_COUNT;
  }
}

void GeneratorWizardModal::OnEnterStep(uint8_t step, bool forward) {
  switch (kind(step)) {
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
  if (kind(step) == STEP_TARGET) {
    target_ = row; // the choice follows the highlight
  }
}

void GeneratorWizardModal::OnValueChange(uint8_t step, uint8_t row, int delta) {
  const Step which = kind(step);
  if (which != STEP_DRUM && which != STEP_BASS && which != STEP_CHORDS) {
    return;
  }

  // the choice rows only step by one, whatever the arrow's delta
  const int dir = delta > 0 ? 1 : -1;

  if (which == STEP_BASS) {
    switch (row) {
      case BR_STYLE:
        bassStyle_ = (bassStyle_ + (int)BasslineGenerator::Style::Count + dir) % (int)BasslineGenerator::Style::Count;
        break;
      case BR_KEY:
        bassKey_ = (bassKey_ + 12 + dir) % 12;
        break;
      case BR_SCALE:
        bassScale_ = (bassScale_ + (int)BasslineGenerator::Scale::Count + dir) % (int)BasslineGenerator::Scale::Count;
        break;
      case BR_DENSITY:
        bassDensity_ = std::clamp(bassDensity_ + delta, 0, 255);
        break;
      case BR_MOVEMENT:
        bassMovement_ = std::clamp(bassMovement_ + delta, 0, 255);
        break;
      case BR_VARIATION:
        bassVariation_ = std::clamp(bassVariation_ + delta, 0, 255);
        break;
    }
    return;
  }

  if (which == STEP_CHORDS) {
    // carry, then the degrees of the scale
    const int values = BasslineGenerator::scaleSize(static_cast<BasslineGenerator::Scale>(bassScale_)) + 1;
    uint8_t &slot = chords_[row / BasslineGenerator::kChordSlots][row % BasslineGenerator::kChordSlots];
    const int current = slot == BasslineGenerator::kCarry ? 0 : slot + 1;
    const int next = (current + values + dir) % values;
    slot = next == 0 ? BasslineGenerator::kCarry : next - 1;
    return;
  }

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
  switch (kind(step)) {
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
  switch (kind(step)) {
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
    case STEP_BASS:
      drawBassStep();
      break;
    case STEP_CHORDS:
      drawChordStep();
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
  drawListScrollBar(entryCount_);
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

void GeneratorWizardModal::drawBassStep() {
  const char *labels[BR_COUNT] = {"Style", "Key", "Scale", "Density", "Movement", "Variation"};
  char buf[8];

  for (int i = 0; i < kVisibleRows && scroll_ + i < BR_COUNT; i++) {
    const int row = scroll_ + i;
    const char *value = buf;
    switch (row) {
      case BR_STYLE:
        value = kBassStyleNames[bassStyle_];
        break;
      case BR_KEY:
        value = kKeyNames[bassKey_];
        break;
      case BR_SCALE:
        value = kScaleNames[bassScale_];
        break;
      case BR_DENSITY:
        npf_snprintf(buf, sizeof(buf), "%02X", bassDensity_);
        break;
      case BR_MOVEMENT:
        npf_snprintf(buf, sizeof(buf), "%02X", bassMovement_);
        break;
      default:
        npf_snprintf(buf, sizeof(buf), "%02X", bassVariation_);
        break;
    }
    drawLabeledField(i, labels[row], value, row_ == row);
  }
  drawListScrollBar(BR_COUNT);
}

void GeneratorWizardModal::drawChordStep() {
  const int rows = phraseCount_ * BasslineGenerator::kChordSlots;
  const auto scale = static_cast<BasslineGenerator::Scale>(bassScale_);

  for (int i = 0; i < kVisibleRows && scroll_ + i < rows; i++) {
    const int row = scroll_ + i;
    const int phrase = row / BasslineGenerator::kChordSlots;
    const int quarter = row % BasslineGenerator::kChordSlots;
    const uint8_t degree = chords_[phrase][quarter];

    char label[8];
    npf_snprintf(label, sizeof(label), "P%d Q%d", phrase + 1, quarter + 1);

    // the value area holds kChordWidth characters (the last cell of a focused
    // field is taken by the indicator): numeral left, root note right aligned
    char value[kChordWidth + 1];
    memset(value, ' ', kChordWidth);
    value[kChordWidth] = 0;
    if (degree == BasslineGenerator::kCarry) {
      memcpy(value, char_border_single_vertical_s, 1);
    } else {
      char chord[12];
      BasslineGenerator::chordLabel(bassKey_, scale, degree, chord, sizeof(chord));
      const char *space = strchr(chord, ' ');
      const int numeralLen = space - chord;
      const int noteLen = strlen(space + 1);
      memcpy(value, chord, numeralLen);
      memcpy(value + kChordWidth - noteLen, space + 1, noteLen);
    }
    drawLabeledField(i, label, value, row_ == row);
  }
  drawListScrollBar(rows);
}
