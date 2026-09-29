/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "TextInputModal.h"
#include "Application/AppWindow.h"
#include "Foundation/Constants/SpecialCharacters.h"
#include <new>

// Grid geometry, in content relative character cells.
//
// Rows are spaced one blank row apart so the highlight brackets drawn at
// (y +- 1) never land on a neighbouring row's characters. Columns are spaced
// two apart for the same reason horizontally.
#define GRID_COLS 11
#define GRID_ROWS 4
#define GRID_X0 2 // leaves column 1 free for the first column's bracket
#define GRID_Y0 7 // row 0's top bracket sits at y=6, clear of the cursor row
#define CELL_W 2  // characters sit every other column
#define CELL_H 2  // ... and every other row

// Window spans the widest bracket plus a border column either side.
#define WINDOW_W (GRID_X0 + (GRID_COLS - 1) * CELL_W + 3)
#define WINDOW_H 19

// The value field spans the same visual width as the character grid, so its
// background lines up with the columns below it. The editable run is only
// maxLength_ long; the remainder is background padding.
#define FIELD_W ((GRID_COLS - 1) * CELL_W + 1)

// The character repertoire deliberately matches getNext() in
// Application/Utils/stringutils.cpp so every value stays usable as a filename.
// Rows 0-2 are letters and punctuation (letters case swapped by EDIT), row 3
// holds the wide SPACE cell followed by '.' and the remaining digits.
static const char *kUpperRows[3] = {"ABCDEFGHIJK", "LMNOPQRSTUV", "WXYZ_-01234"};
static const char *kLowerRows[3] = {"abcdefghijk", "lmnopqrstuv", "wxyz_-01234"};
static const char *kLastRow = ".56789";

// Row 3 layout: the SPACE cell occupies columns 0..4, then kLastRow's
// characters occupy columns 5..10.
#define ROW_LAST 3
#define SPACE_COLS 5
#define SPACE_LABEL "  SPACE  "

bool TextInputModal::inUse_ = false;
alignas(TextInputModal) static unsigned char TextInputModalStorage[sizeof(TextInputModal)];
void *TextInputModal::storage_ = TextInputModalStorage;

TextInputModal *TextInputModal::Create(View &view, const char *label, const char *initialValue, uint8_t maxLength) {
  if (inUse_) {
    auto *existing = reinterpret_cast<TextInputModal *>(storage_);
    existing->~TextInputModal();
    inUse_ = false;
  }
  inUse_ = true;
  return new (storage_) TextInputModal(view, label, initialValue, maxLength);
}

TextInputModal::TextInputModal(View &view, const char *label, const char *initialValue, uint8_t maxLength)
    : ModalView(view), maxLength_(maxLength) {
  if (maxLength_ > MAX_TEXT_INPUT_LENGTH) {
    maxLength_ = MAX_TEXT_INPUT_LENGTH;
  }

  if (label) {
    label_.assign(label);
    // Labels are inconsistent: some are padded and already carry a trailing
    // ':' ("Project   :" in ProjectView), others are bare ("Name" in
    // InstrumentView). Strip both so the title renders as "<label>" either
    // way.
    while (!label_.empty() && (label_.back() == ' ' || label_.back() == ':')) {
      label_.pop_back();
    }
  }

  if (initialValue) {
    value_.assign(initialValue);
    if (value_.size() > maxLength_) {
      value_.resize(maxLength_);
    }
  }

  // Cursor starts past the existing text so typing appends.
  cursor_ = (uint8_t)value_.size();
}

TextInputModal::~TextInputModal() {
}

void TextInputModal::Destroy() {
  this->~TextInputModal();
  inUse_ = false;
}

uint8_t TextInputModal::columnsInRow(uint8_t row) const {
  // Every row is the full width: on the last row the SPACE cell simply covers
  // the first SPACE_COLS columns, so navigating over them lands on SPACE.
  return GRID_COLS;
}

char TextInputModal::highlightedChar() const {
  if (gridRow_ < ROW_LAST) {
    return (upperCase_ ? kUpperRows : kLowerRows)[gridRow_][gridCol_];
  }
  if (gridCol_ < SPACE_COLS) {
    return ' ';
  }
  return kLastRow[gridCol_ - SPACE_COLS];
}

void TextInputModal::cellGeometry(uint8_t row, uint8_t col, int &x, int &width) const {
  if (row == ROW_LAST && col < SPACE_COLS) {
    // The SPACE cell is drawn as one wide button covering columns 0..4.
    x = GRID_X0;
    width = (int)strlen(SPACE_LABEL);
    return;
  }
  x = GRID_X0 + col * CELL_W;
  width = 1;
}

