/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

void InstrumentView::fillStackParameters() {
  int i = viewData_->currentInstrumentID_;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  I_Instrument *instr = bank->GetInstrument(i);
  StackInstrument *instrument = (StackInstrument *)instr;
  GUIPoint position = GetAnchor();

  // extra y spacing to allow for gap between export/import and parameters
  position.y_ += 2;

  // Wave
  Variable *v = instrument->FindVariable(Token::StackInstrumentWave);
  intVarField_.emplace_back(position, *v, "Waveform      :%s", 0, stackNumWaveforms - 1, 1, 1);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));  
  addIndexToLine(0, position.y_);
  position.y_++;

  // Transpose
  v = instrument->FindVariable(Token::StackInstrumentTranspose);
  intVarField_.emplace_back(position, *v, "Transpose    :%+03d", -24, 24, 1, 12);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));  
  addIndexToLine(1, position.y_);
  position.y_++;

  // Volume
  v = instrument->FindVariable(Token::StackInstrumentVolume);
  intVarField_.emplace_back(position, *v, "Volume        :%02X", 0x00, 0xff, 1, 16);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));  
  addIndexToLine(2, position.y_);
  position.y_ += 2;

  // Attack
  v = instrument->FindVariable(Token::StackInstrumentAttack);
  intVarField_.emplace_back(position, *v, "Attack        :%02X", 0x00, 0xff, 1, 0x10);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));  
  addIndexToLine(3, position.y_);
  position.y_++;

  // Decay
  v = instrument->FindVariable(Token::StackInstrumentDecay);
  intVarField_.emplace_back(position, *v, "Decay         :%02X", 0x00, 0xff, 1, 0x10);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));  
  addIndexToLine(4, position.y_);
  position.y_++;

  // Sustain
  v = instrument->FindVariable(Token::StackInstrumentSustain);
  intVarField_.emplace_back(position, *v, "Sustain       :%02X", 0x00, 0xff, 1, 0x10);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));  
  addIndexToLine(5, position.y_);
  position.y_++;

  // Release
  v = instrument->FindVariable(Token::StackInstrumentRelease);
  intVarField_.emplace_back(position, *v, "Release       :%02X", 0x00, 0xff, 1, 0x10);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));  
  addIndexToLine(6, position.y_);
  position.y_ += 2;

  // Spread
  v = instrument->FindVariable(Token::StackInstrumentSpread);
  intVarField_.emplace_back(position, *v, "Spread        :%02X", 0x0000, 0xff, 1, 16);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));  
  addIndexToLine(7, position.y_);
  position.y_++;

  // Brightness
  v = instrument->FindVariable(Token::StackInstrumentBrightness);
  intVarField_.emplace_back(position, *v, "Brightness    : %01X", 0, stackBrightnessMax, 1, stackBrightnessMax / 2);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));  
  addIndexToLine(8, position.y_);
  position.y_++;

  // Glide
  v = instrument->FindVariable(Token::StackInstrumentGlide);
  intVarField_.emplace_back(position, *v, "Glide         :%02X", 0x00, 0xff, 1, 16);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));  
  addIndexToLine(9, position.y_);
  position.y_ += 2;

  // Table
  v = instrument->FindVariable(Token::StackInstrumentTable);
  intVarOffField_.emplace_back(position, *v, "Table         :%2.2X", 0x00, TABLE_COUNT - 1, 1, 16);
  fieldList_.insert(fieldList_.end(), &(*intVarOffField_.rbegin()));  
  position.y_++;

  // Automate
  v = instrument->FindVariable(Token::StackInstrumentTableAutomation);
  intVarField_.emplace_back(position, *v, last_sub_item "Automation:%s", 0, 1, 1, 1);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));  
  position.y_++;
}

void InstrumentView::DrawViewStack() {
  int i = viewData_->currentInstrumentID_;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  I_Instrument *instr = bank->GetInstrument(i);
  StackInstrument *instrument = (StackInstrument *)instr;

  GUIPoint p = GetAnchor();

  // indicators
  SetBackgroundColor(Theme::View::bg);
  SetColor(Theme::View::fg);
  char buffer[16];

  // transpose
  Variable *var = instrument->FindVariable(Token::StackInstrumentTranspose);
  int transpose = var->GetInt();
  horizontal_ruler_6(buffer, map_48_to_bargraph(transpose + 24));
  if (transpose != 0) {
    SetColor((transpose < 0) ? Theme::Data::negative : Theme::Data::positive);
  }
  DrawString(p.x_ + 18, p.y_ + 3, buffer);
  SetColor(Theme::View::fg);

  // volume
  var = instrument->FindVariable(Token::StackInstrumentVolume);
  horizontal_bar_graph_6(buffer, map_255_to_bargraph(var->GetInt()));
  DrawString(p.x_ + 18, p.y_ + 4, buffer);
 
  // attack
  var = instrument->FindVariable(Token::StackInstrumentAttack);
  horizontal_bar_graph_6(buffer, map_255_to_bargraph(var->GetInt()));
  DrawString(p.x_ + 18, p.y_ + 6, buffer);
 
  // decay
  var = instrument->FindVariable(Token::StackInstrumentDecay);
  horizontal_bar_graph_6(buffer, map_255_to_bargraph(var->GetInt()));
  DrawString(p.x_ + 18, p.y_ + 7, buffer);

  // sustain
  var = instrument->FindVariable(Token::StackInstrumentSustain);
  horizontal_bar_graph_6(buffer, map_255_to_bargraph(var->GetInt()));
  DrawString(p.x_ + 18, p.y_ + 8, buffer);

  // release
  var = instrument->FindVariable(Token::StackInstrumentRelease);
  horizontal_bar_graph_6(buffer, map_255_to_bargraph(var->GetInt()));
  DrawString(p.x_ + 18, p.y_ + 9, buffer);

  // spread
  var = instrument->FindVariable(Token::StackInstrumentSpread);
  horizontal_bar_graph_6(buffer, map_255_to_bargraph(var->GetInt()));
  DrawString(p.x_ + 18, p.y_ + 11, buffer);

  // brightness
  var = instrument->FindVariable(Token::StackInstrumentBrightness);
  horizontal_ruler_6(buffer, map_12_to_bargraph(var->GetInt()));
  DrawString(p.x_ + 18, p.y_ + 12, buffer);

  // glide
  var = instrument->FindVariable(Token::StackInstrumentGlide);
  horizontal_bar_graph_6(buffer, map_255_to_bargraph(var->GetInt()));
  DrawString(p.x_ + 18, p.y_ + 13, buffer);
}
