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
// Rows are spaced one blank row apart so the highlight corners drawn at
// (x +- 1, y +- 1) never land on a neighbouring row's characters.
#define GRID_COLS 13
#define GRID_ROWS 4
#define GRID_X0 0 // leaves column 0 free for the first column's corner glyphs
#define GRID_Y0 3 // row 0's highlight corners sit at y=2, clear of the cursor at y=1
#define CELL_W 2  // characters sit every other column
#define CELL_H 2  // ... and every other row

#define CONTENT_W (GRID_X0 + (GRID_COLS - 1) * CELL_W + 1) // 27
#define CONTENT_H 14

// Rows 0 and 1 hold the letters (case swapped by EDIT), row 2 the digits and
// the filename safe punctuation, row 3 the single space cell. The character
// repertoire deliberately matches getNext() in Application/Utils/stringutils.cpp
// so that every value stays usable as a filename.
static const char *kUpperRows[2] = {"ABCDEFGHIJKLM", "NOPQRSTUVWXYZ"};
static const char *kLowerRows[2] = {"abcdefghijklm", "nopqrstuvwxyz"};
static const char *kDigitRow = "0123456789-._";

#define ROW_DIGITS 2
#define ROW_SPACE 3

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
  switch (row) {
    case 0:
    case 1:
      return GRID_COLS;
    case ROW_DIGITS:
      return (uint8_t)strlen(kDigitRow);
    case ROW_SPACE:
    default:
      return 1;
  }
}

char TextInputModal::highlightedChar() const {
  switch (gridRow_) {
    case 0:
    case 1:
      return (upperCase_ ? kUpperRows : kLowerRows)[gridRow_][gridCol_];
    case ROW_DIGITS:
      return kDigitRow[gridCol_];
    case ROW_SPACE:
    default:
      return ' ';
  }
}

void TextInputModal::insertChar(char c) {
  if (value_.size() >= maxLength_) {
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
    uint8_t cols = columnsInRow(gridRow_);
    gridCol_ = (uint8_t)((gridCol_ + dx + cols) % cols);
  }
  // Moving onto a shorter row (digits, space) can leave the column past the
  // end, so clamp after any move.
  uint8_t cols = columnsInRow(gridRow_);
  if (gridCol_ >= cols) {
    gridCol_ = (uint8_t)(cols - 1);
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
  SetWindow(CONTENT_W, CONTENT_H);
  SetBackgroundColor(Theme::Dialog::bg);

  // --- title and current value -------------------------------------------
  int x = GRID_X0;
  if (!label_.empty()) {
    SetColor(Theme::Dialog::fg);
    DrawString(x, 0, label_.c_str(), fBold);
    x += (int)label_.size() + 1;
  }

  SetBackgroundColor(Theme::Dialog::bg);
  SetColor(Theme::Dialog::Selectable::fg(true));
  DrawChar(x - 1, 0, CHAR(char_button_border_left_s));
  DrawChar(x + maxLength_, 0, CHAR(char_button_border_right_s));

  SetColor(Theme::Dialog::Selectable::bg(true));
  SetBackgroundColor(Theme::Dialog::Selectable::fg(true));
  for (uint8_t i = 0; i < maxLength_; i++) {
    char c = (i < value_.size()) ? value_[i] : ' ';
    DrawChar(x + i, 0, c);
  }

  // Text cursor: a block under the insertion point.
  SetBackgroundColor(Theme::Dialog::bg);
  SetColor(Theme::Dialog::Selectable::bg(true));
  DrawChar(x + cursor_, 1, CHAR(char_upper_cursor_s), fRegular, true);

  // --- character grid ------------------------------------------------------
  SetBackgroundColor(Theme::Dialog::bg);
  SetColor(Theme::Dialog::fg);

  const char **letterRows = upperCase_ ? kUpperRows : kLowerRows;
  for (uint8_t r = 0; r < 2; r++) {
    for (uint8_t c = 0; c < GRID_COLS; c++) {
      bool active = (r == gridRow_ && c == gridCol_);
      SetColor(Theme::Dialog::Selectable::fg(active));
      SetBackgroundColor(Theme::Dialog::Selectable::bg(active));
      DrawChar(GRID_X0 + c * CELL_W, GRID_Y0 + r * CELL_H, letterRows[r][c]);
    }
  }

  for (uint8_t c = 0; c < strlen(kDigitRow); c++) {
    bool active = (gridRow_ == 2 && c == gridCol_);
    SetColor(Theme::Dialog::Selectable::fg(active));
    SetBackgroundColor(Theme::Dialog::Selectable::bg(active));
    DrawChar(GRID_X0 + c * CELL_W, GRID_Y0 + ROW_DIGITS * CELL_H, kDigitRow[c]);
  }

  // The space cell needs a visible affordance, it is otherwise blank.

  bool active = gridRow_ == 3;
  SetColor(Theme::Dialog::Selectable::fg(active));
  SetBackgroundColor(Theme::Dialog::Selectable::bg(active));
  DrawString(GRID_X0 + CONTENT_W / 2 - 3, GRID_Y0 + ROW_SPACE * CELL_H, "Space");

  // --- highlight -----------------------------------------------------------
  int hx = GRID_X0 + gridCol_ * CELL_W;
  int hy = GRID_Y0 + gridRow_ * CELL_H;
  if (active) {
    hx += CONTENT_W / 2 - 3;
  }
  int selWidth = (gridRow_ == 3) ? 5 : 1;
  drawHighlight(hx, hy, selWidth);
  focusRect_ = GUIRect(hx + left_ - 1, hy + top_ - 1, selWidth + 2, 3);

  // --- legend --------------------------------------------------------------
  SetColor(Theme::Dialog::inactive);
  SetBackgroundColor(Theme::Dialog::bg);
  int ly = GRID_Y0 + (GRID_ROWS - 1) * CELL_H + 2;
  DrawString(GRID_X0, ly, char_key_play_s " OK");
  DrawString(GRID_X0, ly + 1, char_key_edit_s " lower     " char_key_nav_s "+" char_key_left_s " Abort");
  if (!upperCase_) {
    DrawString(GRID_X0 + 2, ly + 1, "UPPER");
  }
  DrawString(GRID_X0, ly + 2, char_key_enter_s " Use       " char_key_alt_s "+" char_key_enter_s " Backspc");
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
