/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "PianoRollView.h"
#include "Application/Instruments/I_Instrument.h"
#include "Application/Instruments/InstrumentBank.h"
#include "Application/Model/Scale.h"
#include "Application/Model/Song.h"
#include "Application/Player/Player.h"
#include "Application/Utils/ContextLegend.generated.h"
#include "Application/Utils/char.h"
#include "Foundation/Constants/GraphicCharacters.h"
#include "Foundation/Constants/SpecialCharacters.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>

// one note per row, 16 rows
static constexpr int VISIBLE_NOTES = 16;
// row positions: the grid (the highest note in the first row) and the fields right of it, which start in
// the row of the step index
static constexpr int GRID_Y = 3;
static constexpr int STEP_INDEX_Y = GRID_Y - 1;
static constexpr int PANEL_Y = STEP_INDEX_Y;
// the help under the fields, in the last two rows of the grid
static constexpr int HELP_Y = GRID_Y + 16 - 2;
// column positions
static constexpr int PIANO_X = 0;
static constexpr int LABEL_X = 4;
static constexpr int GRID_X = 8;
static constexpr int STEP_INFO_X = 25;
// scrolling goes from note 0 at the bottom to HIGHEST_NOTE at the top
static constexpr int MAX_BOTTOM_NOTE = HIGHEST_NOTE - (VISIBLE_NOTES - 1);
// the view starts on E3..G4 so middle C is about in the middle
static constexpr int DEFAULT_BOTTOM_NOTE = 40;

// What is drawn in the note grid, one cell per step and note
static constexpr char GLYPH_EMPTY = CHAR(gchar_piano_roll_empty_s); // nothing is drawn for the step and note
static constexpr char GLYPH_SINGLE = CHAR(gchar_piano_roll_note_s); // a note, the next step is a note or a note off
static constexpr char GLYPH_START = CHAR(gchar_piano_roll_note_start_s); // a note that goes on, the next step is empty
static constexpr char GLYPH_SUSTAIN = CHAR(gchar_piano_roll_note_mid_s); // the note goes on, the next step is empty too
static constexpr char GLYPH_END =
    CHAR(gchar_piano_roll_note_end_s); // the last step of a note, the next step is not empty
static constexpr char GLYPH_NOTE_OFF = CHAR(gchar_piano_roll_note_off_s); // a note off, in the row of the note it ends
// the pitch of a glyph that belongs to no note: it is drawn in every row of its step
static constexpr int ALL_NOTES = -1;

// What is drawn for a step of the phrase
struct GridCell {
  char glyph; // 0 if nothing is drawn for the step
  int pitch;  // the row it belongs to, ALL_NOTES for a note off that has no note to end
  Color color;
};

// The cell for a step of the phrase. `sounding` is the pitch of the note that is still held, or -1,
// and is updated for the next step, so call this for the steps in order.
// The end of the phrase counts as empty: a note goes on into the next phrase.
static GridCell noteGridGlyph(const PhraseStep *steps, int step, int &sounding) {
  const uint8_t note = steps[step].note;
  const uint8_t next = (step + 1 < STEPS_PER_PHRASE) ? steps[step + 1].note : NO_NOTE;

  if (note == NO_NOTE) {
    if (sounding < 0) {
      return {0, 0, Theme::View::fg};
    }
    return {(next == NO_NOTE) ? GLYPH_SUSTAIN : GLYPH_END, sounding, Theme::View::fg};
  }

  if (note == NOTE_OFF) {
    // without a note to end (e.g. the one it ends is in the previous phrase) it has no pitch
    const int pitch = (sounding < 0) ? ALL_NOTES : sounding;
    sounding = -1;
    return {GLYPH_NOTE_OFF, pitch, Theme::Data::negative};
  }

  sounding = note;
  return {(next == NO_NOTE) ? GLYPH_START : GLYPH_SINGLE, note, Theme::View::fg};
}

// The step a value of `step` comes from: the step itself if it has the value, otherwise (if `sticky`) the
// closest step before it that has one. Values are valid until a new note or a note off comes in: the
// values of the step of that note itself still count, what was set before it does not.
template <typename Has> static int valueSource(const PhraseStep *steps, int step, bool sticky, Has has) {
  for (int i = step; i >= 0; --i) {
    if (has(steps[i])) {
      return i;
    }
    if (!sticky || steps[i].note != NO_NOTE) {
      return -1;
    }
  }
  return -1;
}

// Whether the phrase has notes above or below the visible range
static void notesOutsideRange(const PhraseStep *steps, int bottomNote, bool &above, bool &below) {
  const int topNote = bottomNote + VISIBLE_NOTES - 1;

  above = false;
  below = false;
  for (int step = 0; step < STEPS_PER_PHRASE; step++) {
    const uint8_t note = steps[step].note;
    if (note == NO_NOTE || note == NOTE_OFF) {
      continue;
    }
    above = above || note > topNote;
    below = below || note < bottomNote;
  }
}

