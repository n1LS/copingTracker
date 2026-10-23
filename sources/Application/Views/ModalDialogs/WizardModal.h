/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _WIZARD_MODAL_H_
#define _WIZARD_MODAL_H_

#include "Application/Views/BaseClasses/ModalView.h"
#include <stdint.h>

// Base class for multi step wizards.
//
// Owns the common layout: a window with a title and a right aligned progress
// bar, a prompt line, a panel with selectable / editable rows (kVisibleRows of
// them) and a bottom row with up to two buttons. Subclasses describe the steps
// through the hooks below and draw the rows of the current step.
//
// The flow is linear: the left button goes one step back (Cancel on the first
// step), the right button or ENTER on a row goes one step forward. On the last
// step that finishes the wizard with WM_ACCEPT.
//
// Controls: UP/DOWN move between the rows and the button row. On the button
// row LEFT/RIGHT pick a button. ENTER is acted on when released. Holding ENTER
// and pressing the arrows edits the focused row (LEFT/RIGHT by 1, UP/DOWN by
// 16).
//
// Subclasses keep the "static storage + placement new" Create()/Destroy()
// pattern of the other modals, the base can't know their size.
class WizardModal : public ModalView {
public:
  enum ReturnCode { WM_CANCEL = 0, WM_ACCEPT = 1 };

  // What the base needs to know to lay out one step.
  struct StepInfo {
    const char *prompt;               // line above the panel
    const char *leftLabel = nullptr;  // nullptr: "Cancel" on the first step, "Back" otherwise
    const char *rightLabel = nullptr; // nullptr: no right button, ENTER on a row advances
  };

  virtual ~WizardModal();

  virtual void DrawView() override;
  virtual void OnPlayerUpdate(PlayerEventType, unsigned int) override {
  }
  virtual void OnFocus() override {
  }
  virtual void ButtonDown(uint16_t newButton, uint16_t mask) override;
  virtual void ButtonUp(uint16_t button, uint16_t mask) override;
  virtual void AnimationUpdate() override {
  }

protected:
  static constexpr uint8_t kVisibleRows = 4;

  WizardModal(View &view, int width = 26, int height = 14);

  // --- hooks ---------------------------------------------------------------

  virtual const char *GetTitle() = 0;
  virtual uint8_t NumSteps() const = 0;
  virtual StepInfo GetStepInfo(uint8_t step) = 0;
  // Number of selectable rows of a step, the button row follows them.
  virtual uint8_t RowCount(uint8_t step) = 0;
  // Draws the rows of a step into the panel, see the draw helpers below.
  virtual void DrawStep(uint8_t step) = 0;

  // A step was entered. forward is false when coming back from the next step.
  // Typically builds data and positions row_ (the focus is on row 0 by default).
  virtual void OnEnterStep(uint8_t step, bool forward) {
  }
  // The focus moved onto a (non button) row.
  virtual void OnRowFocused(uint8_t step, uint8_t row) {
  }
  // ENTER is held and an arrow was pressed on a row.
  virtual void OnValueChange(uint8_t step, uint8_t row, int delta) {
  }
  // The user wants to leave a step forward. row is the focused row, or the
  // button row for the right button. Return false to stay, e.g. when the choice
  // isn't available. Also stores the choice. Called on the last step too, right
  // before OnAccept().
  virtual bool OnNext(uint8_t step, uint8_t row) {
    return true;
  }
  // The last step was left forward. Return false to end with WM_CANCEL instead,
  // e.g. when allocating something failed.
  virtual bool OnAccept() {
    return true;
  }

  // --- draw helpers (call from DrawStep) -----------------------------------

  // Full width selectable item on a panel line (0..kVisibleRows-1), shows the
  // ENTER glyph when focused.
  void drawListItem(int line, const char *text, bool focused);
  // "label  [value]" on a panel line.
  void drawLabeledField(int line, const char *label, const char *value, bool focused);
  // Scrollbar on the right edge of the panel for a list of total rows, shown
  // only when the list doesn't fit (uses scroll_).
  void drawListScrollBar(uint8_t total);
  // Plain text on a panel line.
  void drawPanelText(int line, const char *text);
  // Instrument style field: square blocks when idle, rounded ends when focused
  // (like UIField). x/y are in window coordinates.
  void drawField(int x, int y, const char *text, int width, bool focused, char indicator = ' ');

  // --- navigation for subclasses -------------------------------------------

  uint8_t step_ = 0;
  uint8_t row_ = 0;    // focused row, RowCount(step_) is the button row
  uint8_t scroll_ = 0; // first visible row of a scrolling list

  bool isButtonRow() {
    return row_ == RowCount(step_);
  }

private:
  enum Button : uint8_t { BTN_LEFT = 0, BTN_RIGHT };

  void goToStep(uint8_t step, bool forward);
  void advance();
  void back();
  void activate();
  void updateScroll();
  void drawProgress();
  void drawButtons(const StepInfo &info);

  // lists longer than the visible rows give up a column for the scrollbar
  bool scrolls() {
    return RowCount(step_) > kVisibleRows;
  }
  int fieldWidth() {
    return width_ - 6 - (scrolls() ? 1 : 0);
  }
  int valueWidth();

  int width_;
  int height_;
  Button button_ = BTN_LEFT;
  bool enterDown_ = false;
  bool valueChanged_ = false; // ENTER was used to edit, don't act on its release
};

#endif // _WIZARD_MODAL_H_
