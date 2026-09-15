/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */


#include "DrumEnums.h"
#include "Application/Utils/char.h" 

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
  
  GUIPoint position = GUIPoint(4, 6);

  const uint8_t drumOrder[12] = {0, 2, 4, 1, 3, 5, 7, 9, 6, 8, 10, 11};

  for (int n = 0; n < 12; n++) {
    Variable *v = instrument->FindVariable(Token(Token::DrumInstrumentParamsVoice0 + n));
    hexVarField_.emplace_back(position, *v, 4, drumFormatStrings[drumOrder[n]], 0x0000, 0xffff, 16);
    hexVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
    fieldList_.insert(fieldList_.end(), &hexVarField_.back());
    position.y_++;
  }

  for (auto &f : hexVarField_) {
    f.SetWrapDigits(false);
    f.SetWrap(false);
  }

  // character
  addTitleLabel("Effects", position.y_);  

  position.y_++;
  position.x_ = 1;
  Variable *v = instrument->FindVariable(Token::DrumInstrumentParamsCharacter);
  intVarField_.emplace_back(position, *v, "Wobble  : %02X", 0, 255, 1, 16);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::effect);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(4, position.y_, true);

  // volume row
  addTitleLabel("Volume", SCREEN_HEIGHT - 4);
  AddVolumeRow(SCREEN_HEIGHT - 3);

  addTitleLabel("Automation", SCREEN_HEIGHT - 2);
  AddTableRow(SCREEN_HEIGHT - 1);
}

void InstrumentView::DrawViewDrum() {
  GUIPoint p = GetAnchor();

  SetColor(Theme::View::fg);
  SetBackgroundColor(Theme::View::bg);

  // waveform display
  const int displayOrder[12] = { 0, 2, 4, 1, 3, 5, 7, 9, 6, 8, 10, 11 };
  Color colors[4] = {Theme::SemanticColors::pitch, Theme::SemanticColors::effect, Theme::SemanticColors::volume, Theme::SemanticColors::sample(true)}; 

  for (int n = 0; n < 12; n++) {
    UIHexVarField &field = hexVarField_[n];
    Variable *var = field.GetVariable();
    int column = field.GetColumn();
    bool selected = field.HasFocus();
    drum_parameters_t params = std::bit_cast<drum_parameters_t>(var->GetInt());

    SetColor(selected ? (column == 3 ? colors[0] : Theme::View::fg) : Theme::View::inactive);
    DrawChar(p.x_ + 14, p.y_ + 3 + n, char_v_bar_lookup[(params.pitch * 10 + 7) / 15]);
    SetColor(selected ? (column == 2 ? colors[1] : Theme::View::fg) : Theme::View::inactive);
    DrawChar(p.x_ + 15, p.y_ + 3 + n, char_v_bar_lookup[(params.note * 10 + 7) / 15]);
    SetColor(selected ? (column == 1 ? colors[2] : Theme::View::fg) : Theme::View::inactive);
    DrawChar(p.x_ + 16, p.y_ + 3 + n, char_v_bar_lookup[(params.decay * 10 + 7) / 15]);
    SetColor(selected ? (column == 0 ? colors[3] : Theme::View::fg) : Theme::View::inactive);
    DrawString(p.x_ + 18, p.y_ + 3 + n, chiptune_waveforms[params.wave % drumNumWaveforms]);
  }

  // legend labels below
  for (int n = 0; n < 12; n++) {
    UIHexVarField &field = hexVarField_[n];
    
    if (field.HasFocus()) {
      int column = 3 - field.GetColumn();
      
#define horz char_border_single_horizontal_s
#define horz2 horz horz
#define horz4 horz2 horz2
      
      const char *labels[4] = {
        char_border_single_bottomLeft_s horz4 "Sweep", 
        char_border_single_bottomLeft_s horz2 horz "Note", 
        char_border_single_bottomLeft_s horz2 "Decay", 
        char_border_single_bottomLeft_s horz "Waveform"
      };
    
      SetColor(colors[column]);
      DrawString(13 + column, 18, labels[column]);
      break;
    }
  }

  // note labels
  for (int j = 0; j < 12; j++) {
    SetColor(Theme::View::index(j % ALT_ROW_NUMBER == 0));
    DrawString(1, 6 + j, noteNames[displayOrder[j]]);
  }
}
