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


#include "UITextField.h"
#include "Application/AppWindow.h"
#include "View.h"
#include "Application/Utils/stringutils.h"
#include <System/Console/nanoprintf.h>

template <uint8_t MaxLength>
UITextField<MaxLength>::UITextField(Variable &v, const GUIPoint &position,
                                    const etl::string<MAX_UITEXTFIELD_LABEL_LENGTH> &label, uint8_t token,
                                    etl::string<MaxLength> &defaultValue_) : UIField(position), src_(&v), label_(label), token_(token),
      defaultValue_(defaultValue_) {}

template <uint8_t MaxLength> UITextField<MaxLength>::~UITextField(){};

template <uint8_t MaxLength>
void UITextField<MaxLength>::Draw(GUIWindow &w, int offset) {
  GUIPoint position = GetPosition();
  position.y_ += offset;

  auto srcString = src_->GetString();
  const char *value;

  // If the variable's value is empty, use the default value for display
   if (!srcString.empty()) {
     value = srcString.c_str();
   } else {
     value = defaultValue_.c_str();
   }

  // borders pre and post text
  char buffer[33];
  npf_snprintf(buffer, sizeof(buffer), "%s:%-*.*s", label_.c_str(), MaxLength, MaxLength, value);
  DrawLabeledField(w, position, buffer, currentChar_, 1);
  focusWidth_ = MaxLength + 2;
}

template <uint8_t MaxLength> void UITextField<MaxLength>::OnClick() {
  SetChanged();
  NotifyObservers(
      reinterpret_cast<I_ObservableData *>(static_cast<uintptr_t>(token_)));
}

template <uint8_t MaxLength> void UITextField<MaxLength>::OnEditClick() {
  etl::string<MAX_VARIABLE_STRING_LENGTH> buffer(src_->GetString());
  if (currentChar_ > 0 && currentChar_ < buffer.length()) {
    buffer.erase(currentChar_, 1);
    currentChar_--;
  }
  src_->SetString(buffer.c_str(), true);
  SetChanged();
  NotifyObservers(
      reinterpret_cast<I_ObservableData *>(static_cast<uintptr_t>(token_)));
}

template <uint8_t MaxLength>
void UITextField<MaxLength>::ProcessArrow(uint16_t mask) {
  etl::string<MAX_VARIABLE_STRING_LENGTH> buffer(src_->GetString());
  auto applyAndNotify = [&]() {
    src_->SetString(buffer.c_str(), true);
    SetChanged();
    NotifyObservers(
        reinterpret_cast<I_ObservableData *>(static_cast<uintptr_t>(token_)));
  };

  // If the variable's value is empty, we need to initialize it when the user
  // starts editing
  bool isEmptyBuffer = buffer.empty();

  switch (mask) {
  case BM_UP:
  case BM_DOWN:
    // If buffer is empty or matches default, initialize with 'A'
    if (isEmptyBuffer || buffer.compare(defaultValue_) == 0) {
      currentChar_ = 0;
      buffer = "A";
    } else {
      buffer[currentChar_] = 
        getNext(buffer.c_str()[currentChar_], mask == BM_DOWN);
    }
    applyAndNotify();
    break;
  case BM_LEFT:
    // If we're showing the default value and user presses left, initialize with
    // the default
    if (isEmptyBuffer) {
      buffer = defaultValue_;
      applyAndNotify();
    }
    if (currentChar_ > 0) {
      currentChar_--;
    }
    break;
  case BM_RIGHT:
    // If we're showing the default value and user presses right, initialize
    // with the default
    if (isEmptyBuffer) {
      buffer = defaultValue_;
      applyAndNotify();
    }
    if (currentChar_ < (buffer.length() - 1)) {
      currentChar_++;
      // -1 to allow for adding 1 more char
    } else if (currentChar_ < (MaxLength - 1)) {
      currentChar_++;
      char str[2] = {lastUsedChar_, 0};
      buffer.append(str);
      applyAndNotify();
    }
    break;
  };

  // remember last used char for appending when user moves right at the end
  // of the string
  lastUsedChar_= buffer.c_str()[currentChar_];
}

template <uint8_t MaxLength>
etl::string<MaxLength> UITextField<MaxLength>::GetString() {
  return src_->GetString().substr(0, MaxLength);
}

template <uint8_t MaxLength>
void UITextField<MaxLength>::SetVariable(Variable &v) {
  // Set the variable this UITextField is bound to
  src_ = &v;
  currentChar_ = 0; // Reset cursor position
}

template <uint8_t MaxLength>
int UITextField<MaxLength>::GetFocusOffset() {
  return strlen(label_.c_str());
}

template <uint8_t MaxLength>
int UITextField<MaxLength>::GetFocusWidth() {
  return focusWidth_;
}

// Explicit template instantiations so the linker can resolve symbols for the
// MaxLength values used across the application.
template class UITextField<16>; // MAX_INSTRUMENT_NAME_LENGTH, MAX_PROJECT_NAME_LENGTH, MAX_THEME_NAME_LENGTH