// The lowest note of the visible range that shows the most notes of the phrase. Among equally good
// ranges the one that has the notes it shows in the middle wins. A phrase without notes gets the default.
static int bestBottomNote(const PhraseStep *steps) {
  int best = DEFAULT_BOTTOM_NOTE;
  int bestCount = 0;
  int bestOffCenter = 0;

  for (int bottom = 0; bottom <= MAX_BOTTOM_NOTE; bottom++) {
    const int top = bottom + VISIBLE_NOTES - 1;

    int count = 0;
    int lowest = HIGHEST_NOTE;
    int highest = 0;
    for (int step = 0; step < STEPS_PER_PHRASE; step++) {
      const uint8_t note = steps[step].note;
      if (note == NO_NOTE || note == NOTE_OFF || note < bottom || note > top) {
        continue;
      }
      count++;
      lowest = std::min<int>(lowest, note);
      highest = std::max<int>(highest, note);
    }
    if (count == 0) {
      continue;
    }

    // twice the distance between the middle of the range and the middle of the notes in it
    const int offCenter = std::abs(2 * bottom + (VISIBLE_NOTES - 1) - (lowest + highest));
    if (count > bestCount || (count == bestCount && offCenter < bestOffCenter)) {
      best = bottom;
      bestCount = count;
      bestOffCenter = offCenter;
    }
  }

  return best;
}

PianoRollView::PianoRollView(GUIWindow &w, ViewData *viewData)
    : PhraseEditingView(w, viewData), bottomNote_(DEFAULT_BOTTOM_NOTE),
      cursorNote_(DEFAULT_BOTTOM_NOTE + VISIBLE_NOTES / 2) {
}

PianoRollView::~PianoRollView() {
}

void PianoRollView::OnFocus() {
  // the other views that lead to the phrase view open this one next time
  PhraseEditingView::OnFocus();

  // the grid has the focus when the view opens, there is no selection
  panelFocus_ = false;
  selecting_ = false;
  menu_ = false;

  centerOnNotes();
  placeCursor();
}

void PianoRollView::centerOnNotes() {
  bottomNote_ = bestBottomNote(currentSteps());
}

void PianoRollView::onPhraseSwitched() {
  // another phrase has its notes somewhere else, and the selection was in the old one
  selecting_ = false;
  centerOnNotes();
  placeCursor();
  isDirty_ = true;
}

// one of the four arrows (the button that was just pressed is a single button)
static bool isDirection(uint16_t button) {
  return (button & BM_DIRECTIONAL) != 0;
}

static ViewUpdateDirection toDirection(uint16_t button) {
  switch (button) {
    case BM_LEFT:
      return VUD_LEFT;
    case BM_RIGHT:
      return VUD_RIGHT;
    case BM_UP:
      return VUD_UP;
    default:
      return VUD_DOWN;
  }
}

void PianoRollView::ButtonDown(uint16_t newButton, uint16_t mask) {
  // playback and switching to another phrase, as in the phrase view
  if (processPlaybackInput(newButton, mask) || processPhraseSwitchInput(newButton, mask)) {
    return;
  }

  // ALT + RIGHT / LEFT: to the fields of the step on the right / back to the grid
  if (IsChord(newButton, mask, BM_RIGHT, BM_ALT)) {
    focusPanel(true);
    return;
  }
  if (IsChord(newButton, mask, BM_LEFT, BM_ALT)) {
    focusPanel(false);
    return;
  }

  // the fields have their own input
  if (panelFocus_) {
    processPanelInput(newButton, mask);
    return;
  }

  // selecting and the clipboard, as in the phrase view
  if (IsPair(newButton, mask, BM_EDIT, BM_ALT)) {
    if (selecting_) {
      extendSelectionToPhrase();
    } else {
      // the menu: undo / redo, and any other button starts the selection
      menu_ = true;
      isDirty_ = true;
    }
    return;
  }
  if (menu_) {
    menu_ = false;
    startSelection();
  }
  if (selecting_) {
    if (IsChord(newButton, mask, BM_EDIT)) {
      copySelection();
      return;
    }
    if (IsChord(newButton, mask, BM_ENTER, BM_ALT)) {
      cutSelection();
      return;
    }
    // ENTER + EDIT + LEFT / RIGHT move the selected steps
    if ((newButton == BM_LEFT || newButton == BM_RIGHT) && mask == (BM_ENTER | BM_EDIT)) {
      moveSelectionInTime(newButton == BM_RIGHT ? +1 : -1);
      return;
    }
    // ENTER + arrows change the notes of the selection
    if (isDirection(newButton) && mask == BM_ENTER) {
      transposeSelection(toDirection(newButton));
      return;
    }
    // only the arrows are left: LEFT / RIGHT extend the selection, UP / DOWN move the cursor note
    if (!(isDirection(newButton) && mask == 0)) {
      return;
    }
  } else if (IsChord(newButton, mask, BM_ENTER, BM_ALT)) {
    pasteClipboard();
    return;
  }

  if (IsChord(newButton, mask, BM_ENTER)) {
    placeNote();
  } else if (IsChord(newButton, mask, BM_EDIT, BM_ENTER)) {
    removeNote();
  } else if (IsChord(newButton, mask, BM_UP, BM_ENTER)) {
    moveNote(+1);
  } else if (IsChord(newButton, mask, BM_DOWN, BM_ENTER)) {
    moveNote(-1);
  } else if (IsChord(newButton, mask, BM_LEFT, BM_ENTER)) {
    moveNoteInTime(-1);
  } else if (IsChord(newButton, mask, BM_RIGHT, BM_ENTER)) {
    moveNoteInTime(+1);
  } else if (IsChord(newButton, mask, BM_UP)) {
    moveCursorNote(+1);
  } else if (IsChord(newButton, mask, BM_DOWN)) {
    moveCursorNote(-1);
  } else if (IsChord(newButton, mask, BM_LEFT)) {
    moveCursorStep(-1);
  } else if (IsChord(newButton, mask, BM_RIGHT)) {
    moveCursorStep(+1);
  }
}

