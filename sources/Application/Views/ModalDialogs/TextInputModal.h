/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _TEXT_INPUT_MODAL_H_
#define _TEXT_INPUT_MODAL_H_

#include "Application/Views/BaseClasses/ModalView.h"
#include "Application/Views/BaseClasses/UITextField.h"
#include "Externals/etl/include/etl/string.h"
#include <stdint.h>

// Longest value the on screen keyboard can edit. All current UITextField users
// are 16 (project / theme / instrument names).
#define MAX_TEXT_INPUT_LENGTH 16
#define MAX_TEXT_INPUT_LABEL_LENGTH MAX_UITEXTFIELD_LABEL_LENGTH

// Gameboy / LSDJ style on screen keyboard.
//
// Opened when the user presses ENTER on a UITextField. The grid shows one case
// at a time (EDIT swaps), with the digits and the filename safe punctuation
// always visible. Arrows move the grid highlight, ENTER types the highlighted
// character at the cursor.
//
// The modal is deliberately not templated even though UITextField is: the text
// is handed over as an etl::istring reference plus a max length, the same type
// erasure Variable uses for its string storage.
class TextInputModal : public ModalView {
public:
  static TextInputModal *Create(View &view, const char *label, const char *initialValue, uint8_t maxLength);
  virtual ~TextInputModal();

  virtual void Destroy() override;
  virtual void DrawView() override;
  virtual void OnPlayerUpdate(PlayerEventType, unsigned int) override {
  }
  virtual void OnFocus() override {
  }
  virtual void ProcessButtonMask(uint16_t mask, bool pressed) override;
  virtual void AnimationUpdate() override {
  }

  // Valid once the modal finished with TIM_ACCEPT.
  const etl::istring &GetValue() const {
    return value_;
  }

  enum ReturnCode { TIM_CANCEL = 0, TIM_ACCEPT = 1 };

private:
  TextInputModal(View &view, const char *label, const char *initialValue, uint8_t maxLength);

  // Character currently under the grid highlight.
  char highlightedChar() const;
  // Number of columns actually occupied by the given grid row.
  uint8_t columnsInRow(uint8_t row) const;

  void insertChar(char c);
  void deleteChar();
  void moveGrid(int8_t dx, int8_t dy);
  void drawHighlight(int x, int y, int width);

  static bool inUse_;
  static void *storage_;

  etl::string<MAX_TEXT_INPUT_LENGTH> value_;
  etl::string<MAX_TEXT_INPUT_LABEL_LENGTH> label_;

  uint8_t maxLength_;
  uint8_t cursor_ = 0;  // insertion point within value_
  uint8_t gridRow_ = 0; // 0..3
  uint8_t gridCol_ = 0; // 0..12
  bool upperCase_ = true;
};

#endif // _TEXT_INPUT_MODAL_H_
