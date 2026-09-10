/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

 void InstrumentView::fillMidiParameters() {
  int i = viewData_->currentInstrumentID_;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  I_Instrument *instr = bank->GetInstrument(i);
  MidiInstrument *instrument = (MidiInstrument *)instr;

  GUIPoint position = GUIPoint(1, 6);
   
  Variable *v = instrument->FindVariable(Token::MidiInstrumentChannel);
  intVarField_.emplace_back(UIIntVarField(position, *v, "Channel : %2.2d", 0, 0x0F, 1, 0x04, 1));
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(4, position.y_, true);

  position.y_ += 1;
  v = instrument->FindVariable(Token::MidiInstrumentProgram);
  intVarOffField_.emplace_back(UIIntVarOffField(position, *v, "Program : %2.2X", 0, 0x7F, 1, 0x10));
  intVarOffField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarOffField_.back());
  addIndexToLine(5, position.y_, true);

  position.y_ += 1;
  v = instrument->FindVariable(Token::MidiInstrumentNoteLength);
  intVarField_.emplace_back(UIIntVarField(position, *v, "Length  : %2.2X", 0, 0xFF, 1, 0x10));
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(6, position.y_, true);

  // volume, pan, table, automate

  addTitleLabel("Volume", 20);
  AddVolumeRow();

  addTitleLabel("Automation", 22);
  AddTableRow();
}

void InstrumentView::DrawViewMIDI() {
}