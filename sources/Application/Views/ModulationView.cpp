/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "ModulationView.h"
#include "Application/Instruments/I_Instrument.h"
#include "Application/Model/Project.h"
#include "Application/Player/Player.h"
#include "Foundation/Constants/SpecialCharacters.h"

ModulationView::ModulationView(GUIWindow &w, ViewData *data) : FieldView(w, data) {
}

ModulationView::~ModulationView() {
}

void ModulationView::Reset() {
  fieldList_.clear();
  clearTitleLabels();
  intVarField_.clear();
}

I_Instrument *ModulationView::getInstrument() {
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  return bank->GetInstrument(viewData_->currentInstrumentID_);
}

void ModulationView::buildFields() {
  // the fields point to the variables of the current instrument, so they need
  // to be rebuilt whenever the screen is entered
  ClearFocus();
  fieldList_.clear();
  clearTitleLabels();
  intVarField_.clear();

  I_Instrument *instr = getInstrument();
  if (!instr) {
    return;
  }

  addTitleLabel("Effects send", 2);

  GUIPoint position = GUIPoint(1, 3);
  Variable *v = instr->FindVariable(Token::InstrumentParameterOutput);
  intVarField_.emplace_back(position, *v, "Output  :%-19.19s", 0, OE_LAST - 1, 1, 1);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::effect);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  
  // Delay send effect, time is in ticks. The range is what fits the delay
  // line at the fastest tempo, at slower tempos it gets clamped further
  addTitleLabel("Global Delay", 5);
  
  Project *project = viewData_->project_;

  position.y_ += 3;
  v = project->FindVariable(Token::VarDelayTime);
  constexpr int maxDelayTicks = DelayEffect::MaxTicks(MAX_TEMPO);
  static_assert(maxDelayTicks >= 1 && maxDelayTicks <= 0xFF, "delay time does not fit the 2 digit field");
  intVarField_.emplace_back(position, *v, "Time    : %2.2X", 1, maxDelayTicks, 1, 0x10);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  position.y_ += 1;
  v = project->FindVariable(Token::VarDelayFeedback);
  intVarField_.emplace_back(position, *v, "Feedback: %2.2X", 0, 0xFF, 1, 0x10);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  position.y_ += 1;
  v = project->FindVariable(Token::VarDelayWet);
  intVarField_.emplace_back(position, *v, "Wet mix : %2.2X", 0, 0xFF, 1, 0x10);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  SetFocus(&intVarField_.back());
}

void ModulationView::OnFocus() {
  buildFields();
}

void ModulationView::ProcessButtonMask(uint16_t mask, bool pressed) {
  if (!pressed) {
    FieldView::ProcessButtonMask(mask, pressed);
    return;
  }

  if (mask & BM_NAV) {
    if (mask & BM_LEFT) {
      Navigate(VT_INSTRUMENT, vtCollapse);
    }
    return;
  }

  FieldView::ProcessButtonMask(mask, pressed);

  if (mask & BM_PLAY) {
    Player *player = Player::GetInstance();
    player->OnStartButton(PM_PHRASE, viewData_->songX_, false, viewData_->chainRow_);
  }
}

void ModulationView::DrawView() {
  Clear();

  DrawTitle(char_back_s " Modulation %2.2X", viewData_->currentInstrumentID_);

  FieldView::Redraw();
}
