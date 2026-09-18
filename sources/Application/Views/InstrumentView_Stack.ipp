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

  GUIPoint position = GUIPoint(1, 6);

  // Wave
  Variable *v = instrument->FindVariable(Token::StackInstrumentWave);
  intVarField_.emplace_back(position, *v, "Waveform:%-19.19s", 0, stackNumWaveforms - 1, 1, 1);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::sample(true));
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(4, position.y_);
  
  // Transpose
  position.y_++;
  addTitleLabel("Pitch", position.y_);

  position.y_++;
  v = instrument->FindVariable(Token::StackInstrumentTranspose);
  intVarField_.emplace_back(position, *v, "Transpse:%+03d", -24, 24, 1, 12);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::pitch);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());  
  addIndexToLine(5, position.y_, true);

  // Chord
  position.y_++;
  v = instrument->FindVariable(Token::StackInstrumentChord);
  hexVarField_.emplace_back(position, *v, 4,  "Chord  :%04X", 0x0000, 0xffff, 16);
  hexVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  hexVarField_.back().SetLabelColor(Theme::SemanticColors::pitch);
  fieldList_.insert(fieldList_.end(), &hexVarField_.back());  
  addIndexToLine(6, position.y_, true);

  // Glide
  position.y_++;
  v = instrument->FindVariable(Token::StackInstrumentGlide);
  intVarField_.emplace_back(position, *v, "Glide   : %02X", 0x00, 0xff, 1, 16);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::pitch);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());  
  addIndexToLine(7, position.y_, true);

  // Spread
  position.y_++;
  addTitleLabel("Effects", position.y_);

  position.y_++;
  v = instrument->FindVariable(Token::StackInstrumentSpread);
  intVarField_.emplace_back(position, *v, "Spread  : %02X", 0x0000, 0xff, 1, 16);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::effect);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());  
  addIndexToLine(8, position.y_, true);

  // Brightness/Timber
  position.y_++;
  v = instrument->FindVariable(Token::StackInstrumentBrightness);
  intVarField_.emplace_back(position, *v, "Timbre  : %02X", 0, stackBrightnessMax, 1, stackBrightnessMax / 2);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::effect);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());  
  addIndexToLine(9, position.y_, true);

  // Attack
  position.y_++;
  addTitleLabel("Volume", position.y_);

  position.y_++;
  v = instrument->FindVariable(Token::StackInstrumentAttack);
  intVarField_.emplace_back(position, *v, "Attack  : %02X", 0x00, 0xff, 1, 0x10);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::volume);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());  
  addIndexToLine(10, position.y_, true);

  // Decay
  position.y_++;
  v = instrument->FindVariable(Token::StackInstrumentDecay);
  intVarField_.emplace_back(position, *v, "Decay   : %02X", 0x00, 0xff, 1, 0x10);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::volume);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());  
  addIndexToLine(11, position.y_, true);

  // Sustain
  position.y_++;
  v = instrument->FindVariable(Token::StackInstrumentSustain);
  intVarField_.emplace_back(position, *v, "Sustain : %02X", 0x00, 0xff, 1, 0x10);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::volume);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());  
  addIndexToLine(12, position.y_, true);
  
  // Release
  position.y_++;
  v = instrument->FindVariable(Token::StackInstrumentRelease);
  intVarField_.emplace_back(position, *v, "Release : %02X", 0x00, 0xff, 1, 0x10);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::volume);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());  
  addIndexToLine(13, position.y_, true);

  // default Items

  addTitleLabel("Automation", SCREEN_HEIGHT - 2);
  AddTableRow();

  addTitleLabel("Volume", SCREEN_HEIGHT - 4);
  AddVolumeRow();
}

void InstrumentView::DrawViewStack() {
    /* TODO nILS: re add those
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
  Variable *va r = instrument->FindVariable(Token::StackInstrumentTranspose);
  int transpose = var->GetInt();
  horizontal_ruler_6(buffer, map_48_to_bargraph(transpose + 24));
  if (transpose != 0) {
    SetColor((transpose < 0) ? Theme::Data::negative : Theme::Data::positive);
  }
  DrawString(p.x_ + 18, p.y_ + 3, buffer);
  SetColor(Theme::View::fg);

  // volume
  var = instrument->FindVariable(Token::InstrumentParameterVolume);
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
*/
}
