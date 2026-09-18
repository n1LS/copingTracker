/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "GMBank.h"

void InstrumentView::fillSampleParameters() {
  int i = viewData_->currentInstrumentID_;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  I_Instrument *instr = bank->GetInstrument(i);
  SampleInstrument *instrument = (SampleInstrument *)instr;
  lastSampleIndex_ = instrument->GetSampleIndex();

  GUIPoint position = GUIPoint(1, 6);
  
  SamplePool *sp = SamplePool::GetInstance();
  Variable *v;

  // offset y to account for instrument type and export/import fields
  v = instrument->FindVariable(Token::SampleInstrumentGMInstrument);
  intVarOffField_.emplace_back(position, *v, "GM Instr:%03d", 0, kGMInstrumentCount - 1, 1, 0x10);
  intVarOffField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  intVarOffField_.back().SetLabelColor(Theme::SemanticColors::sample(true));
  gmInputField_ = &intVarOffField_.back();
  gmInputField_->SetActive(v->GetInt() != NO_GM_INSTRUMENT);
  fieldList_.insert(fieldList_.end(), &intVarOffField_.back());
  addIndexToLine(4, position.y_);

  position.y_++;
  v = instrument->FindVariable(Token::SampleInstrumentSample);
  intVarField_.emplace_back(position, *v, "Sample  :%-19.19s", 0, sp->GetNameListSize() - 1, 1, 0x10);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::sample(true));
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  sampleInputField_ = &intVarField_.back();
  sampleInputField_->SetActive(v->GetInt() != NO_SAMPLE);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(5, position.y_);

  position.y_++;
  position.x_ = 23;
  GUIPoint actionPos = position;
  sampleActionField_.emplace_back("Adjust", Token::ActionShowSampleSlices, actionPos);
  fieldList_.insert(fieldList_.end(), &sampleActionField_.back());
  sampleActionField_.back().AddObserver(*this);

  // row loop mode / From
  
  position.y_++;
  position.x_ = 1;
  v = instrument->FindVariable(Token::SampleInstrumentLoopMode);
  intVarField_.emplace_back(position, *v, "Loop:%-7.7s", 0, SILM_LAST - 1, 1, 1);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::sample(true));
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(6, position.y_, true);

  position.x_ = 17;
  v = instrument->FindVariable(Token::SampleInstrumentLoopStart);
  hexVarField_.emplace_back(position, *v, 7, "From:%7.7X", 0, instrument->GetSampleSize() - 1, 16);
  hexVarField_.back().SetLabelColor(Theme::SemanticColors::sample(true));
  hexVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &hexVarField_.back());

  // offs / to

  position.y_++;
  position.x_ = 1;
  v = instrument->FindVariable(Token::SampleInstrumentStart);
  hexVarField_.emplace_back(position, *v, 7, "Offs:%7.7X", 0, instrument->GetSampleSize() - 1, 16);
  hexVarField_.back().SetLabelColor(Theme::SemanticColors::sample(true));
  hexVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &hexVarField_.back());

  position.x_ = 17;
  v = instrument->FindVariable(Token::SampleInstrumentEnd);
  hexVarField_.emplace_back(position, *v, 7, "To  :%7.7X", 0, instrument->GetSampleSize() - 1, 16);
  hexVarField_.back().SetLabelColor(Theme::SemanticColors::sample(true));
  hexVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &hexVarField_.back());
  
  // Root Note / Detune
  
  position.y_++;
  position.x_ = 1;
  addTitleLabel("Pitch", position.y_);

  position.y_++;
  v = instrument->FindVariable(Token::SampleInstrumentRootNote);
  noteVarField_.emplace_back(position, *v, "RootNote:%-3.3s", 0, 0x7F, 1, 0x0C);
  noteVarField_.back().SetLabelColor(Theme::SemanticColors::pitch);
  noteVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &noteVarField_.back());
  addIndexToLine(7, position.y_, true);

  position.x_ = 17;
  v = instrument->FindVariable(Token::SampleInstrumentFineTune);
  intVarField_.emplace_back(position, *v, "Detune  : %2.2X", 0, 255, 1, 0x10); // TODO signed int
  intVarField_.back().SetLabelColor(Theme::SemanticColors::pitch);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(8, position.y_);

  // --------------------------------------------------------------------------

  // alternating colums of fx and filter parameters

  position.y_++;
  addTitleLabel("FX              Filter", position.y_);

  position.y_++;
  position.x_ = 1;
  v = instrument->FindVariable(Token::SampleInstrumentCrush);
  intVarField_.emplace_back(position, *v, "Crush   : %2.2X", 1, 0x10, 1, 4);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::effect);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(9, position.y_, true);

  position.x_ = 17;
  v = instrument->FindVariable(Token::SampleInstrumentFilterCutOff);
  intVarField_.emplace_back(position, *v, "Cutoff  : %2.2X", 0, 0xFF, 1, 0x10);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::filter);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(10, position.y_);

  position.y_++;
  position.x_ = 1;
  v = instrument->FindVariable(Token::SampleInstrumentCrushVolume);
  intVarField_.emplace_back(position, *v, "Drive   : %2.2X", 0, 0xFF, 1, 0x10);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::effect);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(11, position.y_, true);

  position.x_ = 17;
  v = instrument->FindVariable(Token::SampleInstrumentFilterResonance);
  intVarField_.emplace_back(position, *v, "Reso    : %2.2X", 0, 0xFF, 1, 0x10);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::filter);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(12, position.y_);

  position.y_++;
  position.x_ = 1;
  v = instrument->FindVariable(Token::SampleInstrumentDownsample);
  intVarField_.emplace_back(position, *v, "Downsmpl: %2d", 0, 8, 1, 4);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::effect);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(13, position.y_, true);
  
  position.x_ = 17;
  v = instrument->FindVariable(Token::SampleInstrumentFilterType);
  intVarField_.emplace_back(position, *v, "Type    : %2.2X", 0, 0xFF, 1, 0x10);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::filter);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(14, position.y_);
  
  position.y_++;
  position.x_ = 1;
  v = instrument->FindVariable(Token::SampleInstrumentInterpolation);
  intVarField_.emplace_back(position, *v, "Interpol:%-3.3s", 0, 1, 1, 1);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::effect);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(15, position.y_, true);
  
  position.x_ = 17;
  v = instrument->FindVariable(Token::SampleInstrumentFilterMode);
  intVarField_.emplace_back(position, *v, "Mode:%7.7s", 0, 2, 1, 1);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::filter);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(16, position.y_);

  // --------------------------------------------------------------------------

  position.y_++;
  addTitleLabel("Volume", position.y_);

  position.y_++;
  position.x_ = 1;
  v = instrument->FindVariable(Token::SampleInstrumentAttack);
  intVarField_.emplace_back(position, *v, "Attack  : %2.2X", 0, 0xFF, 1, 0x10);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::volume);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(17, position.y_, true);

  position.x_ = 17;
  v = instrument->FindVariable(Token::SampleInstrumentSustain);
  intVarField_.emplace_back(position, *v, "Sustain : %2.2X", 0, 0xFF, 1, 0x10);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::volume);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(18, position.y_);

  position.y_++;
  position.x_ = 1;
  v = instrument->FindVariable(Token::SampleInstrumentDecay);
  intVarField_.emplace_back(position, *v, "Decay   : %2.2X", 0, 0xFF, 1, 0x10);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::volume);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(19, position.y_, true);

  position.x_ = 17;
  v = instrument->FindVariable(Token::SampleInstrumentRelease);
  intVarField_.emplace_back(position, *v, "Release : %2.2X", 0, 0xFF, 1, 0x10);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::volume);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
  addIndexToLine(20, position.y_);

  // row Table / Automate

  AddTableRow();

  addTitleLabel("Automation", 22);
  AddVolumeRow();
}

