/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */


 #include "DrumEnums.h"

 static const char *drumFormatStrings[12] = {
  "BassDrum:%4.4X",
  "Rimshot :%4.4X",
  "Snare   :%4.4X",
  "Clap    :%4.4X",
  "Snare 2 :%4.4X",
  "Tom 1   :%4.4X",
  "HHclosed:%4.4X",
  "Tom 2   :%4.4X",
  "HHalt   :%4.4X",
  "Tom 3   :%4.4X",
  "HHopen  :%4.4X",
  "Crash   :%4.4X",
};

void InstrumentView::fillDrumParameters() {
  int i = viewData_->currentInstrumentID_;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  I_Instrument *instr = bank->GetInstrument(i);
  DrumInstrument *instrument = (DrumInstrument *)instr;
  GUIPoint position = GetAnchor();

  // extra y spacing to allow for gap between export/import and parameters
  position.y_ += 2;

  #define horz_2 char_border_single_horizontal_s char_border_single_horizontal_s 
  #define horz_4 horz_2 horz_2
  #define horz_5 horz_4 char_border_single_horizontal_s
  #define horz_6 horz_4 horz_2
  #define vert char_border_single_vertical_s
  #define vert2 vert vert
  #define vert4 vert2 vert2
  
  staticField_.emplace_back(position, "Waveform" horz_4 char_border_single_topRight_s);
  fieldList_.insert(fieldList_.end(), &(*staticField_.rbegin()));
  position.y_++;

  staticField_.emplace_back(position, "Decay" horz_6 char_border_single_topRight_s vert);
  fieldList_.insert(fieldList_.end(), &(*staticField_.rbegin()));
  position.y_++;

  staticField_.emplace_back(position, "Note" horz_6 char_border_single_topRight_s vert2);
  fieldList_.insert(fieldList_.end(), &(*staticField_.rbegin()));
  position.y_++;

  staticField_.emplace_back(position, "Sweep" horz_4 char_border_single_topRight_s vert2 vert);
  fieldList_.insert(fieldList_.end(), &(*staticField_.rbegin()));
  position.y_++;  

  staticField_.emplace_back(position, "         " vert4);
  fieldList_.insert(fieldList_.end(), &(*staticField_.rbegin()));
  position.y_++;  

  Variable *v = instrument->FindVariable(FourCC::DrumInstrumentParamsVoice0);
  bigHexVarField_.emplace_back(position, *v, 4, drumFormatStrings[0], 0x0000, 0xffff, 16);
  fieldList_.insert(fieldList_.end(), &(*bigHexVarField_.rbegin()));  
  position.y_++;

  v = instrument->FindVariable(FourCC::DrumInstrumentParamsVoice1);
  bigHexVarField_.emplace_back(position, *v, 4, drumFormatStrings[1], 0x0000, 0xffff, 16);
  fieldList_.insert(fieldList_.end(), &(*bigHexVarField_.rbegin()));  
  position.y_++;

  v = instrument->FindVariable(FourCC::DrumInstrumentParamsVoice2);
  bigHexVarField_.emplace_back(position, *v, 4, drumFormatStrings[2], 0x0000, 0xffff, 16);
  fieldList_.insert(fieldList_.end(), &(*bigHexVarField_.rbegin()));  
  position.y_++;

  v = instrument->FindVariable(FourCC::DrumInstrumentParamsVoice3);
  bigHexVarField_.emplace_back(position, *v, 4, drumFormatStrings[3], 0x0000, 0xffff, 16);
  fieldList_.insert(fieldList_.end(), &(*bigHexVarField_.rbegin()));  
  position.y_++;

  v = instrument->FindVariable(FourCC::DrumInstrumentParamsVoice4);
  bigHexVarField_.emplace_back(position, *v, 4, drumFormatStrings[4], 0x0000, 0xffff, 16);
  fieldList_.insert(fieldList_.end(), &(*bigHexVarField_.rbegin()));  
  position.y_++;

  v = instrument->FindVariable(FourCC::DrumInstrumentParamsVoice5);
  bigHexVarField_.emplace_back(position, *v, 4, drumFormatStrings[5], 0x0000, 0xffff, 16);
  fieldList_.insert(fieldList_.end(), &(*bigHexVarField_.rbegin()));  
  position.y_++;

  v = instrument->FindVariable(FourCC::DrumInstrumentParamsVoice6);
  bigHexVarField_.emplace_back(position, *v, 4, drumFormatStrings[6], 0x0000, 0xffff, 16);
  fieldList_.insert(fieldList_.end(), &(*bigHexVarField_.rbegin()));  
  position.y_++;

  v = instrument->FindVariable(FourCC::DrumInstrumentParamsVoice7);
  bigHexVarField_.emplace_back(position, *v, 4, drumFormatStrings[7], 0x0000, 0xffff, 16);
  fieldList_.insert(fieldList_.end(), &(*bigHexVarField_.rbegin()));  
  position.y_++;

  v = instrument->FindVariable(FourCC::DrumInstrumentParamsVoice8);
  bigHexVarField_.emplace_back(position, *v, 4, drumFormatStrings[8], 0x0000, 0xffff, 16);
  fieldList_.insert(fieldList_.end(), &(*bigHexVarField_.rbegin()));  
  position.y_++;

  v = instrument->FindVariable(FourCC::DrumInstrumentParamsVoice9);
  bigHexVarField_.emplace_back(position, *v, 4, drumFormatStrings[9], 0x0000, 0xffff, 16);
  fieldList_.insert(fieldList_.end(), &(*bigHexVarField_.rbegin()));  
  position.y_++;

  v = instrument->FindVariable(FourCC::DrumInstrumentParamsVoice10);
  bigHexVarField_.emplace_back(position, *v, 4, drumFormatStrings[10], 0x0000, 0xffff, 16);
  fieldList_.insert(fieldList_.end(), &(*bigHexVarField_.rbegin()));  
  position.y_++;

  v = instrument->FindVariable(FourCC::DrumInstrumentParamsVoice11);
  bigHexVarField_.emplace_back(position, *v, 4, drumFormatStrings[11], 0x0000, 0xffff, 16);
  fieldList_.insert(fieldList_.end(), &(*bigHexVarField_.rbegin()));  
  position.y_++;
  
  for (auto &f : bigHexVarField_) {
    f.SetWrapDigits(false);
    f.SetWrap(false);
  }
}

void InstrumentView::DrawViewDrum() {
  GUIPoint p = GetAnchor();

  UIField *f = GetFocus();
  UIBigHexVarField *field = (UIBigHexVarField *)(f);

  char buffer[16];

  int currentID = viewData_->currentInstrumentID_;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  I_Instrument *instr = bank->GetInstrument(currentID);
  
  SetColor(Theme::View::fg);
  SetBackgroundColor(Theme::View::bg);

  for (int n = 0; n < 12; n++) {
    Variable *v = instr->FindVariable(FourCC::enum_type(FourCC::DrumInstrumentParamsVoice0 + n));
    uint32_t wave = v->GetInt() % drumNumWaveforms;
    DrawString(p.x_ + 14, p.y_ + 7 + n, chiptune_waveforms[wave]);
  }
}
