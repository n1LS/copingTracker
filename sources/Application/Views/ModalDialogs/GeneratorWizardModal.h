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
#include "Application/Views/ModalDialogs/WizardModal.h"
#include <stdint.h>

class InstrumentBank;
class I_Instrument;

// Wizard that collects the parameters for the phrase generators.
//
//   Step 1: generator type (Drum / Melody / Bassline / Chords)
//   Step 2: target instrument (Drum Instrument / LSDJ Kit / Sliced Sample / Sample GM Set)
//   Step 3: instrument of that type, one of the allocated ones, or the first
//           free slot set up as new when there is none
//   Step 4: type specific settings (currently only Drum is implemented)
//
// Layout, navigation and button handling live in WizardModal.
class GeneratorWizardModal : public WizardModal {
public:
  enum ReturnCode { GWM_CANCEL = WM_CANCEL, GWM_ACCEPT = WM_ACCEPT };
  enum GeneratorType : uint8_t { GT_DRUM = 0, GT_MELODY, GT_BASSLINE, GT_CHORDS, GT_COUNT };

  static GeneratorWizardModal *Create(View &view, InstrumentBank *bank);
  virtual ~GeneratorWizardModal();

  virtual void Destroy() override;

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

protected:
  virtual const char *GetTitle() override;
  virtual uint8_t NumSteps() const override;
  virtual StepInfo GetStepInfo(uint8_t step) override;
  virtual uint8_t RowCount(uint8_t step) override;
  virtual void DrawStep(uint8_t step) override;
  virtual void OnEnterStep(uint8_t step, bool forward) override;
  virtual void OnRowFocused(uint8_t step, uint8_t row) override;
  virtual void OnValueChange(uint8_t step, uint8_t row, int delta) override;
  virtual bool OnNext(uint8_t step, uint8_t row) override;
  virtual bool OnAccept() override;

private:
  GeneratorWizardModal(View &view, InstrumentBank *bank);

  enum Step : uint8_t { STEP_TYPE = 0, STEP_TARGET, STEP_INSTRUMENT, STEP_DRUM, NUM_STEPS };
  static constexpr uint8_t kTargetCount = 4;
  enum DrumRow : uint8_t { DR_STYLE = 0, DR_FEEL, DR_DENSITY, DR_VARIATION, DR_COUNT };

  void drawInstrumentStep();
  void drawDrumStep();

  // InstrumentType the selected target is played by
  InstrumentType instrumentType() const;
  bool matchesTarget(I_Instrument *instrument) const;
  void buildInstrumentList();

  static bool isImplemented(GeneratorType type);

  static bool inUse_;
  static void *storage_;

  GeneratorType type_ = GT_DRUM;
  uint8_t target_ = 0; // DrumGenerator::Target

  InstrumentBank *bank_;
  uint8_t instrumentId_ = NO_INSTRUMENT;
  // Instrument step: slot ids of the listed instruments. When creating_ is set
  // the single entry is a free slot that gets set up on accept.
  uint8_t entries_[MAX_INSTRUMENT_COUNT];
  uint8_t entryCount_ = 0;
  bool creating_ = false;

  uint8_t style_ = 0;
  uint8_t feel_ = 0;
  uint8_t density_ = 128;
  uint8_t variation_ = 128;
};

#endif // _GENERATOR_WIZARD_MODAL_H_
