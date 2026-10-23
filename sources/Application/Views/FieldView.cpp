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
#include "Application/Views/ModalDialogs/TextInputModal.h"
#include "ModalView.h"
#include "System/Console/Trace.h"
#include "UIIntVarField.h"

FieldView::FieldView(GUIWindow &w, ViewData *data) : ScreenView(w, data) {
  focus_ = 0;
}

// Applies the on screen keyboard's result back onto the field that opened it.
void FieldView::TextEditorCallback(View &v, ModalView &modal) {
  auto &fieldView = static_cast<FieldView &>(v);
  UIField *field = fieldView.editingField_;
  fieldView.editingField_ = nullptr;

  if (!field || modal.GetReturnCode() != TextInputModal::TIM_ACCEPT) {
    return;
  }

  auto &textModal = static_cast<TextInputModal &>(modal);
  field->ApplyEditedValue(textModal.GetValue().c_str());
  fieldView.isDirty_ = true;
}

void FieldView::openTextEditor(UIField *field) {
  editingField_ = field;
  TextInputModal *modal =
      TextInputModal::Create(*this, field->GetEditorLabel(), field->GetEditorValue(), field->GetEditorMaxLength());
  DoModal(modal, ModalViewCallback::create<&FieldView::TextEditorCallback>());
}

void FieldView::UpdateFocusRect() {
  if (!focus_) {
    return;
  }

  GUIPoint pos = focus_->GetPosition();
  int w = focus_->GetFocusWidth();
  int dx = focus_->GetFocusOffset();
  focusRect_ = GUIRect(pos.x_ + dx, pos.y_, w, 1);
}

void FieldView::SetFocus(UIField *field) {
  if (focus_) {
    focus_->ClearFocus();
  }
  focus_ = field;

  //  Empty field view, we don't have anything to do

  if (focus_ == 0)
    return;

  UpdateFocusRect();

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

  if (focus_ == nullptr) {
    SetFocus(*fieldList_.begin());
  }

  auto it = fieldList_.begin();
  for (size_t i = 0; i < fieldList_.size(); i++) {
    // draw the focused item last to overlap all others
    if (focus_ != *it) {
      (*it)->Draw(w_);
    }
    it++;
  }

  // A field only knows its focus width once it has been drawn (the value text
  // length can change with the value, eg. "9%" -> "10%"). Refresh the cached
  // rect here so subclasses reading focusRect_ directly see the new width.
  if (focus_) {
    // draw the focused item last to overlap all others
    focus_->Draw(w_);
    UpdateFocusRect();
  }
}

void FieldView::ensureFocus() {
  if (focus_ == nullptr) {
    focus_ = *fieldList_.begin();
    if (focus_ != nullptr) {
      focus_->SetFocus();
    }
  }
}

void FieldView::ButtonUp(uint16_t button, uint16_t mask) {
  ensureFocus();
  //  Empty field view, we don't have anything to do
  if (focus_ == nullptr)
    return;

  // Button release: fire click/edit-click actions on release
  if (button == BM_ENTER) {
    focus_->SetPressed(false);
    if (focus_->WantsTextEditor()) {
      // Text fields open the on screen keyboard instead of firing OnClick().
      // Done on release rather than press so the ENTER release event does
      // not leak into the freshly opened modal and type a character.
      openTextEditor(focus_);
    } else {
      focus_->OnClick();
    }
    isDirty_ = true;
  } else if (button == BM_EDIT) {
    focus_->OnEditClick();
    isDirty_ = true;
  }
}

void FieldView::ButtonDown(uint16_t newButton, uint16_t mask) {
  ensureFocus();
  if (focus_ == nullptr)
    return;

  const bool isDirection = (newButton & BM_DIRECTIONAL) != 0;

  if (IsChord(newButton, mask, BM_ENTER)) {
    focus_->SetPressed(true);
    isDirty_ = true;
  } else if (isDirection && mask == BM_ENTER) { // ENTER+ARROW is sent to the field
    focus_->ProcessArrow(newButton);
    isDirty_ = true;
  } else if (IsPair(newButton, mask, BM_ENTER, BM_EDIT)) {
    focus_->ProcessClear();
    isDirty_ = true;
  } else if (isDirection && mask == BM_EDIT) { // EDIT+ARROW is sent to the field
    focus_->ProcessEditArrow(newButton);
    isDirty_ = true;
  } else if (isDirection && mask == 0) {
    UIField *target = nullptr;
    if (newButton == BM_DOWN) {
      target = findAdjacentField(true, +1);
    } else if (newButton == BM_UP) {
      target = findAdjacentField(true, -1);
    } else if (newButton == BM_RIGHT) {
      target = findAdjacentField(false, +1);
    } else if (newButton == BM_LEFT) {
      target = findAdjacentField(false, -1);
    }
    if (target) {
      SetFocus(target);
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

    if (field == focus_)
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

const GUIRect FieldView::GetFocusRect() {
  ModalView *v = GetModalView();

  if (v) {
    return v->GetFocusRect();
  }

  UpdateFocusRect();

  return focusRect_;
}

void FieldView::addTitleLabel(const char *title, uint8_t line, bool left) {
  // etl doesn't stop at the capacity in release builds, it would write past
  // the end of the vector
  if (staticField_.full()) {
    Trace::Error("FIELDVIEW", "Too many title labels");
    return;
  }
  staticField_.emplace_back(GUIPoint(left ? 1 : 17, line), title);
  staticField_.back().color_ = Theme::View::inactive;
  fieldList_.insert(fieldList_.end(), &staticField_.back());
}

void FieldView::clearTitleLabels() {
  staticField_.clear();
}
