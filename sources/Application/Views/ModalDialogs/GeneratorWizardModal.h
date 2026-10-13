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
#include "Application/Views/BaseClasses/ModalView.h"
#include <stdint.h>

// Multi step wizard that collects the parameters for the phrase generators.
//
//   Step 1: generator type (Drum / Melody / Bassline / Chords)
//   Step 2: type specific settings (currently only Drum is implemented)
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

  static GeneratorWizardModal *Create(View &view);
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
  // style/feel/density/variation are filled in, the caller supplies the rest
  // (steps, phraseCount, voices).
  DrumGenerator::Settings GetDrumSettings() const;

private:
  explicit GeneratorWizardModal(View &view);

  enum Step : uint8_t { STEP_TYPE = 0, STEP_DRUM };
  enum DrumRow : uint8_t { DR_STYLE = 0, DR_FEEL, DR_DENSITY, DR_VARIATION, DR_COUNT };

  void DrawStep1();
  void DrawStep2();

  void buttonStep1();
  void buttonStep2();

  static bool isImplemented(GeneratorType type);
  void drawRow(int y, const char *label, const char *value, bool selected, bool enabled = true);
  void drawField(int x, int y, const char *text, int width, bool focused, char indicator = ' ');
  void drawButtons(const char *left, const char *right);
  void changeValue(int delta);
  void goBack();
  void activatePrimary();
  bool isButtonRow() const {
    return row_ == ((step_ == STEP_TYPE) ? GT_COUNT : DR_COUNT);
  }

  static bool inUse_;
  static void *storage_;

  enum Button : uint8_t { BTN_LEFT = 0, BTN_RIGHT };

  Step step_ = STEP_TYPE;
  Button btn_ = BTN_LEFT;
  uint16_t lastMask_ = 0;
  bool enterDown_ = false;
  uint8_t row_ = 0;
  GeneratorType type_ = GT_DRUM;

  uint8_t style_ = 0;
  uint8_t feel_ = 0;
  uint8_t density_ = 128;
  uint8_t variation_ = 128;
};

#endif // _GENERATOR_WIZARD_MODAL_H_
