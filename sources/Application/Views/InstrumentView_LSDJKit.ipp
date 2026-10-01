/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "LSDJKits.generated.h"

 void InstrumentView::fillLSDJKitParameters() {
  int i = viewData_->currentInstrumentID_;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  I_Instrument *instr = bank->GetInstrument(i);
  LSDJKitInstrument *instrument = (LSDJKitInstrument *)instr;

  GUIPoint position = GUIPoint(1, 6);
   
  Variable *v = instrument->FindVariable(Token::LSDJKitInstrumentKit1);
  
  addTitleLabel("Kits", position.y_);
  position.y_++;

  intVarField_.emplace_back(UIIntVarField(position, *v, "Kit 1   : %-18.18s", 0, lsdjKits::drum_kit_count - 1, 1, 0x10));
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::sample(true));
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(4, position.y_);

  position.y_ += 1;
  v = instrument->FindVariable(Token::LSDJKitInstrumentKit2);
  intVarField_.emplace_back(UIIntVarOffField(position, *v, "Kit 2   : %-18.18s", 0, lsdjKits::drum_kit_count - 1, 1, 0x10));
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::sample(true));
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(5, position.y_);

  position.y_++;
  addTitleLabel("Effects", position.y_);

  position.y_++;
  v = instrument->FindVariable(Token::LSDJKitInstrumentBitDepth);
  intVarField_.emplace_back(UIIntVarField(position, *v, "BitDepth:  %1d", 2, 8, 1, 7));
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::effect);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(6, position.y_);

  // row Table / Automate

  addTitleLabel("Volume", 20);
  AddTableRow();

  addTitleLabel("Automation", 22);
  AddVolumeRow();

}

void InstrumentView::DrawViewLSDJKit() {
}