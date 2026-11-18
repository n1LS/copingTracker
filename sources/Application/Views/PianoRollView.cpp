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
#include "Application/Model/Song.h"
#include "Application/Player/Player.h"
#include "Application/Utils/char.h"
#include "Foundation/Constants/GraphicCharacters.h"
#include "Foundation/Constants/SpecialCharacters.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>

// one note per row, in the rows of the song grid (3..18)
static constexpr int VISIBLE_NOTES = 16;
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

  // the grid has the focus when the view opens
  panelFocus_ = false;

  centerOnNotes();
  placeCursor();
}

void PianoRollView::centerOnNotes() {
  bottomNote_ = bestBottomNote(viewData_->song_->phrase_.steps_[viewData_->currentPhrase_]);
}

void PianoRollView::onPhraseSwitched() {
  // another phrase has its notes somewhere else
  centerOnNotes();
  placeCursor();
  isDirty_ = true;
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
  // stop the preview when ENTER is released, as in the phrase view
  if (!(mask & BM_ENTER)) {
    stopAudition();
  }
}

void PianoRollView::focusPanel(bool panel) {
  if (panelFocus_ == panel) {
    return;
  }
  panelFocus_ = panel;
  isDirty_ = true;
}

// the row of a field in the panel: note, instrument, volume, command 1, command 2
static int panelRow(StepField field) {
  switch (field) {
    case StepField::Note:
      return 0;
    case StepField::Instrument:
      return 1;
    case StepField::Volume:
      return 2;
    case StepField::Command1:
    case StepField::Param1:
      return 3;
    case StepField::Command2:
    case StepField::Param2:
      return 4;
  }
  return 0;
}

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

void PianoRollView::movePanelField(uint16_t direction) {
  int row = panelRow(panelField_);
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
  const int y = GetAnchor().y_ - 1;
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
  if (IsChord(newButton, mask, BM_LEFT) || IsChord(newButton, mask, BM_RIGHT) || IsChord(newButton, mask, BM_UP) ||
      IsChord(newButton, mask, BM_DOWN)) {
    movePanelField(newButton);
  } else if (IsChord(newButton, mask, BM_ENTER)) {
    // as in the phrase view: an empty field gets the last value, the values that can be heard start the preview
    const int step = viewData_->phraseCurPos_;
    pasteLastValue(step, panelField_);
    followNoteOfStep();
    isDirty_ = true;
    if (panelField_ == StepField::Note || panelField_ == StepField::Instrument || panelField_ == StepField::Volume) {
      stopAudition();
      startAudition(true);
    }
  } else if (IsChord(newButton, mask, BM_LEFT, BM_ENTER) || IsChord(newButton, mask, BM_RIGHT, BM_ENTER) ||
             IsChord(newButton, mask, BM_UP, BM_ENTER) || IsChord(newButton, mask, BM_DOWN, BM_ENTER)) {
    editPanelField(toDirection(newButton));
  } else if (IsChord(newButton, mask, BM_EDIT, BM_ENTER)) {
    clearPanelField();
  }
}

