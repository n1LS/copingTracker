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

#include "FieldView.h"
#include "System/Console/Trace.h"
#include "UIIntVarField.h"

FieldView::FieldView(GUIWindow &w, ViewData *data) : ScreenView(w, data) {
  focus_ = 0;
}

void FieldView::SetFocus(UIField *field) {

  if (focus_) {
    focus_->ClearFocus();
  }
  focus_ = field;

  //  Empty field view, we don't have anything to do

  if (focus_ == 0)
    return;

  focus_->SetFocus();
  isDirty_ = true;
}

void FieldView::ClearFocus() {
  if (focus_) {
    focus_->ClearFocus();
  };
  focus_ = nullptr;
}

UIField *FieldView::GetFocus() {
  return focus_;
}

void FieldView::Redraw() {

  if (focus_ == 0) {
    SetFocus(*fieldList_.begin());
  }

  auto it = fieldList_.begin();
  for (size_t i = 0; i < fieldList_.size(); i++) {
    (*it)->Draw(w_);
    it++;
  };
}

void FieldView::ProcessButtonMask(uint16_t mask, bool pressed) {

  if (focus_ == 0) {
    focus_ = *fieldList_.begin();
    //  Empty field view, we don't have anything to do
    if (focus_ == 0)
      return;
    focus_->SetFocus();
  }

  if (mask & BM_ENTER) { // ENTER or ENTER+ARROW is sent to the field
    if (mask & BM_DOWN) {
      focus_->ProcessArrow(BM_DOWN);
      isDirty_ = true;
    }
    if (mask & BM_UP) {
      focus_->ProcessArrow(BM_UP);
      isDirty_ = true;
    }

    if (mask & BM_LEFT) {
      focus_->ProcessArrow(BM_LEFT);
      isDirty_ = true;
    }

    if (mask & BM_RIGHT) {
      focus_->ProcessArrow(BM_RIGHT);
      isDirty_ = true;
    }

    if (mask & BM_EDIT) {
      focus_->ProcessClear();
      isDirty_ = true;
    }

    if (mask == BM_ENTER) {
      focus_->OnClick();
    };

  } else {
    if (mask & BM_EDIT) { // EDIT or EDIT+ARROW is sent to the field

      if (mask == BM_EDIT) {
        focus_->OnEditClick();
        isDirty_ = true;
      };

      if (mask & BM_DOWN) {
        focus_->ProcessEditArrow(BM_DOWN);
        isDirty_ = true;
      }
      if (mask & BM_UP) {
        focus_->ProcessEditArrow(BM_UP);
        isDirty_ = true;
      }

      if (mask & BM_LEFT) {
        focus_->ProcessEditArrow(BM_LEFT);
        isDirty_ = true;
      }

      if (mask & BM_RIGHT) {
        focus_->ProcessEditArrow(BM_RIGHT);
        isDirty_ = true;
      }

    } else { // Nor ENTER or EDIT is pressed

      if (!(mask & (BM_ENTER | BM_EDIT | BM_ALT | BM_NAV | BM_PLAY))) {

        if (mask & BM_DOWN) {
          UIField *next = findAdjacentField(true, +1);
          SetFocus(next);
        }

        if (mask & BM_UP) {
          UIField *prev = findAdjacentField(true, -1);
          SetFocus(prev);
        }

        if (mask & BM_RIGHT) {
          UIField *next = findAdjacentField(false, +1);
          SetFocus(next);
        }

        if (mask & BM_LEFT) {
          UIField *prev = findAdjacentField(false, -1);
          SetFocus(prev);
        }
      }
    }
  }
}

// Finds the next focusable field adjacent to the current focus.
//
// vertical=true  → UP/DOWN navigation (searches across all rows)
// vertical=false → LEFT/RIGHT navigation (stays on the focus row)
// direction=+1   → DOWN or RIGHT; direction=-1 → UP or LEFT
//
// Candidate: closest field strictly in the given direction; vertical
// tie-breaks on same row by closest X to current focus.
// Wrap: topmost/leftmost (direction=+1) or bottommost/rightmost (direction=-1)
// field, used when no candidate exists (at the edge).
UIField *FieldView::findAdjacentField(bool vertical, int8_t direction) {
  UIField *candidate = nullptr;
  UIField *wrap = nullptr;

  int32_t focusY = focus_->GetPosition().y_;
  int32_t focusX = focus_->GetPosition().x_;

  for (auto it = fieldList_.begin(); it != fieldList_.end(); ++it) {
    UIField *field = *it;
    if (field->IsStatic())
      continue;

    int32_t fy = field->GetPosition().y_;
    int32_t fx = field->GetPosition().x_;

    // Horizontal navigation is constrained to the focus row
    if (!vertical && fy != focusY)
      continue;

    int32_t primary = vertical ? fy : fx;
    int32_t focusPrimary = vertical ? focusY : focusX;

    // Update wrap: keeps the extreme field in the wrap direction
    if (!wrap) {
      wrap = field;
    } else {
      int32_t wrapPrimary = vertical ? wrap->GetPosition().y_ : wrap->GetPosition().x_;
      if (direction > 0 ? primary < wrapPrimary : primary > wrapPrimary)
        wrap = field;
    }

    // Only consider fields strictly beyond the focus in the given direction
    if (direction > 0 ? primary <= focusPrimary : primary >= focusPrimary)
      continue;

    if (!candidate) {
      candidate = field;
    } else {
      int32_t candPrimary = vertical ? candidate->GetPosition().y_ : candidate->GetPosition().x_;
      bool closerPrimary = direction > 0 ? primary < candPrimary : primary > candPrimary;
      if (closerPrimary) {
        candidate = field;
      } else if (primary == candPrimary && vertical) {
        // Tie-break on same row: prefer field with X closest to focus
        if (abs(fx - focusX) < abs(candidate->GetPosition().x_ - focusX))
          candidate = field;
      }
    }
  }

  return candidate ? candidate : wrap;
}

int FieldView::GetFocusIndex() {

  int focusIndex = 0;
  auto it = fieldList_.begin();
  for (size_t i = 0; i < fieldList_.size(); i++) {
    if (*it == focus_) {
      break;
    };
    focusIndex++;
    it++;
  };
  return focusIndex;
}
