/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _PHRASE_EDITING_VIEW_H_
#define _PHRASE_EDITING_VIEW_H_

#include "BaseClasses/UIHexVarField.h"
#include "ScreenView.h"
#include "ViewData.h"

// The fields of a step that are edited one by one (the columns of the phrase view)
enum class StepField : uint8_t { Note, Instrument, Volume, Command1, Param1, Command2, Param2 };

// Base of the views that edit the current phrase (viewData_->currentPhrase_): the phrase view and the
// piano roll. They share one place in the navigation map, NAV + NAV switches between them and they
// take the same input for navigation, playback and for switching to another phrase.
class PhraseEditingView : public ScreenView {
public:
  PhraseEditingView(GUIWindow &w, ViewData *viewData);
  ~PhraseEditingView();

  // NAV + arrows: the same places from both views
  bool navigationDown(uint16_t direction) override;
  // NAV + NAV: switch to the other view
  bool OnNavDoubleTap() override;
  // remembers this view as the one the other views open when they navigate to the phrase
  virtual void OnFocus() override;
  // called with the command that was picked in the command picker (see openCommandPicker)
  virtual void onCommandPicked(Token command) = 0;
  // the drawing of what plays happens in AnimationUpdate, this can run on another core
  virtual void OnPlayerUpdate(PlayerEventType, unsigned int tick = 0) override {
    needsUIUpdate_ = true;
  }

protected:
  // the view NAV + NAV switches to
  virtual ViewType otherView() const = 0;
  // the step (row of the phrase) the cursor is in
  virtual int cursorStep() const;
  // called after another phrase was switched to, the view has to show it
  virtual void onPhraseSwitched() = 0;
  // Audition: plays the step of the cursor with its instrument. Without an instrument there is nothing to hear.
  // If the player is not running it only starts when startIfNotRunning is set; if it is auditioning
  // it restarts to pick up what changed.
  void startAudition(bool startIfNotRunning);
  void stopAudition();

  // Input of both views. They return true if the press was handled.
  // PLAY: play the phrase, NAV + PLAY: play it from the previous position
  bool processPlaybackInput(uint16_t newButton, uint16_t mask);
  // EDIT + LEFT / RIGHT: the phrase of the previous / next channel,
  // EDIT + UP / DOWN: the previous / next phrase in the chain
  bool processPhraseSwitchInput(uint16_t newButton, uint16_t mask);

  // the steps of the phrase that is edited: to read them. editStep / editSteps record what they hand out in the undo
  // history, call them right before the change.
  const PhraseStep *currentSteps() const {
    return viewData_->song_->phrase_.Steps(viewData_->currentPhrase_);
  }
  PhraseStep &editStep(int step) {
    return viewData_->song_->phrase_.EditStep(viewData_->currentPhrase_, step);
  }
  PhraseStep *editSteps() {
    return viewData_->song_->phrase_.EditSteps(viewData_->currentPhrase_);
  }

  // the fields that change what can be heard: they restart the preview when they change
  static bool isAudible(StepField field) {
    return field == StepField::Note || field == StepField::Instrument || field == StepField::Volume;
  }

  // the instrument that is used at a step: the last one set at or before it, or NO_INSTRUMENT
  uint8_t getEffectiveInstrumentForStep(int step);

  // Changes the note of a step by one step in the scale (smallChange) or by an octave, up
  // (sign > 0) or down, with the project's scale and key. It is the note editing of the phrase view.
  void changeNote(int step, int sign, bool smallChange);
  // The same as the phrase view does it for an arrow: LEFT / RIGHT one step in the scale, UP / DOWN an octave
  void changeNote(int step, ViewUpdateDirection direction);

  // The editing of the other fields of a step, one step of a value as the phrase view does it for the cursor
  // (LEFT / RIGHT -1 / +1, UP / DOWN +16 / -16 for numbers). They remember the value as the last one to paste.
  void changeInstrument(int step, ViewUpdateDirection direction);
  void changeVolume(int step, ViewUpdateDirection direction);
  void changeCommand(int step, bool second, ViewUpdateDirection direction);
  // the parameter is edited digit by digit by cmdEditField_: LEFT / RIGHT select the digit, UP / DOWN change it
  void changeCommandParam(int step, bool second, ViewUpdateDirection direction);
  // makes cmdEdit_ the parameter of the step, call it before changeCommandParam if the cursor was elsewhere
  void loadParamEditor(int step, bool second);
  // An empty field gets the last value used, a field that has one makes it the last value to paste.
  // Returns true if the step changed.
  bool pasteLastValue(int step, StepField field);
  // Opens the command picker (CommandView) for the command of a step, if the picker is enabled in the config.
  // It starts with the command of the step, or the last command used if it has none. The result comes back
  // in onCommandPicked. Returns false if the picker is disabled.
  bool openCommandPicker(int step, bool second);
  // Sets the command of a step (a command that is none clears the parameter as well)
  void setCommand(int step, bool second, Token command);
  // The clipboard (in ViewData, shared by both views). Copies whole steps, `count` steps from `first`.
  void copyStepsToClipboard(int first, int count);
  // Pastes the clipboard over the steps from `step` (wrapping around at the end of the phrase), only the fields
  // that are in it. Returns the number of steps it covers (0 if the clipboard is empty).
  int pasteClipboardAt(int step);
  // Moves the content of the steps first..last (only the fields firstField..lastField, in the order of the
  // enum) one step up (delta -1) or down (delta +1), the step in the way jumps to the step that was just emptied
  // at the other end. Returns false and changes nothing at the first / last step.
  bool nudgeSteps(int first, int last, StepField firstField, StepField lastField, int delta);
  // Clears all fields of `count` steps from `first`
  void clearSteps(int first, int count);
  // Clears a field: no instrument, no volume, a command without parameter, a parameter of 0.
  void clearStepField(int step, StepField field);

  // What plays: the step the first channel that plays the phrase of this view is in (false if none) and the
  // indicator of the first queued chain that holds the chain of this view (live mode, nullptr if none).
  bool findPlayingStep(int &step, bool &muted);
  const char *findLiveIndicator();

  // Set by OnPlayerUpdate, so that all drawing is done by the main core in AnimationUpdate
  bool needsUIUpdate_ = false;

  // the editor of the parameter of a command, one for both views (it keeps the selected digit)
  static Variable cmdEdit_;
  static GUIPoint cmdEditPos_;
  static UIHexVarField cmdEditField_;

  void playPhrase(bool startFromPrevious);
  void warpToNeighbour(int offset);
  void warpInChain(int offset);
  void enterInstrumentUnderCursor();
};

#endif
