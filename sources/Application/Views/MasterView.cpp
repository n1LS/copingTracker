/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "MasterView.h"
#include "Application/Model/Project.h"
#include "Application/Player/Player.h"
#include "Foundation/Constants/GraphicCharacters.h"
#include "Foundation/Constants/SpecialCharacters.h"

typedef struct master_param_t {
  Token token;
  const char *format;
} master_param_t;

MasterView::MasterView(GUIWindow &w, ViewData *data) : FieldView(w, data) {
}

MasterView::~MasterView() {
}

void MasterView::Reset() {
  fieldList_.clear();
  clearTitleLabels();
  intVarField_.clear();
}

void MasterView::buildFields() {
  ClearFocus();
  fieldList_.clear();
  clearTitleLabels();
  intVarField_.clear();

  Project *project = viewData_->project_;
  GUIPoint position = GUIPoint(1, 3);
  Variable *v;

  // both run on the summed mix before the master volume, compressor first
  addTitleLabel("Master Compressor", 2);

  v = project->FindVariable(Token::VarMasterCompOn);
  intVarField_.emplace_back(position, *v, "Enabled : %1s ", 0, 1, 1, 1);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::volume);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  master_param_t compParams[] = {
      {Token::VarMasterCompThreshold, "Thresh  : %2.2X"},
      {Token::VarMasterCompRatio, "Ratio   : %2.2X"},
      {Token::VarMasterCompRelease, "Release : %2.2X"},
  };
  for (const auto &param : compParams) {
    position.y_ += 1;
    v = project->FindVariable(param.token);
    intVarField_.emplace_back(position, *v, param.format, 0, 0xFF, 1, 0x10);
    intVarField_.back().SetLabelColor(Theme::SemanticColors::volume);
    intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
    fieldList_.insert(fieldList_.end(), &intVarField_.back());
  }

  position.y_ += 2;
  addTitleLabel("Master Equalizer", position.y_);

  position.y_ += 1;
  v = project->FindVariable(Token::VarMasterEqOn);
  intVarField_.emplace_back(position, *v, "Enabled : %1s ", 0, 1, 1, 1);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::filter);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  master_param_t eqBands[] = {
      {Token::VarMasterEqLow, "Low     : %2.2X"},
      {Token::VarMasterEqMid, "Mid     : %2.2X"},
      {Token::VarMasterEqHigh, "High    : %2.2X"},
  };
  for (const auto &band : eqBands) {
    position.y_ += 1;
    v = project->FindVariable(band.token);
    intVarField_.emplace_back(position, *v, band.format, 0, 0xFF, 1, 0x10);
    intVarField_.back().SetLabelColor(Theme::SemanticColors::filter);
    intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
    fieldList_.insert(fieldList_.end(), &intVarField_.back());
  }

  SetFocus(&intVarField_.front());
}

void MasterView::OnFocus() {
  buildFields();
}

void MasterView::ProcessButtonMask(uint16_t mask, bool pressed) {
  if (!pressed) {
    FieldView::ProcessButtonMask(mask, pressed);
    return;
  }

  if (mask & BM_NAV) {
    if (mask & BM_LEFT) {
      Navigate(VT_PROJECT, vtCollapse);
    }
    return;
  }

  FieldView::ProcessButtonMask(mask, pressed);

  if (mask & BM_PLAY) {
    Player *player = Player::GetInstance();
    player->OnStartButton(PM_SONG, viewData_->songX_, false, viewData_->songX_);
  }
}

void MasterView::DrawView() {
  Clear();

  DrawTitle(char_back_s " Master Out");

  FieldView::Redraw();

  if (!GetFocus() || !GetFocus()->GetVariable()) {
    return;
  }

#define _color(x) (focused == x) ? Theme::View::fg : Theme::View::inactive

  Token focused = GetFocus()->GetVariable()->GetID();
  Project *project = viewData_->project_;
  char buffer[16];

  // bars next to the fields, on the same rows
  struct {
    Token token;
    int row;
  } bars[] = {
      {Token::VarMasterCompThreshold, 4}, {Token::VarMasterCompRatio, 5}, {Token::VarMasterCompRelease, 6},
      {Token::VarMasterEqLow, 10},        {Token::VarMasterEqMid, 11},    {Token::VarMasterEqHigh, 12},
  };
  for (const auto &bar : bars) {
    Variable *var = project->FindVariable(bar.token);
    SetColor(_color(bar.token));
    horizontal_bar_graph_6(buffer, map_255_to_bargraph6(var->GetInt()));
    DrawString(15, bar.row, buffer, fGraphic);
  }
}
