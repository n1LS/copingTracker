/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "GMBank.h"

void InstrumentView::DrawViewSample() {
  int i = viewData_->currentInstrumentID_;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  I_Instrument *instr = bank->GetInstrument(i);
  StackInstrument *instrument = (StackInstrument *)instr;
  GUIPoint position = GetAnchor();

  // extra y spacing to allow for gap between export/import and parameters
  position.y_ += 2;

  // Wave
  SetColor(Theme::View::fg);
  SetBackgroundColor(Theme::View::bg);

  Variable *gm = instrument->FindVariable(Token::SampleInstrumentGMInstrument);
  int ins = gm->GetInt();
  int offset = 14;

  char buffer[16];
  const char *name = (ins >= 0) ? GMBank::nameForPreset(ins) : "";

  const size_t displayLength = 13;
  const size_t gapLength = 2;
  const size_t nameLength = strlen(name);

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
  
  DrawString(position.x_ + offset, position.y_, buffer);

  // draw the indicator between loop start and end
  SetColor(Theme::View::fg);
  DrawChar(position.x_ + 18, position.y_ + 15, '>');
}

void InstrumentView::AnimationUpdateSample() {
  DrawViewSample();  
}