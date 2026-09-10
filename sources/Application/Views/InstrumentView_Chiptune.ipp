/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#define inner(x) x.format, x.min, x.max, x.step, x.bigStep
#define expand(x) inner(chiptune_instrument_ui_t.x)

void InstrumentView::addIndexToLine(uint8_t index, uint8_t line, bool left) {
  static const char *hexIndexLabels[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O"};

  staticField_.emplace_back(GUIPoint(left ? 14 : 30, line), hexIndexLabels[index]);
  staticField_.back().color_ = Theme::View::info;
  fieldList_.insert(fieldList_.end(), &staticField_.back());
}

void InstrumentView::addTitleLabel(const char *title, uint8_t line, bool left) {
  staticField_.emplace_back(GUIPoint(left ? 1 : 17, line), title);
  staticField_.back().color_ = Theme::View::inactive;
  fieldList_.insert(fieldList_.end(), &staticField_.back());
}

void InstrumentView::fillChiptuneParameters() {
  int i = viewData_->currentInstrumentID_;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  I_Instrument *instr = bank->GetInstrument(i);
  ChiptuneInstrument *instrument = (ChiptuneInstrument *)instr;
  
  GUIPoint position = GUIPoint(1, 6);

  // extra y spacing to allow for gap between export/import and parameters
  Variable *v = instrument->FindVariable(Token::ChiptuneInstrumentWaveform);
  intVarField_.emplace_back(position, *v, expand(wave));
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::sample(true));
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(4, position.y_);

  // --------------------------------------------------------------------------

  position.y_++;
  addTitleLabel("Pitch", position.y_);

  position.y_++;
  v = instrument->FindVariable(Token::ChiptuneInstrumentTranspose);
  intVarField_.emplace_back(position, *v, expand(transpose));
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::pitch);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(5, position.y_, true);

  // --------------------------------------------------------------------------

  position.y_++;
  addTitleLabel("Sweep", position.y_);

  position.y_++;
  v = instrument->FindVariable(Token::ChiptuneInstrumentSweepTime);
  intVarField_.emplace_back(position, *v, expand(sweep_time));
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::pitch);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(6, position.y_, true);
  
  position.y_++;
  v = instrument->FindVariable(Token::ChiptuneInstrumentSweepAmount);
  intVarField_.emplace_back(position, *v, expand(sweep_amount));
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::pitch);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(7, position.y_, true);
  
  position.y_++;
  addTitleLabel("Vibrato", position.y_);

  position.y_++;
  v = instrument->FindVariable(Token::ChiptuneInstrumentVibratoDelay);
  intVarField_.emplace_back(position, *v, expand(vibrato_delay));
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::pitch);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(8, position.y_, true);

  position.y_++;
  v = instrument->FindVariable(Token::ChiptuneInstrumentVibrato);
  intVarField_.emplace_back(position, *v, expand(vibrato_amount));
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::pitch);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(9, position.y_, true);

  // --------------------------------------------------------------------------

  position.y_++;
  addTitleLabel("Effects", position.y_);

  position.y_++;
  v = instrument->FindVariable(Token::ChiptuneInstrumentBurst);
  intVarOffField_.emplace_back(position, *v, expand(burst));
  intVarOffField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarOffField_.back().SetLabelColor(Theme::SemanticColors::effect);
  fieldList_.insert(fieldList_.end(), &intVarOffField_.back());
  addIndexToLine(10, position.y_, true);

  position.y_++;
  v = instrument->FindVariable(Token::ChiptuneInstrumentArpSpeed);
  intVarField_.emplace_back(position, *v, expand(arp));
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::effect);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(11, position.y_, true);

  position.y_++;
  v = instrument->FindVariable(Token::ChiptuneInstrumentLength);
  intVarOffField_.emplace_back(position, *v, expand(length));
  intVarOffField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarOffField_.back().SetLabelColor(Theme::SemanticColors::effect);
  fieldList_.insert(fieldList_.end(), &intVarOffField_.back());
  addIndexToLine(12, position.y_, true);

  // --------------------------------------------------------------------------

  position.y_++;
  addTitleLabel("Volume", position.y_);

  position.y_++;
  v = instrument->FindVariable(Token::ChiptuneInstrumentAttack);
  intVarField_.emplace_back(position, *v, expand(attack));
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::volume);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(13, position.y_, true);

  position.x_ = 17;
  v = instrument->FindVariable(Token::ChiptuneInstrumentDecay);
  intVarField_.emplace_back(position, *v, expand(decay));
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::volume);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(14, position.y_);

  addTitleLabel("Automation", SCREEN_HEIGHT - 2);

  // Default rows
  AddVolumeRow(SCREEN_HEIGHT - 3);
  AddTableRow(SCREEN_HEIGHT - 1);
}

void InstrumentView::DrawViewChiptune() {
  int currentID = viewData_->currentInstrumentID_;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  I_Instrument *instr = bank->GetInstrument(currentID);
}