uint8_t TextInputModal::cursorColumn() const {
  // The cursor sits at the insertion point, clipped to the last editable
  // column so it stays inside the field when the value is full.
  return (cursor_ >= maxLength_) ? (uint8_t)(maxLength_ - 1) : cursor_;
}

void TextInputModal::insertChar(char c) {
  if (cursor_ >= maxLength_) {
    // Field is full and the cursor has clipped to the last column: keep
    // typing overwrites that character instead of doing nothing.
    if (maxLength_ > 0) {
      value_[maxLength_ - 1] = c;
    }
    return;
  }
  if (value_.size() >= maxLength_) {
    // Full, but the cursor is mid-string: overwrite in place rather than
    // silently dropping the keypress.
    value_[cursor_] = c;
    cursor_++;
    return;
  }
  value_.insert(value_.begin() + cursor_, c);
  cursor_++;
}

void TextInputModal::deleteChar() {
  if (cursor_ == 0 || value_.empty()) {
    return;
  }
  cursor_--;
  value_.erase(cursor_, 1);
}

void TextInputModal::moveGrid(int8_t dx, int8_t dy) {
  if (dy != 0) {
    gridRow_ = (uint8_t)((gridRow_ + dy + GRID_ROWS) % GRID_ROWS);
  }

  if (dx != 0) {
    if (gridRow_ == ROW_LAST) {
      // SPACE covers columns 0..SPACE_COLS-1 and behaves as a single cell, so
      // step over the whole run in one move.
      if (dx > 0) {
        if (gridCol_ < SPACE_COLS) {
          gridCol_ = SPACE_COLS;
        } else if (gridCol_ + 1 >= GRID_COLS) {
          gridCol_ = 0;
        } else {
          gridCol_++;
        }
      } else {
        if (gridCol_ <= SPACE_COLS) {
          gridCol_ = (gridCol_ == 0) ? (uint8_t)(GRID_COLS - 1) : 0;
        } else {
          gridCol_--;
        }
      }
    } else {
      gridCol_ = (uint8_t)((gridCol_ + dx + GRID_COLS) % GRID_COLS);
    }
  }

  if (gridCol_ >= GRID_COLS) {
    gridCol_ = (uint8_t)(GRID_COLS - 1);
  }
}

void TextInputModal::drawHighlight(int x, int y, int width) {
  SetColor(Theme::Dialog::Selectable::bg(true));
  SetBackgroundColor(Theme::Dialog::bg);

  // Bracket the focused character with a half wide border
  DrawChar(x - 1, y - 1, CHAR(char_filledHalfBorder_topLeft_s), fRegular, true);
  DrawChar(x - 1, y, CHAR(char_block_left_s));
  DrawChar(x - 1, y + 1, CHAR(char_filledHalfBorder_bottomLeft_s), fRegular, true);

  DrawChar(x + width, y - 1, CHAR(char_filledHalfBorder_topRight_s), fRegular, true);
  DrawChar(x + width, y, CHAR(char_block_right_s));
  DrawChar(x + width, y + 1, CHAR(char_filledHalfBorder_bottomRight_s), fRegular, true);

  for (int dx = 0; dx < width; dx++) {
    DrawChar(x + dx, y - 1, CHAR(char_block_bottom_s));
    DrawChar(x + dx, y + 1, CHAR(char_block_top_s));
  }
}

