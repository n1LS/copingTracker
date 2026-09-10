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

struct FieldConfiguration {
  Color backgroundColor = Theme::Input::bg(false);
  Color color = Theme::Input::fg(false);
  Color activeBackgroundColor = Theme::Input::bg(true);
  Color activeColor = Theme::Input::fg(true);
  Color cursor = Theme::Input::cursor;
};

static const FieldConfiguration instrumentFieldConfiguration = {
    Theme::InstrumentInput::bg(false), Theme::InstrumentInput::fg(false), Theme::InstrumentInput::bg(true),
    Theme::InstrumentInput::fg(true), Theme::InstrumentInput::cursor};

static const FieldConfiguration actionFieldConfiguration = {Theme::Button::bg(false), Theme::Button::fg(false),
                                                            Theme::Button::bg(true), Theme::Button::fg(true),
                                                            Theme::InstrumentInput::cursor};

static const FieldConfiguration instrumentActionFieldConfiguration = {
    Theme::PanelButton::bg(false), Theme::PanelButton::fg(false), Theme::PanelButton::bg(true),
    Theme::PanelButton::fg(true), Theme::InstrumentInput::cursor};

static const FieldConfiguration panelFieldConfiguration = {Theme::PanelInput::bg(false), Theme::PanelInput::fg(false),
                                                           Theme::PanelInput::bg(true), Theme::PanelInput::fg(true),
                                                           Theme::PanelInput::cursor};

static const FieldConfiguration defaultFieldConfiguration;

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

  virtual Variable *GetVariable() {
    return nullptr;
  }

  void SetBackgroundColor(Color color) {
    backgroundColor_ = color;
  }

  void SetLabelColor(Color color) {
    labelColor_ = color;
  }

  void SetFieldConfiguration(const FieldConfiguration config) {
    fieldConfig_ = config;
  }

  virtual int GetFocusWidth() {
    return focusWidth_;
  }
  virtual int GetFocusOffset() = 0;

  void SetPosition(const GUIPoint &);
  GUIPoint GetPosition();

  virtual int GetColumn() {
    return -1;
  }
  virtual void SetColumn(uint8_t column) {
  }

  virtual bool IsStatic();

  int DrawLabeledField(GUIWindow &w, GUIPoint position, char *buffer, int subSelectionOffset = 0,
                       int subSelectionLength = 0);

protected:
  uint8_t x_;
  uint8_t y_;
  bool focus_;
  bool active_ = true;
  int focusWidth_;
  Color backgroundColor_ = Theme::View::bg;
  Color labelColor_ = Theme::View::fg;
  FieldConfiguration fieldConfig_ = defaultFieldConfiguration;
};
#endif