void InstrumentView::DrawViewSample() {
  // slice count

  SetBackgroundColor(Theme::View::bg);
  SetColor(Theme::SemanticColors::sample(true));
  DrawString(1, 8, "Slices");

  // draw borders first
  SetColor(Theme::InstrumentInput::bg(false));
  DrawChar(9, 8, CHAR(char_block_left_s));
  DrawChar(13, 8, CHAR(char_block_right_s));

  // content 
  SetBackgroundColor(Theme::InstrumentInput::bg(false));
  SetColor(Theme::InstrumentInput::placeholder);
  
  if (sliceCount_ <= 1) {
    DrawString(10, 8, "off");
  } else {
    char label[3];
    npf_snprintf(label, sizeof(label), " %2d", sliceCount_);
    DrawString(10, 8, label);
  }

  DrawViewSample_GMInstrument();
}

void InstrumentView::DrawViewSample_GMInstrument() {
  int i = viewData_->currentInstrumentID_;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  I_Instrument *instr = bank->GetInstrument(i);
  SampleInstrument *instrument = (SampleInstrument *)instr;

  // Wave

  SetColor(Theme::View::fg);
  SetBackgroundColor(Theme::View::bg);

  Variable *gm = instrument->FindVariable(Token::SampleInstrumentGMInstrument);

  int ins = gm->GetInt();
  if (ins != NO_GM_INSTRUMENT) {
    const char *name = (ins >= 0) ? GMBank::nameForPreset(ins) : "";

    const size_t displayLength = 15;
    const size_t gapLength = 2;
    const size_t nameLength = strlen(name);

    char buffer[displayLength + 1];

    if (nameLength <= displayLength) {
      // Static, space-padded text.
      snprintf(buffer, sizeof(buffer), "%s", name);
    } else {
      // Advance one character every 256 ms ~= 4 characters/second.
      const size_t cycleLength = nameLength + gapLength;
      const size_t start = ((System::GetInstance()->Millis() - scrollStartTime_) >> 8) % cycleLength;

      for (size_t i = 0; i < displayLength; ++i) {
        const size_t source = (start + i) % cycleLength;
        buffer[i] = source < nameLength ? name[source] : ' ';
      }
      buffer[displayLength] = '\0';
    }
    DrawString(14, 6, buffer);
  }
}

void InstrumentView::AnimationUpdateSample() {
  DrawViewSample_GMInstrument();
}