void PianoRollView::ButtonUp(uint16_t button, uint16_t mask) {
  // there is nothing to do in the menu without ALT: the selection starts here instead of with the next button (when
  // the menu was used for undo / redo it just ends)
  if (menu_ && button == BM_ALT) {
    menu_ = false;
    if (!menuUsedUndo_) {
      startSelection();
    }
    isDirty_ = true;
  }
  // stop the preview when ENTER is released, as in the phrase view
  if (!(mask & BM_ENTER)) {
    stopAudition();
  }
}

ContextLegend PianoRollView::currentContextLegend() const {
  if (menu_) {
    return ContextLegend::RollMenu;
  }
  return selecting_ ? ContextLegend::RollSelection : ContextLegend::None;
}

// the menu that ALT + EDIT opened stays open while ALT is held
void PianoRollView::AfterUndoRedo() {
  View::AfterUndoRedo();
  menu_ = menuOpen_;
}

int PianoRollView::selectionFirst() const {
  return std::min<int>(selectionAnchor_, viewData_->phraseCurPos_);
}

int PianoRollView::selectionLast() const {
  return std::max<int>(selectionAnchor_, viewData_->phraseCurPos_);
}

bool PianoRollView::isSelected(int step) const {
  return selecting_ && step >= selectionFirst() && step <= selectionLast();
}

void PianoRollView::startSelection() {
  selecting_ = true;
  selectionAnchor_ = static_cast<int8_t>(viewData_->phraseCurPos_);
  isDirty_ = true;
}

void PianoRollView::endSelection() {
  // the cursor goes back to where the selection started, as in the phrase view
  viewData_->phraseCurPos_ = selectionAnchor_;
  selecting_ = false;
  isDirty_ = true;
}

void PianoRollView::extendSelectionToPhrase() {
  // all steps, towards the end the cursor is on (a second time it flips to the other end)
  if (viewData_->phraseCurPos_ < selectionAnchor_) {
    viewData_->phraseCurPos_ = 0;
    selectionAnchor_ = STEPS_PER_PHRASE - 1;
  } else {
    selectionAnchor_ = 0;
    viewData_->phraseCurPos_ = STEPS_PER_PHRASE - 1;
  }
  isDirty_ = true;
}

void PianoRollView::copySelection() {
  copyStepsToClipboard(selectionFirst(), selectionLast() - selectionFirst() + 1);
  endSelection();
}

void PianoRollView::cutSelection() {
  const int first = selectionFirst();
  const int count = selectionLast() - first + 1;
  copyStepsToClipboard(first, count);
  clearSteps(first, count);
  endSelection();
}

void PianoRollView::transposeSelection(ViewUpdateDirection direction) {
  const PhraseStep *steps = currentSteps();

  // the steps with a note, an empty step or a note off has nothing to transpose
  for (int step = selectionFirst(); step <= selectionLast(); step++) {
    if (steps[step].note < NOTE_OFF) {
      changeNote(step, direction);
    }
  }

  followNoteOfStep();
  isDirty_ = true;
}

void PianoRollView::moveSelectionInTime(int delta) {
  // whole steps move, the step in the way ends up where the selection was at the other end
  if (!nudgeSteps(selectionFirst(), selectionLast(), StepField::Note, StepField::Param2, delta)) {
    return;
  }

  // the selection and the cursor move with the steps
  selectionAnchor_ = static_cast<int8_t>(selectionAnchor_ + delta);
  viewData_->phraseCurPos_ += delta;
  followNoteOfStep();
  isDirty_ = true;
}

void PianoRollView::pasteClipboard() {
  // overwrites the steps from the cursor, the cursor goes to the step after them
  const int height = pasteClipboardAt(viewData_->phraseCurPos_);
  if (height == 0) {
    return;
  }
  viewData_->phraseCurPos_ = (viewData_->phraseCurPos_ + height) % STEPS_PER_PHRASE;
  followNoteOfStep();
  isDirty_ = true;
}

void PianoRollView::focusPanel(bool panel) {
  if (panelFocus_ == panel) {
    return;
  }
  panelFocus_ = panel;
  // the fields edit single values, there is no selection in them
  selecting_ = false;
  isDirty_ = true;
}

// the row of a field in the panel: note, instrument, volume, command 1, command 2
static const int panelRow[7] = {0, 1, 2, 3, 3, 4, 4};

