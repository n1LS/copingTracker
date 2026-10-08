/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _MODULATION_VIEW_H_
#define _MODULATION_VIEW_H_

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

  virtual void ProcessButtonMask(uint16_t mask, bool pressed) override;
  virtual void DrawView() override;
  virtual void OnPlayerUpdate(PlayerEventType, unsigned int) override {};
  virtual void OnFocus() override;

private:
  I_Instrument *getInstrument();
  void buildFields();

  etl::vector<UIIntVarField, 1> intVarField_;
};

#endif
