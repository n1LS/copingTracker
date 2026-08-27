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

#ifndef _UI_FIELD_H_
#define _UI_FIELD_H_

#include "System/Console/Trace.h"
#include "UIFramework/BasicDatas/GUIPoint.h"
#include "UIFramework/SimpleBaseClasses/GUIWindow.h"
#include "View.h"

class UIField {
public:
  UIField(const GUIPoint &position);
  virtual ~UIField();
  virtual void Draw(GUIWindow &w, int offset = 0) = 0;
  virtual void OnClick() = 0; // ENTER pressed
  virtual void ProcessArrow(uint16_t mask) = 0;
  virtual void OnEditClick() {}; // EDIT pressed
  virtual void ProcessEditArrow(uint16_t mask) {};
  virtual void ProcessClear() {}; // EDIT+ENTER pressed
  void SetFocus();
  void ClearFocus();
  void SetActive(bool active);
  bool HasFocus();

  void SetPosition(const GUIPoint &);
  GUIPoint GetPosition();

  virtual int GetColumn() {
    return -1;
  }
  virtual void SetColumn(uint8_t column) {
  }

  virtual bool IsStatic();

  void DrawLabeledField(GUIWindow &w, GUIPoint position, char *buffer, int subSelectionOffset = -1,
                        int subSelectionLength = 1);

protected:
  uint8_t x_;
  uint8_t y_;
  bool focus_;
  bool active_ = true;
};
#endif
