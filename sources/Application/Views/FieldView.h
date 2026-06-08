/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2018 Discodirt
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#ifndef _FIELD_VIEW_H_
#define _FIELD_VIEW_H_

#include "BaseClasses/UIField.h"
#include "ScreenView.h"

class FieldView : public ScreenView {
public:
  FieldView(GUIWindow &w, ViewData *viewData);

  virtual void Redraw();
  virtual void ProcessButtonMask(uint16_t mask, bool pressed) override;

  void SetFocus(UIField *);
  UIField *GetFocus();
  void ClearFocus();
  int GetFocusIndex();
  void SetSize(int size);

  etl::list<UIField *, 64> fieldList_; // adjust to maximum fields on one screen
  // ThemeView currently biggest user: uses 64 (12 colors * 5 + font + theme
  // name + buttons)

private:
  UIField *focus_;
};

#endif