static bool isParam(StepField field) {
  return field == StepField::Param1 || field == StepField::Param2;
}

static StepField panelFieldAt(int row, bool param) {
  switch (row) {
    case 0:
      return StepField::Note;
    case 1:
      return StepField::Instrument;
    case 2:
      return StepField::Volume;
    case 3:
      return param ? StepField::Param1 : StepField::Command1;
    default:
      return param ? StepField::Param2 : StepField::Command2;
  }
}

void PianoRollView::movePanelField(uint16_t direction) {
  int row = panelRow[(int)panelField_];
  bool param = isParam(panelField_);

  switch (direction) {
    case BM_UP:
      row = std::max(row - 1, 0);
      break;
    case BM_DOWN:
      row = std::min(row + 1, 4);
      break;
    case BM_LEFT:
      param = false;
      break;
    case BM_RIGHT:
      // only the commands have a parameter
      param = row >= 3;
      break;
  }

  const StepField next = panelFieldAt(row, param && row >= 3);
  if (next != panelField_) {
    panelField_ = next;
    isDirty_ = true;
  }
}

GUIRect PianoRollView::panelFieldRect() {
  // rows and columns of drawStepInfo: every field has a title row and a value row, the commands have their
  // parameter right of them
  const int y = PANEL_Y;
  switch (panelField_) {
    case StepField::Note:
      return GUIRect(STEP_INFO_X, y + 1, 3, 1);
    case StepField::Instrument:
      return GUIRect(STEP_INFO_X, y + 3, 2, 1);
    case StepField::Volume:
      return GUIRect(STEP_INFO_X, y + 5, 1, 1);
    case StepField::Command1:
      return GUIRect(STEP_INFO_X, y + 7, 3, 1);
    case StepField::Param1:
      return GUIRect(STEP_INFO_X + 3, y + 7, 4, 1);
    case StepField::Command2:
      return GUIRect(STEP_INFO_X, y + 9, 3, 1);
    case StepField::Param2:
      return GUIRect(STEP_INFO_X + 3, y + 9, 4, 1);
  }
  return GUIRect(STEP_INFO_X, y + 1, 3, 1);
}

void PianoRollView::processPanelInput(uint16_t newButton, uint16_t mask) {
  if (isDirection(newButton) && mask == 0) {
    movePanelField(newButton);
  } else if (IsChord(newButton, mask, BM_ENTER)) {
    // as in the phrase view: the command picker (if it is enabled) opens on a command, an empty field gets the
    // last value, the values that can be heard start the preview
    const int step = viewData_->phraseCurPos_;
    if (panelField_ == StepField::Command1 || panelField_ == StepField::Command2) {
      openCommandPicker(step, panelField_ == StepField::Command2);
    }
    pasteLastValue(step, panelField_);
    followNoteOfStep();
    isDirty_ = true;
    if (isAudible(panelField_)) {
      stopAudition();
      startAudition(true);
    }
  } else if (isDirection(newButton) && mask == BM_ENTER) {
    editPanelField(toDirection(newButton));
  } else if (IsChord(newButton, mask, BM_EDIT, BM_ENTER)) {
    clearPanelField();
  }
}

void PianoRollView::onCommandPicked(Token command) {
  if (panelField_ != StepField::Command1 && panelField_ != StepField::Command2) {
    return;
  }

  setCommand(viewData_->phraseCurPos_, panelField_ == StepField::Command2, command);
  isDirty_ = true;
}

void PianoRollView::editPanelField(ViewUpdateDirection direction) {
  const int step = viewData_->phraseCurPos_;
  const PhraseStep &data = currentSteps()[step];

  switch (panelField_) {
    case StepField::Note:
      // there is no note to change in an empty step
      if (data.note == NO_NOTE) {
        return;
      }
      // as in the phrase view: LEFT / RIGHT one step in the scale, UP / DOWN an octave
      changeNote(step, direction);
      followNoteOfStep();
      break;
    case StepField::Instrument:
      changeInstrument(step, direction);
      break;
    case StepField::Volume:
      changeVolume(step, direction);
      break;
    case StepField::Command1:
    case StepField::Command2:
      changeCommand(step, panelField_ == StepField::Command2, direction);
      break;
    case StepField::Param1:
    case StepField::Param2:
      loadParamEditor(step, panelField_ == StepField::Param2);
      changeCommandParam(step, panelField_ == StepField::Param2, direction);
      break;
  }

  isDirty_ = true;

  // the preview restarts with what changed (commands do not change what can be heard)
  if (isAudible(panelField_)) {
    startAudition(false);
  }
}

void PianoRollView::clearPanelField() {
  if (panelField_ == StepField::Note) {
    // as ENTER + EDIT in the grid: removes the note with its instrument and volume, an empty step gets a note off
    removeNote();
    return;
  }

  clearStepField(viewData_->phraseCurPos_, panelField_);
  isDirty_ = true;
}

void PianoRollView::followNoteOfStep() {
  const uint8_t note = currentSteps()[viewData_->phraseCurPos_].note;
  if (note < NOTE_OFF) {
    cursorNote_ = std::clamp<int>(note, 0, HIGHEST_NOTE);
    scrollToCursor();
  }
}