void TextInputModal::DrawView() {
  // DrawWindow paints the frame, the title bar and the body fill. Position the
  // window via left_/top_ and draw at (0,0): ModalView's DrawChar/DrawString
  // overrides add the offset, so every coordinate here is window relative.
  left_ = (SCREEN_WIDTH - WINDOW_W) / 2;
  top_ = (SCREEN_HEIGHT - WINDOW_H) / 2;
  DrawWindow(0, 0, WINDOW_W, WINDOW_H, label_.c_str());

  // --- current value ------------------------------------------------------
  // Same bracketed treatment as the highlighted grid cell, drawn permanently.
  const int fieldX = GRID_X0;
  const int fieldY = 4;
  drawHighlight(fieldX, fieldY, FIELD_W);

  // Background runs the full field width even though only the first
  // maxLength_ cells are editable.
  SetColor(Theme::Dialog::Selectable::fg(true));
  SetBackgroundColor(Theme::Dialog::Selectable::bg(true));
  for (int i = 0; i < FIELD_W; i++) {
    char c = (i < (int)value_.size()) ? value_[i] : ' ';
    DrawChar(fieldX + i, fieldY, c);
  }

  // --- character grid -----------------------------------------------------
  const char **letterRows = upperCase_ ? kUpperRows : kLowerRows;
  for (uint8_t r = 0; r < ROW_LAST; r++) {
    for (uint8_t c = 0; c < GRID_COLS; c++) {
      bool active = (r == gridRow_ && c == gridCol_);
      SetColor(Theme::Dialog::Selectable::fg(active));
      SetBackgroundColor(Theme::Dialog::Selectable::bg(active));
      DrawChar(GRID_X0 + c * CELL_W, GRID_Y0 + r * CELL_H, letterRows[r][c]);
    }
  }

  const int lastY = GRID_Y0 + ROW_LAST * CELL_H;
  bool spaceActive = (gridRow_ == ROW_LAST && gridCol_ < SPACE_COLS);
  SetColor(Theme::Dialog::Selectable::fg(spaceActive));
  SetBackgroundColor(Theme::Dialog::Selectable::bg(spaceActive));
  DrawString(GRID_X0, lastY, SPACE_LABEL);

  for (uint8_t i = 0; i < strlen(kLastRow); i++) {
    uint8_t c = (uint8_t)(SPACE_COLS + i);
    bool active = (gridRow_ == ROW_LAST && c == gridCol_);
    SetColor(Theme::Dialog::Selectable::fg(active));
    SetBackgroundColor(Theme::Dialog::Selectable::bg(active));
    DrawChar(GRID_X0 + c * CELL_W, lastY, kLastRow[i]);
  }

  // --- highlight ----------------------------------------------------------
  int hx, hw;
  cellGeometry(gridRow_, gridCol_, hx, hw);
  int hy = GRID_Y0 + gridRow_ * CELL_H;
  drawHighlight(hx, hy, hw);
  focusRect_ = GUIRect(hx + left_ - 1, hy + top_ - 1, hw + 2, 3);

  // --- legend -------------------------------------------------------------
  SetColor(Theme::Dialog::inactive);
  SetBackgroundColor(Theme::Dialog::bg);
  int ly = GRID_Y0 + (GRID_ROWS - 1) * CELL_H + 2;
  DrawString(GRID_X0, ly, char_key_play_s " OK");
  DrawString(GRID_X0, ly + 1, char_key_enter_s " Use");
  DrawString(GRID_X0, ly + 2, upperCase_ ? char_key_edit_s " lower" : char_key_edit_s " UPPER");
  DrawString(GRID_X0 + 9, ly + 1, char_key_nav_s "+" char_key_left_s " Abort");
  DrawString(GRID_X0 + 9, ly + 2, char_key_alt_s "+" char_key_enter_s " Backspace");
}

void TextInputModal::ProcessButtonMask(uint16_t mask, bool pressed) {
  if (!pressed) {
    return;
  }

  // Chords first: a combo whose modifier is released a frame early would
  // otherwise fall through to the bare key case below.
  if ((mask & BM_ENTER) && (mask & BM_ALT)) {
    deleteChar();
    isDirty_ = true;
    return;
  }

  if ((mask & BM_NAV) && (mask & BM_LEFT)) {
    EndModal(TIM_CANCEL);
    return;
  }

  if (mask & BM_PLAY) {
    EndModal(TIM_ACCEPT);
    return;
  }

  if (mask == BM_ENTER) {
    insertChar(highlightedChar());
    isDirty_ = true;
    return;
  }

  if (mask == BM_EDIT) {
    upperCase_ = !upperCase_;
    isDirty_ = true;
    return;
  }

  if (mask == BM_LEFT) {
    moveGrid(-1, 0);
    isDirty_ = true;
  } else if (mask == BM_RIGHT) {
    moveGrid(+1, 0);
    isDirty_ = true;
  } else if (mask == BM_UP) {
    moveGrid(0, -1);
    isDirty_ = true;
  } else if (mask == BM_DOWN) {
    moveGrid(0, +1);
    isDirty_ = true;
  }
}

void TextInputModal::AnimationUpdate() {
  // Text cursor sits on its own row below the field. It clips to the last
  // editable column: once the value is full, typing overwrites there rather
  // than running the cursor off the end.
  bool visible = (AppWindow::GetInstance()->GetAnimationFrameCounter() >> 4) & 1;
  char character = visible ? CHAR(char_block_top_s) : CHAR(char_upper_cursor_s);

  SetBackgroundColor(Theme::Dialog::bg);
  SetColor(Theme::Dialog::Selectable::bg(true));
  DrawChar(GRID_X0 + cursorColumn(), 5, character);
}