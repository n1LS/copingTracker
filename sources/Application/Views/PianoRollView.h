/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _PIANO_ROLL_VIEW_H_
#define _PIANO_ROLL_VIEW_H_

#include "PhraseEditingView.h"

// Alternative view for the phrase view: the same phrase (viewData_->currentPhrase_) as a piano roll.
// NAV + NAV (double tap) toggles between the two. Work in progress, see TODO.md for what is missing.
class PianoRollView : public PhraseEditingView {
public:
  PianoRollView(GUIWindow &w, ViewData *viewData);
  ~PianoRollView();

  virtual void ButtonDown(uint16_t newButton, uint16_t mask) override;
  virtual void DrawView() override;
  virtual void OnFocus() override;
  virtual void ButtonUp(uint16_t button, uint16_t mask) override;
  virtual void AnimationUpdate() override;

protected:
  // PhraseEditingView
  ViewType otherView() const override {
    return VT_PHRASE;
  }
  void onPhraseSwitched() override;

private:
  void centerOnNotes();
  void drawNoteLabels();
  void drawStepIndex();
  void drawNoteGrid();
  void drawScrollIndicators();
  void drawStepInfo();
  // static help under the fields: how to get to the other part of the view
  void drawHelpText();
  // the playback position, in the row below the grid, in the column of the step that plays
  void drawPlaybackIndicators();

  // The cursor is a note (row) and a step (column). The step is the shared phrase cursor,
  // viewData_->phraseCurPos_, so it is the one of the phrase view as well.
  void moveCursorNote(int delta);
  void moveCursorStep(int delta);
  // ENTER: the note of the step becomes the note of the cursor
  void placeNote();
  // ENTER + EDIT: removes the note of the step, or sets a note off if it has none
  void removeNote();
  // ENTER + UP / DOWN: moves the note of the step one step up or down in the scale, the cursor follows it
  void moveNote(int sign);
  // ENTER + LEFT / RIGHT: moves the note of the step (with its instrument and volume) to the previous / next
  // step, a note that is already there is deleted. The cursor follows.
  void moveNoteInTime(int delta);
  // ALT + LEFT / RIGHT: the focus is on the grid or on the fields of the step on the right
  void focusPanel(bool panel);
  // arrows in the fields: UP / DOWN change the row, LEFT / RIGHT the command and its parameter
  void movePanelField(uint16_t direction);
  // the input while the fields have the focus: ENTER pastes the last value, ENTER + arrows edit the selected
  // field as in the phrase view, ENTER + EDIT clears it
  void processPanelInput(uint16_t newButton, uint16_t mask);
  void editPanelField(ViewUpdateDirection direction);
  void clearPanelField();
  // the cursor follows the note of the step, as after moving it
  void followNoteOfStep();
  // the field of the panel as it is drawn, for the focus rect
  GUIRect panelFieldRect();
  // scrolls the view if the cursor is not inside it
  void scrollToCursor();
  // keeps the cursor inside the visible range, on the note of its step if that is visible
  void placeCursor();
  int cursorRow() const;
  // the note of a row of the grid, the highest note is in the top row (row 0)
  int rowNote(int row) const;

  // the lowest note that is visible, the view shows the 16 notes from here up
  int bottomNote_;
  // the note the cursor is on, always inside the visible range
  int cursorNote_;
  // the column the playback indicator was drawn in, -1 if there is none
  int8_t lastPlayingStep_ = -1;
  // the focus is in the fields on the right, the selection in the grid is drawn inverted then
  bool panelFocus_ = false;
  StepField panelField_ = StepField::Note;
};

#endif