void PianoRollView::editPanelField(ViewUpdateDirection direction) {
  const int step = viewData_->phraseCurPos_;
  PhraseStep &data = viewData_->song_->phrase_.steps_[viewData_->currentPhrase_][step];

  switch (panelField_) {
    case StepField::Note:
      // there is no note to change in an empty step
      if (data.note == NO_NOTE) {
        return;
      }
      // as in the phrase view: LEFT / RIGHT one step in the scale, UP / DOWN an octave
      changeNote(step, (direction == VUD_LEFT || direction == VUD_DOWN) ? -1 : 1,
                 direction == VUD_LEFT || direction == VUD_RIGHT);
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
  if (panelField_ == StepField::Note || panelField_ == StepField::Instrument || panelField_ == StepField::Volume) {
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
  const uint8_t note = viewData_->song_->phrase_.steps_[viewData_->currentPhrase_][viewData_->phraseCurPos_].note;
  if (note < NOTE_OFF) {
    cursorNote_ = std::clamp<int>(note, 0, HIGHEST_NOTE);
    scrollToCursor();
  }
}

void PianoRollView::placeNote() {
  PhraseStep &step = viewData_->song_->phrase_.steps_[viewData_->currentPhrase_][viewData_->phraseCurPos_];

  // as in the phrase view: a step without a note gets the last instrument,
  // and the note becomes the last note and instrument to paste
  const bool hadNote = step.note < NOTE_OFF;
  step.note = static_cast<uint8_t>(cursorNote_);
  if (!hadNote) {
    step.instrument = static_cast<uint8_t>(viewData_->phraseLastInstrument_);
  }
  viewData_->phraseLastNote_ = step.note;
  viewData_->phraseLastInstrument_ = step.instrument;

  isDirty_ = true;

  // preview the note as long as ENTER is held, ButtonUp stops it
  stopAudition();
  startAudition(true);
}

void PianoRollView::removeNote() {
  PhraseStep &step = viewData_->song_->phrase_.steps_[viewData_->currentPhrase_][viewData_->phraseCurPos_];

  if (step.note == NO_NOTE) {
    // cutting an empty note slot adds a note off, as in the phrase view
    step.note = NOTE_OFF;
  } else {
    // ... and cutting a note takes its instrument and volume with it
    step.note = NO_NOTE;
    step.instrument = NO_INSTRUMENT;
    step.volume = NO_VOLUME;
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
  PhraseStep &data = viewData_->song_->phrase_.steps_[viewData_->currentPhrase_][step];

  // there is no note to move in an empty step
  if (data.note == NO_NOTE) {
    return;
  }

  // one step in the scale, with the key and scale of the project, as ENTER + LEFT / RIGHT in the phrase view
  changeNote(step, sign, true);

  // the cursor follows the note
  if (data.note < NOTE_OFF) {
    cursorNote_ = std::clamp<int>(data.note, 0, HIGHEST_NOTE);
    scrollToCursor();
  }

  isDirty_ = true;

  // restart the preview (ENTER started it) with the new note
  startAudition(false);
}

void PianoRollView::moveNoteInTime(int delta) {
  PhraseStep *steps = viewData_->song_->phrase_.steps_[viewData_->currentPhrase_];
  const int from = viewData_->phraseCurPos_;
  const int to = from + delta;

  // nothing to move, or the end of the phrase
  if (to < 0 || to >= STEPS_PER_PHRASE || steps[from].note == NO_NOTE) {
    return;
  }

  // what the note owns moves, as ENTER + EDIT removes it: the commands stay in their step
  steps[to].note = steps[from].note;
  steps[to].instrument = steps[from].instrument;
  steps[to].volume = steps[from].volume;

  steps[from].note = NO_NOTE;
  steps[from].instrument = NO_INSTRUMENT;
  steps[from].volume = NO_VOLUME;

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
  const PhraseStep *steps = viewData_->song_->phrase_.steps_[viewData_->currentPhrase_];
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
  const GUIPoint anchor = GetAnchor();
  focusRect_ =
      panelFocus_ ? panelFieldRect() : GUIRect(GRID_X + viewData_->phraseCurPos_, anchor.y_ + cursorRow(), 1, 1);

  DrawTitle("Piano Roll %2.2X", viewData_->currentPhrase_);

  drawMap();
  drawNoteLabels();
  drawStepIndex();
  drawNoteGrid();
  drawScrollIndicators();
  drawStepInfo();
  drawHelpText();

  // the notes that play, as in the phrase view (the roll has no legends yet)
  infoAreaMode_ = InfoAreaDrawMode::Notes;
  drawNotes();

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

  const GUIPoint anchor = GetAnchor();
  const int y = anchor.y_ + VISIBLE_NOTES;

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
  const GUIPoint anchor = GetAnchor();
  char label[4];

  const int cursorRow = this->cursorRow();

  // the highest note is in the top row
  for (int row = 0; row < VISIBLE_NOTES; row++) {
    const bool selected = (row == cursorRow);
    const int note = rowNote(row);
    const int inOctave = note % 12;

    // draw piano key
    SetColor(Theme::View::bg);
    SetBackgroundColor(Theme::View::PianoRoll::fg);
    const char *string = selected ? gchar_vertical_piano_selected[inOctave] : gchar_vertical_piano[inOctave];
    DrawString(PIANO_X, anchor.y_ + row, string, fGraphic);

    // draw note name
    noteToString(note, label);
    const Color labelColor = Theme::View::index(inOctave == 0);
    if (selected && panelFocus_) {
      // the focus is on the right: the selection is only the regular colors inverted
      SetColor(Theme::View::bg);
      SetBackgroundColor(labelColor);
    } else {
      SetColor(selected ? Theme::View::Selection::fg(true) : labelColor);
      SetBackgroundColor(selected ? Theme::View::Selection::bg(true) : Theme::View::bg);
    }
    DrawString(LABEL_X, anchor.y_ + row, label);
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
    SetColor(step < phraseLength ? Theme::View::index(step % phraseDiv == 0) : Theme::View::inactive);
    DrawChar(GRID_X + step, 2, hexChars[step]);
  }
}

void PianoRollView::drawNoteGrid() {
  const PhraseStep *steps = viewData_->song_->phrase_.steps_[viewData_->currentPhrase_];

  SetBackgroundColor(Theme::View::bg);

  // the empty grid, steps from left to right, the highest note in the top row
  SetColor(Theme::View::inactive);
  SetBackgroundColor(Theme::View::bg);

  for (int row = 0; row < VISIBLE_NOTES; row++) {
    DrawString(GRID_X, 3 + row, gchar_piano_row_empty_row_s, fGraphic);
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
    } else {
      SetBackgroundColor(Theme::View::Selection::bg(selected));
      SetColor(inactive ? Theme::View::inactive : (selected ? Theme::View::Selection::fg(true) : color));
    }
    DrawChar(GRID_X + step, 3 + row, glyph, fGraphic);
    cursorDrawn = cursorDrawn || selected;
  };

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
  const GUIPoint anchor = GetAnchor();
  const int step = viewData_->phraseCurPos_;
  const PhraseStep &data = viewData_->song_->phrase_.steps_[viewData_->currentPhrase_][step];

  // the title of the first field is in the row of the step index, every field has a title row and a value row
  const int x = STEP_INFO_X;
  const int y = anchor.y_ - 1;

  SetBackgroundColor(Theme::View::bg);

  SetColor(Theme::View::inactive);
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

  char noteBuffer[4];
  instrument->noteDisplay(data.note, noteBuffer);
  char buffer[6];

  // the selected field of the panel has the selection colors, as the cell of the phrase view
  auto setFieldColors = [&](StepField field, Color color) {
    const bool selected = panelFocus_ && panelField_ == field;
    SetBackgroundColor(Theme::View::Selection::bg(selected));
    SetColor(selected ? Theme::View::Selection::fg(true) : color);
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

  setFieldColors(StepField::Note, Theme::Phrase::note(true));
  DrawString(x, y + 1, noteBuffer);

  setFieldColors(StepField::Instrument, Theme::Phrase::instrument(true));
  if (data.instrument == NO_INSTRUMENT) {
    DrawString(x, y + 3, "--");
  } else {
    byteToHexString(data.instrument, buffer);
    DrawString(x, y + 3, buffer);
  }

  setFieldColors(StepField::Volume, Theme::Phrase::volume(true));
  DrawChar(x, y + 5, data.volume == NO_VOLUME ? '-' : hexChars[data.volume & 0xF]);

  Token command1 = Token::enum_type(data.cmd1);
  setFieldColors(StepField::Command1, Theme::Phrase::command1(true));
  DrawString(x, y + 7, command1.c_str());
  drawParam(StepField::Param1, y + 7, data.param1, Theme::Phrase::command1(true));

  Token command2 = Token::enum_type(data.cmd2);
  setFieldColors(StepField::Command2, Theme::Phrase::command2(true));
  DrawString(x, y + 9, command2.c_str());
  drawParam(StepField::Param2, y + 9, data.param2, Theme::Phrase::command2(true));

  SetBackgroundColor(Theme::View::bg);
}

void PianoRollView::drawHelpText() {
  // under the fields, it always shows how to get to the part that does not have the focus
  const int x = STEP_INFO_X;
  const int y = GetAnchor().y_ - 1 + 11;

  SetBackgroundColor(Theme::View::bg);
  SetColor(Theme::View::help(false));
  if (panelFocus_) {
    DrawString(x, y, char_key_alt_s "+" char_key_left_s);
    DrawString(x, y + 1, "Grid");
  } else {
    DrawString(x, y, char_key_alt_s "+" char_key_right_s);
    DrawString(x, y + 1, "Fields");
  }
}

void PianoRollView::drawScrollIndicators() {
  const GUIPoint anchor = GetAnchor();
  const PhraseStep *steps = viewData_->song_->phrase_.steps_[viewData_->currentPhrase_];

  // centered above and below the note labels, above is the row of the step index
  const int x = LABEL_X + 1;
  const int topY = anchor.y_ - 1;
  const int bottomY = anchor.y_ + VISIBLE_NOTES;

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
