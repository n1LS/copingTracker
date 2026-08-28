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

  Variable *v = instrument->FindVariable(Token::SampleInstrumentGMInstrument);
  int ins = v->GetInt();
  int offset = (ins > 99) ? 19 : (ins > 10) ? 18 : 17;

  DrawString(position.x_ + offset, position.y_, (ins >= 0) ? GMBank::nameForPreset(ins) : " ---");
  DrawString(position.x_ - 1, position.y_ - 1, char_border_single_topLeft_s);
  DrawString(position.x_ - 1, position.y_, char_border_single_bottomLeft_s);
}
