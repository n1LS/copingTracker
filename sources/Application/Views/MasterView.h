/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _MASTER_VIEW_H_
#define _MASTER_VIEW_H_

#include "BaseClasses/UIIntVarField.h"
#include "FieldView.h"
#include "ViewData.h"

// Second layer screen under the project screen, holds the effects that only
// exist on the master out: compressor and equalizer
class MasterView : public FieldView {
public:
  MasterView(GUIWindow &w, ViewData *data);
  virtual ~MasterView();
  void Reset();

  virtual void ButtonDown(uint16_t newButton, uint16_t mask) override;
  virtual void DrawView() override;
  virtual void OnPlayerUpdate(PlayerEventType, unsigned int) override {};
  virtual void OnFocus() override;

private:
  void buildFields();

  etl::vector<UIIntVarField, 8> intVarField_;
};

#endif
