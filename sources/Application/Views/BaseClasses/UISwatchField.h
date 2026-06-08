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

#ifndef _UI_SWATCH_FIELD_H_
#define _UI_SWATCH_FIELD_H_

#include "UIField.h"

class UISwatchField : public UIField {

public:
  UISwatchField(const GUIPoint &position, const Color color);
  virtual ~UISwatchField() {};
  virtual void Draw(GUIWindow &w, int offset = 0);
  virtual void ProcessArrow(uint16_t mask);
  virtual void OnClick() {};

  virtual bool IsStatic();

protected:
protected:
  Color color_;
};

#endif
