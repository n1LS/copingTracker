/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */


 #include "DrumEnums.h"

 static const char *drumFormatStrings[12] = {
  Drum_Name_0 ":%4.4X",
  Drum_Name_1 ":%4.4X",
  Drum_Name_2 ":%4.4X",
  Drum_Name_3 ":%4.4X",
  Drum_Name_4 ":%4.4X",
  Drum_Name_5 ":%4.4X",
  Drum_Name_6 ":%4.4X",
  Drum_Name_7 ":%4.4X",
  Drum_Name_8 ":%4.4X",
  Drum_Name_9 ":%4.4X",
  Drum_Name_A ":%4.4X",
  Drum_Name_B ":%4.4X",
};

void InstrumentView::fillDrumParameters() {
  int i = viewData_->currentInstrumentID_;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  I_Instrument *instr = bank->GetInstrument(i);
  DrumInstrument *instrument = (DrumInstrument *)instr;
  GUIPoint position = GetAnchor();

  // extra y spacing to allow for gap between export/import and parameters
  position.y_ += 5;

  #define horz_2 char_border_single_horizontal_s char_border_single_horizontal_s 
  #define horz_3 horz_2 char_border_single_horizontal_s
  #define horz_4 horz_2 horz_2
  #define horz_5 horz_4 char_border_single_horizontal_s
  #define horz_6 horz_4 horz_2
  #define vert char_border_single_vertical_s
  #define vert2 vert vert
  #define vert4 vert2 vert2
  
  // bass drum
  Variable *v = instrument->FindVariable(Token::DrumInstrumentParamsVoice0);
  hexVarField_.emplace_back(position, *v, 4, drumFormatStrings[0], 0x0000, 0xffff, 16);
  fieldList_.insert(fieldList_.end(), &(*hexVarField_.rbegin()));  
  position.y_++;

  // snare 1
  v = instrument->FindVariable(Token::DrumInstrumentParamsVoice2);
  hexVarField_.emplace_back(position, *v, 4, drumFormatStrings[2], 0x0000, 0xffff, 16);
  fieldList_.insert(fieldList_.end(), &(*hexVarField_.rbegin()));  
  position.y_++;

  // snare 2
  v = instrument->FindVariable(Token::DrumInstrumentParamsVoice4);
  hexVarField_.emplace_back(position, *v, 4, drumFormatStrings[4], 0x0000, 0xffff, 16);
  fieldList_.insert(fieldList_.end(), &(*hexVarField_.rbegin()));  
  position.y_++;

  // rim
  v = instrument->FindVariable(Token::DrumInstrumentParamsVoice1);
  hexVarField_.emplace_back(position, *v, 4, drumFormatStrings[1], 0x0000, 0xffff, 16);
  fieldList_.insert(fieldList_.end(), &(*hexVarField_.rbegin()));  
  position.y_++;

  // clap
  v = instrument->FindVariable(Token::DrumInstrumentParamsVoice3);
  hexVarField_.emplace_back(position, *v, 4, drumFormatStrings[3], 0x0000, 0xffff, 16);
  fieldList_.insert(fieldList_.end(), &(*hexVarField_.rbegin()));  
  position.y_++;

  // hh closed
  v = instrument->FindVariable(Token::DrumInstrumentParamsVoice5);
  hexVarField_.emplace_back(position, *v, 4, drumFormatStrings[5], 0x0000, 0xffff, 16);
  fieldList_.insert(fieldList_.end(), &(*hexVarField_.rbegin()));  
  position.y_++;

  // hihat pedal
  v = instrument->FindVariable(Token::DrumInstrumentParamsVoice7);
  hexVarField_.emplace_back(position, *v, 4, drumFormatStrings[7], 0x0000, 0xffff, 16);
  fieldList_.insert(fieldList_.end(), &(*hexVarField_.rbegin()));  
  position.y_++;

  // hihat open
  v = instrument->FindVariable(Token::DrumInstrumentParamsVoice9);
  hexVarField_.emplace_back(position, *v, 4, drumFormatStrings[9], 0x0000, 0xffff, 16);
  fieldList_.insert(fieldList_.end(), &(*hexVarField_.rbegin()));  
  position.y_++;

  // low tom
  v = instrument->FindVariable(Token::DrumInstrumentParamsVoice6);
  hexVarField_.emplace_back(position, *v, 4, drumFormatStrings[6], 0x0000, 0xffff, 16);
  fieldList_.insert(fieldList_.end(), &(*hexVarField_.rbegin()));  
  position.y_++;

  // mid tom
  v = instrument->FindVariable(Token::DrumInstrumentParamsVoice8);
  hexVarField_.emplace_back(position, *v, 4, drumFormatStrings[8], 0x0000, 0xffff, 16);
  fieldList_.insert(fieldList_.end(), &(*hexVarField_.rbegin()));  
  position.y_++;

  // high tom
  v = instrument->FindVariable(Token::DrumInstrumentParamsVoice10);
  hexVarField_.emplace_back(position, *v, 4, drumFormatStrings[10], 0x0000, 0xffff, 16);
  fieldList_.insert(fieldList_.end(), &(*hexVarField_.rbegin()));  
  position.y_++;

  // crash
  v = instrument->FindVariable(Token::DrumInstrumentParamsVoice11);
  hexVarField_.emplace_back(position, *v, 4, drumFormatStrings[11], 0x0000, 0xffff, 16);
  fieldList_.insert(fieldList_.end(), &(*hexVarField_.rbegin()));  
  position.y_++;

  // character
  position.y_++;
  v = instrument->FindVariable(Token::DrumInstrumentParamsCharacter);
  intVarField_.emplace_back(position, *v, "Flavor  :%02X", 0, 255, 1, 16);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));  
  position.y_++;
  
  for (auto &f : hexVarField_) {
    f.SetWrapDigits(false);
    f.SetWrap(false);
  }
}

void InstrumentView::DrawViewDrum() {
  GUIPoint p = GetAnchor();

  int currentID = viewData_->currentInstrumentID_;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  I_Instrument *instr = bank->GetInstrument(currentID);
  
  SetColor(Theme::View::fg);
  SetBackgroundColor(Theme::View::bg);

  // waveform display
  const int displayOrder[12] = { 0, 2, 4, 1, 3, 5, 7, 9, 6, 8, 10, 11 };

  for (int n = 0; n < 12; n++) {
    Variable *v = instr->FindVariable(Token::enum_type(Token::DrumInstrumentParamsVoice0 + displayOrder[n]));
    uint32_t wave = v->GetInt() % drumNumWaveforms;
    DrawString(p.x_ + 14, p.y_ + 5 + n, chiptune_waveforms[wave]);
  }

  // character display
  char buffer[14];
  horizontal_bar_graph_6(buffer, instr->FindVariable(Token::DrumInstrumentParamsCharacter)->GetInt());
  SetBackgroundColor(Theme::View::inactive);
  SetColor(Theme::View::fg);
  DrawString(p.x_ + 14, p.y_ + 18, buffer);
  SetBackgroundColor(Theme::View::bg);

  // legend labels up top
  SetColor(Theme::View::inactive);

  DrawString(p.x_ + 2, p.y_ + 2, " Note" horz_3 char_border_single_topRight_s char_border_single_topLeft_s horz_3 "Decay");
  DrawString(p.x_ + 2, p.y_ + 3, "Sweep" horz_2 char_border_single_topRight_s vert2 char_border_single_topLeft_s horz_2 "Waveform");
  DrawString(p.x_ + 9, p.y_ + 4, vert4);
}
