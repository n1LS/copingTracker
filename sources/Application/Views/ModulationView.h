/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _MODULATION_VIEW_H_
#define _MODULATION_VIEW_H_

#include "Application/Instruments/I_Instrument.h"
#include "BaseClasses/UIIntVarField.h"
#include "FieldView.h"
#include "ViewData.h"

class I_Instrument;

// Second layer screen under the instrument screen, holds the routing and
// modulation settings of the current instrument
class ModulationView : public FieldView {
public:
  ModulationView(GUIWindow &w, ViewData *data);
  virtual ~ModulationView();
  void Reset();

  virtual void ButtonDown(uint16_t newButton, uint16_t mask) override;
  bool navigationDown(uint16_t direction) override;
  virtual void DrawView() override;
  virtual void OnPlayerUpdate(PlayerEventType, unsigned int) override {};
  virtual void OnFocus() override;

private:
  I_Instrument *getInstrument();
  void buildFields();

  // The equalizer and compressor settings are plain bytes in the instrument,
  // the fields need Variables to edit. These live in the view (not per
  // instrument) and are loaded on entering the screen and written back after
  // every key press. Same order as I_Instrument::EffectParameterTokens.
  Variable *effectVariable(Token token);
  void loadEffectVariables(I_Instrument *instr);
  void storeEffectVariables(I_Instrument *instr);

  etl::vector<Variable, 8> effectVars_;

  etl::vector<UIIntVarField, 16> intVarField_;
};

#endif