void PianoRollView::placeNote() {
  PhraseStep &step = editStep(viewData_->phraseCurPos_);

  // as in the phrase view: a step without a note gets the last instrument,
  // and the note becomes the last note and instrument to paste
  const bool hadNote = step.note < NOTE_OFF;
  step.note = static_cast<uint8_t>(cursorNote_);
  if (!hadNote) {
    step.setInstrument(static_cast<uint8_t>(viewData_->phraseLastInstrument_));
  }
  viewData_->phraseLastNote_ = step.note;
  viewData_->phraseLastInstrument_ = step.getInstrument();

  isDirty_ = true;

  // preview the note as long as ENTER is held, ButtonUp stops it
  stopAudition();
  startAudition(true);
}

// what a note owns goes with it: its instrument and volume (the commands stay in their step)
static void clearNote(PhraseStep &step) {
  step.note = NO_NOTE;
  step.setInstrument(NO_INSTRUMENT);
  step.setVolume(NO_VOLUME);
}

void PianoRollView::removeNote() {
  PhraseStep &step = editStep(viewData_->phraseCurPos_);

  if (step.note == NO_NOTE) {
    // cutting an empty note slot adds a note off, as in the phrase view
    step.note = NOTE_OFF;
  } else {
    // ... and cutting a note takes its instrument and volume with it
    clearNote(step);
  }

  isDirty_ = true;
}

int PianoRollView::cursorRow() const {
  // the highest note is in the top row
  return bottomNote_ + (VISIBLE_NOTES - 1) - cursorNote_;
}

int PianoRollView::rowNote(int row) const {
  return bottomNote_ + (VISIBLE_NOTES - 1 - row);
}

void PianoRollView::moveCursorNote(int delta) {
  const int next = std::clamp(cursorNote_ + delta, 0, HIGHEST_NOTE);
  if (next == cursorNote_) {
    return;
  }
  cursorNote_ = next;
  scrollToCursor();
  isDirty_ = true;
}

void PianoRollView::scrollToCursor() {
  // the view only scrolls when the cursor would leave it
  if (cursorNote_ < bottomNote_) {
    bottomNote_ = cursorNote_;
  } else if (cursorNote_ > bottomNote_ + VISIBLE_NOTES - 1) {
    bottomNote_ = cursorNote_ - (VISIBLE_NOTES - 1);
  }
}

void PianoRollView::moveNote(int sign) {
  const int step = viewData_->phraseCurPos_;
  const PhraseStep &data = currentSteps()[step];

  // there is no note to move in an empty step
  if (data.note == NO_NOTE) {
    return;
  }

  // one step in the scale, with the key and scale of the project, as ENTER + LEFT / RIGHT in the phrase view
  changeNote(step, sign, true);

  // the cursor follows the note
  followNoteOfStep();

  isDirty_ = true;

  // restart the preview (ENTER started it) with the new note
  startAudition(false);
}

void PianoRollView::moveNoteInTime(int delta) {
  const PhraseStep *steps = currentSteps();
  const int from = viewData_->phraseCurPos_;
  const int to = from + delta;

  // nothing to move, or the end of the phrase
  if (to < 0 || to >= STEPS_PER_PHRASE || steps[from].note == NO_NOTE) {
    return;
  }

  // what the note owns moves, as ENTER + EDIT removes it: the commands stay in their step
  PhraseStep &target = editStep(to);
  target.note = steps[from].note;
  target.setInstrument(steps[from].getInstrument());
  target.setVolume(steps[from].getVolume());

  clearNote(editStep(from));

  viewData_->phraseCurPos_ = to;
  isDirty_ = true;
}

void PianoRollView::moveCursorStep(int delta) {
  const int next = std::clamp(viewData_->phraseCurPos_ + delta, 0, STEPS_PER_PHRASE - 1);
  if (next != viewData_->phraseCurPos_) {
    viewData_->phraseCurPos_ = next;
    isDirty_ = true;
  }
}

void PianoRollView::placeCursor() {
  const PhraseStep *steps = currentSteps();
  const int topNote = bottomNote_ + VISIBLE_NOTES - 1;
  const uint8_t note = steps[viewData_->phraseCurPos_].note;

  if (note < NOTE_OFF && note >= bottomNote_ && note <= topNote) {
    cursorNote_ = note;
  } else {
    cursorNote_ = std::clamp(cursorNote_, bottomNote_, topNote);
  }
}

void PianoRollView::DrawView() {
  Clear();

  // the focus is the cell of the cursor
  focusRect_ = panelFocus_ ? panelFieldRect() : GUIRect(GRID_X + viewData_->phraseCurPos_, GRID_Y + cursorRow(), 1, 1);

  DrawTitle("Piano Roll %2.2X", viewData_->currentPhrase_);

  drawMap();
  drawNoteLabels();
  drawStepIndex();
  drawNoteGrid();
  drawScrollIndicators();
  drawStepInfo();
  drawHelpText();

  // as in the phrase view: the legend of the mode (the selection) or the notes that play
  if (!drawContextLegendInfoArea()) {
    infoAreaMode_ = InfoAreaDrawMode::Notes;
    drawNotes();
  }

  // the clear above removed the playback indicator
  lastPlayingStep_ = -1;
  if (Player::GetInstance()->IsRunning()) {
    OnPlayerUpdate(PET_UPDATE);
  }
}

