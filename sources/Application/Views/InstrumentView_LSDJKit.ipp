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
  Variable *v; 
  
  addTitleLabel("Kits", position.y_);
  position.y_++;
  
  v = instrument->FindVariable(Token::LSDJKitInstrumentKit1);
  intVarField_.emplace_back(UIIntVarField(position, *v, "Kit 1   :%-19.19s", 0, LSDJKits::drum_kit_count - 1, 1, 0x10));
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::sample(true));
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(4, position.y_);

  position.y_ += 1;
  v = instrument->FindVariable(Token::LSDJKitInstrumentKit2);
  intVarField_.emplace_back(UIIntVarOffField(position, *v, "Kit 2   :%-19.19s", 0, LSDJKits::drum_kit_count - 1, 1, 0x10));
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::sample(true));
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(5, position.y_);
 
  position.y_++;
  addTitleLabel("Sample   Kit1            Kit2", position.y_);

  position.y_++;
  v = instrument->FindVariable(Token::LSDJKitInstrumentOffset1);
  intVarField_.emplace_back(UIIntVarField(position, *v, "Offset  : %02X", 0, 255, 1, 16));
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::sample(true));
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(6, position.y_, true);
  
  v = instrument->FindVariable(Token::LSDJKitInstrumentOffset2);
  intVarField_.emplace_back(UIIntVarField(position + GUIPoint(24, 0), *v, ": %02X", 0, 255, 1, 16));
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::sample(true));
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(7, position.y_);

  position.y_++;
  v = instrument->FindVariable(Token::LSDJKitInstrumentLength1);
  intVarOffField_.emplace_back(UIIntVarOffField(position, *v, "Length  : %02X", 0, 255, 1, 16));
  intVarOffField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarOffField_.back().SetLabelColor(Theme::SemanticColors::sample(true));
  fieldList_.insert(fieldList_.end(), &intVarOffField_.back());
  addIndexToLine(7, position.y_, true);

  v = instrument->FindVariable(Token::LSDJKitInstrumentLength2);
  intVarOffField_.emplace_back(UIIntVarOffField(position + GUIPoint(24, 0), *v, ": %02X", 0, 255, 1, 16));
  intVarOffField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarOffField_.back().SetLabelColor(Theme::SemanticColors::sample(true));
  fieldList_.insert(fieldList_.end(), &intVarOffField_.back());
  addIndexToLine(8, position.y_);
  
  position.y_++;
  v = instrument->FindVariable(Token::LSDJKitInstrumentLoop1);
  intVarField_.emplace_back(UIIntVarOffField(position, *v, "Loop    :%-3.3s", 0, loopModeCount - 1, 1, loopModeCount - 1));
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::sample(true));
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(8, position.y_, true);
  
  v = instrument->FindVariable(Token::LSDJKitInstrumentLoop2);
  intVarField_.emplace_back(UIIntVarOffField(position + GUIPoint(24, 0), *v, ":%-3.3s", 0, loopModeCount - 1, 1, loopModeCount - 1));
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::sample(true));
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(9, position.y_);


  position.y_++;
  addTitleLabel("Pitch", position.y_);
  
  position.y_++;
  v = instrument->FindVariable(Token::LSDJKitInstrumentSpeed);
  intVarField_.emplace_back(UIIntVarOffField(position, *v, "Speed   :%-3.3s", 0, speedModeCount - 1, 1, speedModeCount - 1));
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::pitch);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(10, position.y_, true);
  
  position.y_++;
  addTitleLabel("Effects", position.y_);
  
  position.y_++;
  v = instrument->FindVariable(Token::LSDJKitInstrumentBitDepth);
  intVarField_.emplace_back(UIIntVarField(position, *v, "BitDepth:  %1d", 2, 8, 1, 7));
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::effect);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(11, position.y_, true);

  position.y_++;
  v = instrument->FindVariable(Token::LSDJKitInstrumentClip);
  intVarField_.emplace_back(UIIntVarOffField(position, *v, "Clip    :%-3.3s", 0, clippingModeCount, 1, clippingModeCount));
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::effect);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(12, position.y_, true);

  // row Table / Automate

  addTitleLabel("Volume", 20);
  AddTableRow();

  addTitleLabel("Automation", 22);
  AddVolumeRow();

}

void InstrumentView::DrawViewLSDJKit() {
}