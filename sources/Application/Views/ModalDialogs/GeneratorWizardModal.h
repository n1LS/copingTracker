/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _GENERATOR_WIZARD_MODAL_H_
#define _GENERATOR_WIZARD_MODAL_H_

#include "Application/Model/Generators/DrumPhraseGenerator.h"
#include "Application/Model/Song.h"
#include "Application/Views/BaseClasses/ModalView.h"
#include <stdint.h>

class InstrumentBank;
class I_Instrument;

// Multi step wizard that collects the parameters for the phrase generators.
//
//   Step 1: generator type (Drum / Melody / Bassline / Chords)
//   Step 2: target instrument (Drum Instrument / LSDJ Kit / Sliced Sample / Sample GM Set)
//   Step 3: instrument of that type, one of the allocated ones, or the first
//           free slot set up as new when there is none
//   Step 4: type specific settings (currently only Drum is implemented)
//
// Controls: UP/DOWN move between the rows and the button row, LEFT/RIGHT change
// a value (or pick a button on the button row). ENTER, acted on when released,
// triggers the primary button (Next / Generate) from a row, or the focused
// button from the button row. The secondary button is Cancel on the first step
// and Back on the second.
class GeneratorWizardModal : public ModalView {
public:
  enum ReturnCode { GWM_CANCEL = 0, GWM_ACCEPT = 1 };
  enum GeneratorType : uint8_t { GT_DRUM = 0, GT_MELODY, GT_BASSLINE, GT_CHORDS, GT_COUNT };

  static GeneratorWizardModal *Create(View &view, InstrumentBank *bank);
  virtual ~GeneratorWizardModal();

  virtual void Destroy() override;
  virtual void DrawView() override;
  virtual void OnPlayerUpdate(PlayerEventType, unsigned int) override {
  }
  virtual void OnFocus() override {
  }
  virtual void ProcessButtonMask(uint16_t mask, bool pressed) override;
  virtual void AnimationUpdate() override {
  }

  // Valid once the modal finished with GWM_ACCEPT.
  GeneratorType GetType() const {
    return type_;
  }
  // Target instrument picked in step 2.
  DrumGenerator::Target GetTarget() const {
    return static_cast<DrumGenerator::Target>(target_);
  }
  // Instrument slot the phrases are written for. A new instrument is only
  // allocated when the wizard is accepted, so this is valid after GWM_ACCEPT.
  uint8_t GetInstrumentId() const {
    return instrumentId_;
  }
  // style/feel/density/variation/target/instrument are filled in, the caller supplies the rest
  // (steps, phraseCount, voices).
  DrumGenerator::Settings GetDrumSettings() const;

private:
  GeneratorWizardModal(View &view, InstrumentBank *bank);

  enum Step : uint8_t { STEP_TYPE = 0, STEP_TARGET, STEP_INSTRUMENT, STEP_DRUM, NUM_STEPS };
  static constexpr uint8_t kTargetCount = 4;
  enum DrumRow : uint8_t { DR_STYLE = 0, DR_FEEL, DR_DENSITY, DR_VARIATION, DR_COUNT };

  void drawProgress(uint8_t step, uint8_t steps);
  void DrawStep1();
  void DrawStepTarget();
  void DrawStepInstrument();
  void DrawStep2();

  void buttonStep1();
  void buttonStepTarget();
  void buttonStepInstrument();
  void buttonStep2();

  // InstrumentType the selected target is played by
  InstrumentType instrumentType() const;
  bool matchesTarget(I_Instrument *instrument) const;
  void buildInstrumentList();
  void updateScroll();

  static bool isImplemented(GeneratorType type);
  void drawRow(int y, const char *label, const char *value, bool selected, bool enabled = true);
  void drawField(int x, int y, const char *text, int width, bool focused, char indicator = ' ');
  void drawButtons(const char *left, const char *right);
  void changeValue(int delta);
  void goBack();
  void activatePrimary();
  // Number of selectable rows of the current step, the button row follows.
  uint8_t rowCount() const {
    switch (step_) {
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
  bool isButtonRow() const {
    return row_ == rowCount();
  }

  static bool inUse_;
  static void *storage_;

  enum Button : uint8_t { BTN_LEFT = 0, BTN_RIGHT };

  Step step_ = STEP_TYPE;
  Button button_ = BTN_LEFT;
  uint16_t lastMask_ = 0;
  bool enterDown_ = false;
  uint8_t row_ = 0;
  GeneratorType type_ = GT_DRUM;
  uint8_t target_ = 0; // DrumGenerator::Target

  InstrumentBank *bank_;
  uint8_t instrumentId_ = NO_INSTRUMENT;
  // Instrument step: slot ids of the listed instruments. When creating_ is set
  // the single entry is a free slot that gets set up on accept.
  uint8_t entries_[MAX_INSTRUMENT_COUNT];
  uint8_t entryCount_ = 0;
  uint8_t scroll_ = 0;
  bool creating_ = false;

  uint8_t style_ = 0;
  uint8_t feel_ = 0;
  uint8_t density_ = 128;
  uint8_t variation_ = 128;
};

#endif // _GENERATOR_WIZARD_MODAL_H_
