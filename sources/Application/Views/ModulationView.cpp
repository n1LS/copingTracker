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
#include "Foundation/Constants/GraphicCharacters.h"
#include "Foundation/Constants/SpecialCharacters.h"

typedef struct eq_band_t {
  Token token;
  const char *format;
} eq_band_t;

ModulationView::ModulationView(GUIWindow &w, ViewData *data) : FieldView(w, data) {
  // the defaults are what a field's clear button resets to
  eq_params_t eq;
  comp_params_t comp;
  effectVars_.emplace_back(Token::InstrumentParameterEqOn, (bool)eq.on);
  effectVars_.emplace_back(Token::InstrumentParameterEqLow, (int)eq.low);
  effectVars_.emplace_back(Token::InstrumentParameterEqMid, (int)eq.mid);
  effectVars_.emplace_back(Token::InstrumentParameterEqHigh, (int)eq.high);
  effectVars_.emplace_back(Token::InstrumentParameterCompOn, (bool)comp.on);
  effectVars_.emplace_back(Token::InstrumentParameterCompThreshold, (int)comp.threshold);
  effectVars_.emplace_back(Token::InstrumentParameterCompRatio, (int)comp.ratio);
  effectVars_.emplace_back(Token::InstrumentParameterCompRelease, (int)comp.release);
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

Variable *ModulationView::effectVariable(Token token) {
  for (Variable &v : effectVars_) {
    if (v.GetID() == token) {
      return &v;
    }
  }
  return nullptr;
}

void ModulationView::loadEffectVariables(I_Instrument *instr) {
  for (Variable &v : effectVars_) {
    v.SetInt(*instr->EffectParameter(v.GetID()), false);
  }
}

void ModulationView::storeEffectVariables(I_Instrument *instr) {
  for (Variable &v : effectVars_) {
    *instr->EffectParameter(v.GetID()) = (uint8_t)v.GetInt();
  }
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
  loadEffectVariables(instr);

  addTitleLabel("Effects send", 2);

  GUIPoint position = GUIPoint(1, 3);
  Variable *v = instr->FindVariable(Token::InstrumentParameterOutput);
  intVarField_.emplace_back(position, *v, "Output  :%-19.19s", 0, OE_LAST - 1, 1, 1);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::volume);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  // 3 band equalizer of the instrument, gains are 0..2x with 0x80 being flat
  addTitleLabel("Equalizer", 5);

  position.y_ += 3;
  v = effectVariable(Token::InstrumentParameterEqOn);
  intVarField_.emplace_back(position, *v, "Enabled : %1s ", 0, 1, 1, 1);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::filter);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  eq_band_t eqBands[] = {
      {Token::InstrumentParameterEqLow, "Low     : %2.2X"},
      {Token::InstrumentParameterEqMid, "Mid     : %2.2X"},
      {Token::InstrumentParameterEqHigh, "High    : %2.2X"},
  };

  for (const auto &band : eqBands) {
    position.y_ += 1;
    v = effectVariable(band.token);
    intVarField_.emplace_back(position, *v, band.format, 0, 0xFF, 1, 0x10);
    intVarField_.back().SetLabelColor(Theme::SemanticColors::filter);
    intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
    fieldList_.insert(fieldList_.end(), &intVarField_.back());
  }

  // compressor of the instrument, runs after the equalizer
  position.y_ += 2;
  addTitleLabel("Compressor", position.y_);

  position.y_ += 1;
  v = effectVariable(Token::InstrumentParameterCompOn);
  intVarField_.emplace_back(position, *v, "Enabled : %1s ", 0, 1, 1, 1);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::volume);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  eq_band_t compParams[] = {
      {Token::InstrumentParameterCompThreshold, "Thresh  : %2.2X"},
      {Token::InstrumentParameterCompRatio, "Ratio   : %2.2X"},
      {Token::InstrumentParameterCompRelease, "Release : %2.2X"},
  };

  for (const auto &param : compParams) {
    position.y_ += 1;
    v = effectVariable(param.token);
    intVarField_.emplace_back(position, *v, param.format, 0, 0xFF, 1, 0x10);
    intVarField_.back().SetLabelColor(Theme::SemanticColors::volume);
    intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
    fieldList_.insert(fieldList_.end(), &intVarField_.back());
  }

  // Delay send effect, time is in ticks. The range is what fits the delay
  // line at the fastest tempo, at slower tempos it gets clamped further
  Project *project = viewData_->project_;

  position.y_ += 2;
  addTitleLabel("Global Delay", position.y_);

  position.y_ += 1;
  v = project->FindVariable(Token::VarDelayTime);
  constexpr int maxDelayTicks = DelayEffect::MaxTicks(MAX_TEMPO);
  static_assert(maxDelayTicks >= 1 && maxDelayTicks <= 0xFF, "delay time does not fit the 2 digit field");
  intVarField_.emplace_back(position, *v, "Time    : %2.2X", 1, maxDelayTicks, 1, 0x10);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::pitch);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  position.y_ += 1;
  v = project->FindVariable(Token::VarDelayFeedback);
  intVarField_.emplace_back(position, *v, "Feedback: %2.2X", 0, 0xFF, 1, 0x10);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::pitch);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  position.y_ += 1;
  v = project->FindVariable(Token::VarDelayWet);
  intVarField_.emplace_back(position, *v, "Wet mix : %2.2X", 0, 0xFF, 1, 0x10);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::pitch);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  SetFocus(&intVarField_.back());
}

