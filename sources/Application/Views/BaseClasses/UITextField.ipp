#include "Application/AppWindow.h"
#include "View.h"
#include "Application/Utils/stringutils.h"

template <uint8_t MaxLength>
UITextField<MaxLength>::UITextField(
    Variable &v, const GUIPoint &position,
    const etl::string<MAX_UITEXTFIELD_LABEL_LENGTH> &label, uint8_t token,
    etl::string<MaxLength> &defaultValue_)
    : UIField(position), src_(&v), label_(label), token_(token),
      defaultValue_(defaultValue_) {}

template <uint8_t MaxLength> UITextField<MaxLength>::~UITextField(){};

template <uint8_t MaxLength>
void UITextField<MaxLength>::Draw(GUIWindow &w, int offset) {

  GUIPoint position = GetPosition();
  position.y_ += offset;

  // Draw the label
  w.SetBackgroundColor(Theme::View::bg);
  w.SetColor(Theme::Input::label);
  w.DrawString(position.x_, position.y_, label_.c_str());
  position.x_ += label_.length();
  
  auto srcString = src_->GetString();
  const char *value;
  int len;
  
  // If the variable's value is empty, use the default value for display
  if (srcString.empty()) {
    value = defaultValue_.c_str();
    len = defaultValue_.length();
    // Use a different color for default values to indicate they're not set
    w.SetColor(Theme::Input::placeholder);
  } else {
    value = srcString.c_str();
    len = srcString.length();
    w.SetColor(Theme::Input::fg(focus_));
  }

  if (focus_) {
    if (len == 0) {
      // For empty fields, draw a cursor at the beginning position
      w.SetBackgroundColor(Theme::Input::cursor);
      w.DrawString(position.x_, position.y_, " ");
    } else {
      w.SetColor(Theme::Input::fg(true));

      char buffer[2];
      buffer[1] = 0;

      for (int i = 0; i < len; i++) {
        buffer[0] = value[i];
        bool active = currentChar_ == i;
        w.SetBackgroundColor(active ? Theme::Input::cursor : Theme::Input::bg(true));
        w.DrawString(position.x_, position.y_, buffer);
        position.x_ += 1;
      }
    }
  } else {
    if (len != 0) {
      w.SetColor(Theme::Input::fg(false));
      w.SetBackgroundColor(Theme::Input::bg(false));
      w.DrawString(position.x_, position.y_, value);
    }
  }
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
  return GetString().size();
}
