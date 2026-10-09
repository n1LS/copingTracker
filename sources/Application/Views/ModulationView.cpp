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
  intVarField_.back().SetLabelColor(Theme::SemanticColors::volume);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  // 3 band equalizer of the instrument, gains are 0..2x with 0x80 being flat
  addTitleLabel("Equalizer", 5);

  position.y_ += 3;
  v = instr->FindVariable(Token::InstrumentParameterEqOn);
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
    v = instr->FindVariable(band.token);
    intVarField_.emplace_back(position, *v, band.format, 0, 0xFF, 1, 0x10);
    intVarField_.back().SetLabelColor(Theme::SemanticColors::filter);
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

  // Equalizer on the final mix, same gain range as the instrument equalizer
  position.y_ += 4;
  addTitleLabel("Master Equalizer", position.y_);

  position.y_ += 1;
  v = project->FindVariable(Token::VarMasterEqOn);
  intVarField_.emplace_back(position, *v, "Enabled : %1s ", 0, 1, 1, 1);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::filter);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  eq_band_t masterEqBands[] = {
      {Token::VarMasterEqLow, "Low     : %2.2X"},
      {Token::VarMasterEqMid, "Mid     : %2.2X"},
      {Token::VarMasterEqHigh, "High    : %2.2X"},
  };

  for (const auto &band : masterEqBands) {
    position.y_ += 1;
    v = project->FindVariable(band.token);
    intVarField_.emplace_back(position, *v, band.format, 0, 0xFF, 1, 0x10);
    intVarField_.back().SetLabelColor(Theme::SemanticColors::filter);
    intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
    fieldList_.insert(fieldList_.end(), &intVarField_.back());
  }

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

  GUIPoint p = GetAnchor();
  p.y_ += 2;

#define _color(x) (focused == x) ? Theme::View::fg : Theme::View::inactive

  Token focused = GetFocus()->GetVariable()->GetID();
  char buffer[16];
  Variable *var;
  I_Instrument *instrument = getInstrument();

  // draw the equalizer visualization for good measure
  var = instrument->FindVariable(Token::InstrumentParameterEqLow);
  SetColor(_color(Token::InstrumentParameterEqLow));
  horizontal_bar_graph_6(buffer, map_255_to_bargraph6(var->GetInt()));
  DrawString(p.x_ + 10, p.y_ + 2, buffer, fGraphic);

  var = instrument->FindVariable(Token::InstrumentParameterEqMid);
  SetColor(_color(Token::InstrumentParameterEqMid));
  horizontal_bar_graph_6(buffer, map_255_to_bargraph6(var->GetInt()));
  DrawString(p.x_ + 10, p.y_ + 3, buffer, fGraphic);

  var = instrument->FindVariable(Token::InstrumentParameterEqHigh);
  SetColor(_color(Token::InstrumentParameterEqHigh));
  horizontal_bar_graph_6(buffer, map_255_to_bargraph6(var->GetInt()));
  DrawString(p.x_ + 10, p.y_ + 4, buffer, fGraphic);
  
  // global eq
  Project *project = viewData_->project_;

  SetColor(Theme::View::inactive);
  DrawString(0, p.y_ + 11, char_line_11_s char_line_11_s char_line_10_s);

  var = project->FindVariable(Token::VarMasterEqLow);
  if (var) {
    SetColor(_color(Token::VarMasterEqLow));
    horizontal_bar_graph_6(buffer, map_255_to_bargraph6(var->GetInt()));
    DrawString(p.x_ + 10, p.y_ + 15, buffer, fGraphic);
  }

  var = project->FindVariable(Token::VarMasterEqMid);
  SetColor(_color(Token::VarMasterEqMid));
  horizontal_bar_graph_6(buffer, map_255_to_bargraph6(var->GetInt()));
  DrawString(p.x_ + 10, p.y_ + 16, buffer, fGraphic);

  var = project->FindVariable(Token::VarMasterEqHigh);
  SetColor(_color(Token::VarMasterEqHigh));
  horizontal_bar_graph_6(buffer, map_255_to_bargraph6(var->GetInt()));
  DrawString(p.x_ + 10, p.y_ + 17, buffer, fGraphic);
}