void ModulationView::OnFocus() {
  buildFields();
}

bool ModulationView::navigationDown(uint16_t direction) {
  if (direction == BM_LEFT) {
    Navigate(VT_INSTRUMENT, vtCollapse);
    return true;
  }
  return false;
}

void ModulationView::ButtonDown(uint16_t newButton, uint16_t mask) {
  if (mask & BM_NAV || newButton == BM_NAV) {
    return;
  }

  FieldView::ButtonDown(newButton, mask);

  if (I_Instrument *instr = getInstrument()) {
    storeEffectVariables(instr);
  }

  if (IsChord(newButton, mask, BM_PLAY)) {
    Player *player = Player::GetInstance();
    player->OnStartButton(PM_PHRASE, viewData_->songX_, false, viewData_->chainRow_);
  }
}

void ModulationView::DrawView() {
  Clear();

  DrawTitle(char_back_s " Modulation %2.2X", viewData_->currentInstrumentID_);

  FieldView::Redraw();

  GUIPoint p = GetAnchor();
  p.y_ += 2;

#define _color(x) (focused == x) ? Theme::View::fg : Theme::View::inactive

  Token focused = GetFocus()->GetVariable()->GetID();
  char buffer[16];
  I_Instrument *instrument = getInstrument();

  // draw the equalizer visualization for good measure
  SetColor(_color(Token::InstrumentParameterEqLow));
  horizontal_bar_graph_6(buffer, map_255_to_bargraph6(*instrument->EffectParameter(Token::InstrumentParameterEqLow)));
  DrawString(p.x_ + 10, p.y_ + 2, buffer, fGraphic);

  SetColor(_color(Token::InstrumentParameterEqMid));
  horizontal_bar_graph_6(buffer, map_255_to_bargraph6(*instrument->EffectParameter(Token::InstrumentParameterEqMid)));
  DrawString(p.x_ + 10, p.y_ + 3, buffer, fGraphic);

  SetColor(_color(Token::InstrumentParameterEqHigh));
  horizontal_bar_graph_6(buffer, map_255_to_bargraph6(*instrument->EffectParameter(Token::InstrumentParameterEqHigh)));
  DrawString(p.x_ + 10, p.y_ + 4, buffer, fGraphic);

  // and the compressor parameters
  SetColor(_color(Token::InstrumentParameterCompThreshold));
  horizontal_bar_graph_6(buffer,
                         map_255_to_bargraph6(*instrument->EffectParameter(Token::InstrumentParameterCompThreshold)));
  DrawString(p.x_ + 10, p.y_ + 8, buffer, fGraphic);

  SetColor(_color(Token::InstrumentParameterCompRatio));
  horizontal_bar_graph_6(buffer,
                         map_255_to_bargraph6(*instrument->EffectParameter(Token::InstrumentParameterCompRatio)));
  DrawString(p.x_ + 10, p.y_ + 9, buffer, fGraphic);

  SetColor(_color(Token::InstrumentParameterCompRelease));
  horizontal_bar_graph_6(buffer,
                         map_255_to_bargraph6(*instrument->EffectParameter(Token::InstrumentParameterCompRelease)));
  DrawString(p.x_ + 10, p.y_ + 10, buffer, fGraphic);
}