void PianoRollView::AnimationUpdate() {
  ScreenView::AnimationUpdate();

  // do not draw above the modal
  if (HasModalView() || !needsUIUpdate_) {
    return;
  }

  if (infoAreaMode_ == InfoAreaDrawMode::Notes) {
    drawNotes();
  }
  drawPlaybackIndicators();

  needsUIUpdate_ = false;
}

void PianoRollView::drawPlaybackIndicators() {
  Player *player = Player::GetInstance();
  if (!player) {
    return;
  }

  const int y = GRID_Y + VISIBLE_NOTES;

  SetBackgroundColor(Theme::View::bg);

  // clear the last played position
  if (lastPlayingStep_ >= 0) {
    DrawString(GRID_X + lastPlayingStep_, y, " ");
    lastPlayingStep_ = -1;
  }

  // the first channel that plays the phrase of this view gives the position
  int step;
  bool muted;
  if (findPlayingStep(step, muted)) {
    if (muted) {
      SetColor(Theme::Song::Playback::muted);
      DrawString(GRID_X + step, y, char_indicator_positionMuted_s);
    } else {
      SetColor(Theme::Song::Playback::active);
      DrawString(GRID_X + step, y, char_indicator_up_s);
    }
    lastPlayingStep_ = step;
  }

  // the queued chains of the live mode start in the first step, as in the phrase view
  if (const char *indicator = findLiveIndicator()) {
    SetColor(Theme::Song::Playback::live);

    // the playing indicator points up in the roll, the pause and the muted one stay
    if (strcmp(indicator, char_indicator_position_s) == 0) {
      indicator = char_indicator_up_s;
    }
    DrawString(GRID_X, y, indicator);
    if (lastPlayingStep_ < 0) {
      lastPlayingStep_ = 0;
    }
  }
}

void PianoRollView::drawNoteLabels() {
  char label[4];

  const int cursorRow = this->cursorRow();
  const int scale = viewData_->project_->GetScale();
  const int scaleRoot = viewData_->project_->GetScaleRoot();

  // the highest note is in the top row
  for (int row = 0; row < VISIBLE_NOTES; row++) {
    const bool selected = (row == cursorRow);
    const int note = rowNote(row);
    const int inOctave = note % 12;

    // draw piano key
    SetColor(Theme::View::bg);
    SetBackgroundColor(Theme::View::PianoRoll::fg);
    const char *string = selected ? gchar_vertical_piano_selected[inOctave] : gchar_vertical_piano[inOctave];
    DrawString(PIANO_X, GRID_Y + row, string, fGraphic);

    // draw note name
    noteToString(note, label);
    // the notes that are not part of the scale of the project are inactive
    const bool inScale = scaleSteps[scale][(note + 12 - scaleRoot) % 12];
    const Color labelColor = inScale ? Theme::View::index(inOctave == 0) : Theme::View::inactive;
    if (selected && panelFocus_) {
      // the focus is on the right: the selection is only the regular colors inverted
      SetColor(Theme::View::bg);
      SetBackgroundColor(labelColor);
    } else {
      SetColor(selected ? Theme::View::Selection::fg(true) : labelColor);
      SetBackgroundColor(selected ? Theme::View::Selection::bg(true) : Theme::View::bg);
    }
    DrawString(LABEL_X, GRID_Y + row, label);
  }
}

void PianoRollView::drawStepIndex() {
  static_assert(STEPS_PER_PHRASE == 16, "the step index is 0..F");

  const int phraseLength = viewData_->project_->FindVariable(Token::VarPhraseLength)->GetInt();
  const int phraseDiv = viewData_->project_->FindVariable(Token::VarPhraseDivision)->GetInt();

  // as the row numbers of the phrase view: the first step of every division is highlighted, the steps
  // after the end of the phrase are inactive
  SetBackgroundColor(Theme::View::bg);
  for (int step = 0; step < STEPS_PER_PHRASE; step++) {
    const bool selected = isSelected(step);
    SetBackgroundColor(Theme::View::Selection::bg(selected));
    SetColor(selected              ? Theme::View::Selection::fg(true)
             : step < phraseLength ? Theme::View::index(step % phraseDiv == 0)
                                   : Theme::View::inactive);
    DrawChar(GRID_X + step, STEP_INDEX_Y, hexChars[step]);
  }
}

