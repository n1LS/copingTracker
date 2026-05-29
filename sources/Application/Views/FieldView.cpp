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
}

void FieldView::ClearFocus() {
  if (focus_) {
    focus_->ClearFocus();
  };
  focus_ = 0;
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

void FieldView::ProcessButtonMask(unsigned short mask, bool pressed) {

  if (focus_ == 0) {
    focus_ = *fieldList_.begin();
    //  Empty field view, we don't have anything to do
    if (focus_ == 0)
      return;
    focus_->SetFocus();
  }

  if (mask & EPBM_ENTER) { // ENTER or ENTER+ARROW is sent to the field
    if (mask & EPBM_DOWN) {
      focus_->ProcessArrow(EPBM_DOWN);
      isDirty_ = true;
    }
    if (mask & EPBM_UP) {
      focus_->ProcessArrow(EPBM_UP);
      isDirty_ = true;
    }

    if (mask & EPBM_LEFT) {
      focus_->ProcessArrow(EPBM_LEFT);
      isDirty_ = true;
    }

    if (mask & EPBM_RIGHT) {
      focus_->ProcessArrow(EPBM_RIGHT);
      isDirty_ = true;
    }

    if (mask & EPBM_EDIT) {
      focus_->ProcessClear();
      isDirty_ = true;
    }

    if (mask == EPBM_ENTER) {
      focus_->OnClick();
    };

  } else {
    if (mask & EPBM_EDIT) { // EDIT or EDIT+ARROW is sent to the field

      if (mask == EPBM_EDIT) {
        focus_->OnEditClick();
        isDirty_ = true;
      };

      if (mask & EPBM_DOWN) {
        focus_->ProcessEditArrow(EPBM_DOWN);
        isDirty_ = true;
      }
      if (mask & EPBM_UP) {
        focus_->ProcessEditArrow(EPBM_UP);
        isDirty_ = true;
      }

      if (mask & EPBM_LEFT) {
        focus_->ProcessEditArrow(EPBM_LEFT);
        isDirty_ = true;
      }

      if (mask & EPBM_RIGHT) {
        focus_->ProcessEditArrow(EPBM_RIGHT);
        isDirty_ = true;
      }

    } else { // Nor ENTER or EDIT is pressed

      if (!(mask & (EPBM_ENTER | EPBM_EDIT | EPBM_ALT | EPBM_NAV | EPBM_SELECT | EPBM_PLAY))) {

        if (mask & EPBM_DOWN) {
          UIField *next = 0;
          UIField *first = 0;

          auto it = fieldList_.begin();
          for (size_t i = 0; i < fieldList_.size(); i++) {
            if (!(*it)->IsStatic()) {
              if (first) {
                if ((*it)->GetPosition().y_ < first->GetPosition().y_) {
                  first = *it;
                };
              } else {
                first = *it;
              }
              if ((*it)->GetPosition().y_ > focus_->GetPosition().y_) {
                if (next) {
                  if ((*it)->GetPosition().y_ < next->GetPosition().y_) {
                    next = *it;
                  } else if ((*it)->GetPosition().y_ == next->GetPosition().y_) {
                    // if both targets at same height, prefer the target with an
                    // X value closest to the current focus

                    // cast to signed ints
                    int32_t itX = (*it)->GetPosition().x_;
                    int32_t nextX = next->GetPosition().x_;
                    int32_t focusX = focus_->GetPosition().x_;

                    if (abs(itX - focusX) < abs(nextX - focusX)) {
                      next = *it;
                    }
                  };
                } else {
                  next = *it;
                };
              };
            }
            it++;
          }
          if (next == 0) {
            next = first;
          }

          focus_->ClearFocus();
          focus_ = next;
          focus_->SetFocus();
          isDirty_ = true;
        }

        if (mask & EPBM_UP) {

          UIField *prev = 0;
          UIField *last = 0;

          auto it = fieldList_.begin();
          for (size_t i = 0; i < fieldList_.size(); i++) {

            if (!(*it)->IsStatic()) {
              if (last) {
                if ((*it)->GetPosition().y_ > last->GetPosition().y_) {
                  last = *it;
                };
              } else {
                last = *it;
              }
              if ((*it)->GetPosition().y_ < focus_->GetPosition().y_) {
                if (prev) {
                  if ((*it)->GetPosition().y_ > prev->GetPosition().y_) {
                    prev = *it;
                  } else if ((*it)->GetPosition().y_ == prev->GetPosition().y_) {
                    // if both targets at same height, prefer the target with an
                    // X value closest to the current focus

                    // cast to signed ints
                    int32_t itX = (*it)->GetPosition().x_;
                    int32_t prevX = prev->GetPosition().x_;
                    int32_t focusX = focus_->GetPosition().x_;

                    if (abs(itX - focusX) < abs(prevX - focusX)) {
                      prev = *it;
                    }
                  };
                } else {
                  prev = *it;
                };
              };
            }
            it++;
          }
          if (prev == 0) {
            prev = last;
          }

          focus_->ClearFocus();
          focus_ = prev;
          focus_->SetFocus();
          isDirty_ = true;
        }

        if (mask & EPBM_RIGHT) {
          UIField *next = 0;
          UIField *first = 0;

          auto it = fieldList_.begin();
          for (size_t i = 0; i < fieldList_.size(); i++) {

            if (!(*it)->IsStatic() && ((*it)->GetPosition().y_ == focus_->GetPosition().y_)) {
              if (first) {
                if ((*it)->GetPosition().x_ < first->GetPosition().x_) {
                  first = *it;
                };
              } else {
                first = *it;
              }
              if ((*it)->GetPosition().x_ > focus_->GetPosition().x_) {
                if (next) {
                  if ((*it)->GetPosition().x_ < next->GetPosition().x_) {
                    next = *it;
                  } else {
                    // if both target at same height
                  };
                } else {
                  next = *it;
                };
              };
            }
            it++;
          }
          if (next == 0) {
            next = first;
          }

          focus_->ClearFocus();
          focus_ = next;
          focus_->SetFocus();
          isDirty_ = true;
        }

        if (mask & EPBM_LEFT) {

          UIField *prev = 0;
          UIField *last = 0;

          auto it = fieldList_.begin();
          for (size_t i = 0; i < fieldList_.size(); i++) {

            if (!(*it)->IsStatic() && ((*it)->GetPosition().y_ == focus_->GetPosition().y_)) {
              if (last) {
                if ((*it)->GetPosition().x_ > last->GetPosition().x_) {
                  last = *it;
                };
              } else {
                last = *it;
              }
              if ((*it)->GetPosition().x_ < focus_->GetPosition().x_) {
                if (prev) {
                  if ((*it)->GetPosition().x_ > prev->GetPosition().x_) {
                    prev = *it;
                  } else {
                    // if both target at same height
                  };
                } else {
                  prev = *it;
                };
              };
            }
            it++;
          }
          if (prev == 0) {
            prev = last;
          }

          focus_->ClearFocus();
          focus_ = prev;
          focus_->SetFocus();
          isDirty_ = true;
        }
      }
    }
  }
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