void PianoRollView::drawNoteGrid() {
  const PhraseStep *steps = currentSteps();

  SetBackgroundColor(Theme::View::bg);

  // the empty grid, steps from left to right, the highest note in the top row
  SetColor(Theme::View::inactive);
  SetBackgroundColor(Theme::View::bg);

  for (int row = 0; row < VISIBLE_NOTES; row++) {
    DrawString(GRID_X, GRID_Y + row, gchar_piano_row_empty_row_s, fGraphic);
  }

  // the steps after the end of the phrase are never played
  const int phraseLength = viewData_->project_->FindVariable(Token::VarPhraseLength)->GetInt();

  // draws a cell, the one the cursor is in with the selection colors
  const int cursorStep = viewData_->phraseCurPos_;
  const int cursorRow = this->cursorRow();
  bool cursorDrawn = false;
  auto drawCell = [&](int step, int row, char glyph, Color color) {
    const bool selected = (step == cursorStep && row == cursorRow);
    const bool inactive = step >= phraseLength;
    // the cell of the cursor has the selection colors (readable on its background), all others the color of their
    // glyph (a note off is negative), after the end of the phrase everything is inactive. While the focus is in
    // the fields on the right the cursor cell is only inverted: the color of its glyph as the background
    const Color glyphColor = inactive ? Theme::View::inactive : color;
    if (selected && panelFocus_) {
      SetBackgroundColor(glyphColor);
      SetColor(Theme::View::bg);
    } else if (isSelected(step)) {
      // the selected steps are highlighted over the whole height of the grid, the cursor cell in it inverted
      SetBackgroundColor(selected ? Theme::View::Selection::fg(true) : Theme::View::Selection::bg(true));
      SetColor(selected               ? Theme::View::Selection::bg(true)
               : glyph == GLYPH_EMPTY ? Theme::View::inactive
                                      : Theme::View::Selection::fg(true));
    } else {
      SetBackgroundColor(Theme::View::Selection::bg(selected));
      SetColor(inactive ? Theme::View::inactive : (selected ? Theme::View::Selection::fg(true) : color));
    }
    DrawChar(GRID_X + step, GRID_Y + row, glyph, fGraphic);
    cursorDrawn = cursorDrawn || selected;
  };

  // the selected steps over the whole height of the grid
  for (int step = selectionFirst(); selecting_ && step <= selectionLast(); step++) {
    for (int row = 0; row < VISIBLE_NOTES; row++) {
      drawCell(step, row, GLYPH_EMPTY, Theme::View::inactive);
    }
  }

  // the notes
  int sounding = -1;
  for (int step = 0; step < STEPS_PER_PHRASE; step++) {
    const GridCell cell = noteGridGlyph(steps, step, sounding);
    if (cell.glyph == 0) {
      continue;
    }

    if (cell.pitch == ALL_NOTES) {
      for (int row = 0; row < VISIBLE_NOTES; row++) {
        drawCell(step, row, cell.glyph, cell.color);
      }
      continue;
    }

    const int row = bottomNote_ + (VISIBLE_NOTES - 1) - cell.pitch;
    if (row >= 0 && row < VISIBLE_NOTES) {
      drawCell(step, row, cell.glyph, cell.color);
    }
  }

  // an empty cell the cursor is in
  if (!cursorDrawn) {
    drawCell(cursorStep, cursorRow, GLYPH_EMPTY, Theme::View::inactive);
  }

  SetBackgroundColor(Theme::View::bg);
}

void PianoRollView::drawStepInfo() {
  const int step = viewData_->phraseCurPos_;
  const PhraseStep *steps = currentSteps();
  const PhraseStep &data = steps[step];

  // With the grid in focus the fields show what is in effect at the step: the values that were set before
  // and are still valid are drawn inactive. In the fields themselves only the values of the step are shown.
  const bool sticky = !panelFocus_;

  // the title of the first field is in the row of the step index, every field has a title row and a value row
  const int x = STEP_INFO_X;
  const int y = PANEL_Y;

  SetBackgroundColor(Theme::View::bg);
  SetColor(Theme::View::legend);

  DrawString(x, y, "Note");
  DrawString(x, y + 2, "Inst");
  DrawString(x, y + 4, "Vol");
  DrawString(x, y + 6, "Cmd1Val");
  DrawString(x, y + 8, "Cmd2Val");

  // the values, in the colors and formats of the phrase view
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  const uint8_t instrumentId = getEffectiveInstrumentForStep(step);
  I_Instrument *instrument =
      (instrumentId == NO_INSTRUMENT) ? bank->noneInstrument() : bank->GetInstrument(instrumentId);

  // the note that still sounds if the step has none (until a note off)
  uint8_t note = data.note;
  bool noteSticky = false;
  if (sticky && note == NO_NOTE) {
    for (int i = step - 1; i >= 0; --i) {
      if (steps[i].note != NO_NOTE) {
        if (steps[i].note < NOTE_OFF) {
          note = steps[i].note;
          noteSticky = true;
        }
        break;
      }
    }
  }

  char noteBuffer[4];
  instrument->noteDisplay(note, noteBuffer);
  char buffer[6];

  const int volumeSource =
      valueSource(steps, step, sticky, [](const PhraseStep &s) { return s.getVolume() != NO_VOLUME; });
  const int command1Source = valueSource(steps, step, sticky, [](const PhraseStep &s) {
    return Token::enum_type(s.getCmd1()) != Token::InstrumentCommandNone;
  });
  const int command2Source = valueSource(steps, step, sticky, [](const PhraseStep &s) {
    return Token::enum_type(s.getCmd2()) != Token::InstrumentCommandNone;
  });

  // the color of a value: inactive if it was not set in this step
  auto colorOf = [&](int source, Color color) {
    return (source >= 0 && source != step) ? Theme::View::inactive : color;
  };
  const PhraseStep &volumeData = steps[volumeSource >= 0 ? volumeSource : step];
  const PhraseStep &command1Data = steps[command1Source >= 0 ? command1Source : step];
  const PhraseStep &command2Data = steps[command2Source >= 0 ? command2Source : step];

  // the selected field of the panel has the selection colors, as the cell of the phrase view
  // (a dimmed value keeps its dim color on the selection background too)
  auto setFieldColors = [&](StepField field, Color color, bool dim = false) {
    const bool selected = panelFocus_ && panelField_ == field;
    SetBackgroundColor(Theme::View::Selection::bg(selected));
    SetColor(selected && !dim ? Theme::View::Selection::fg(true) : color);
  };

  // a parameter is edited digit by digit, the digit that is edited is drawn inverted
  auto drawParam = [&](StepField field, int row, uint16_t value, Color color) {
    char text[6];
    wordToHexString(value, text);
    setFieldColors(field, color);
    DrawString(x + 3, row, text);
    if (panelFocus_ && panelField_ == field) {
      const int digit = 3 - cmdEditField_.GetColumn();
      SetBackgroundColor(Theme::View::Selection::fg(true));
      SetColor(Theme::View::Selection::bg(true));
      DrawChar(x + 3 + digit, row, text[digit]);
    }
  };

  setFieldColors(StepField::Note, noteSticky ? Theme::View::inactive : Theme::Phrase::note(true));
  DrawString(x, y + 1, noteBuffer);

  // A step without an instrument plays the last one set before it (the shadow instrument, as the phrase view
  // and the preview have it), it is always drawn inactive, in the fields as well
  const bool shadowInstrument = data.getInstrument() == NO_INSTRUMENT && instrumentId != NO_INSTRUMENT;
  setFieldColors(StepField::Instrument, shadowInstrument ? Theme::View::inactive : Theme::Phrase::instrument(true),
                 shadowInstrument);
  if (instrumentId == NO_INSTRUMENT) {
    DrawString(x, y + 3, "--");
  } else {
    byteToHexString(instrumentId, buffer);
    DrawString(x, y + 3, buffer);
  }

  setFieldColors(StepField::Volume, colorOf(volumeSource, Theme::Phrase::volume(true)));
  DrawChar(x, y + 5, volumeData.getVolume() == NO_VOLUME ? '-' : hexChars[volumeData.getVolume() & 0xF]);

  // a parameter stays with its command
  Token command1 = Token::enum_type(command1Data.getCmd1());
  setFieldColors(StepField::Command1, colorOf(command1Source, Theme::Phrase::command1(true)));
  DrawString(x, y + 7, command1.c_str());
  drawParam(StepField::Param1, y + 7, command1Data.param1, colorOf(command1Source, Theme::Phrase::command1(true)));

  Token command2 = Token::enum_type(command2Data.getCmd2());
  setFieldColors(StepField::Command2, colorOf(command2Source, Theme::Phrase::command2(true)));
  DrawString(x, y + 9, command2.c_str());
  drawParam(StepField::Param2, y + 9, command2Data.param2, colorOf(command2Source, Theme::Phrase::command2(true)));

  SetBackgroundColor(Theme::View::bg);
}

void PianoRollView::drawHelpText() {
  // under the fields, it always shows how to get to the part that does not have the focus
  const int x = STEP_INFO_X;
  const int y = HELP_Y;

  SetBackgroundColor(Theme::View::bg);
  SetColor(Theme::View::help(false));
  if (panelFocus_) {
    DrawString(x, y, char_key_alt_s "+" char_key_left_s " to");
    DrawString(x, y + 1, "Grid");
  } else {
    DrawString(x, y, char_key_alt_s "+" char_key_right_s " to");
    DrawString(x, y + 1, "Fields");
  }
}

void PianoRollView::drawScrollIndicators() {
  const PhraseStep *steps = currentSteps();

  // centered above and below the note labels, above is the row of the step index
  const int x = LABEL_X + 1;
  const int topY = STEP_INDEX_Y;
  const int bottomY = GRID_Y + VISIBLE_NOTES;

  bool notesAbove;
  bool notesBelow;
  notesOutsideRange(steps, bottomNote_, notesAbove, notesBelow);

  SetBackgroundColor(Theme::View::bg);
  SetColor(Theme::View::fg);

  // the arrows show where the view can be scrolled to
  if (bottomNote_ < MAX_BOTTOM_NOTE) {
    DrawChar(x, topY, CHAR(char_indicator_up_s));
  }
  if (bottomNote_ > 0) {
    DrawChar(x, bottomY, CHAR(char_indicator_down_s));
  }

  // and the note symbol next to them that there are notes out of the visible range
  if (notesAbove) {
    DrawChar(x + 1, topY, CHAR(char_symbol_bpm_s));
  }
  if (notesBelow) {
    DrawChar(x + 1, bottomY, CHAR(char_symbol_bpm_s));
  }
}
