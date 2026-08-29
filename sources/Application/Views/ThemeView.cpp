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

#include "ThemeView.h"
#include "Application/AppWindow.h"
#include "Application/Model/Config.h"
#include "Application/Persistency/PersistenceConstants.h"
#include "Application/Views/ModalDialogs/MessageBox.h"
#include "System/Console/Trace.h"
#include "System/FileSystem/FileSystem.h"
#include <Application/Model/ThemeConstants.h>
#include <System/Console/nanoprintf.h>
#include <stdint.h>

#define FONT_FIELD_LINE 4

#define COLOR_LABEL_WIDTH 16
#define COMPONENT_WIDTH 3

constexpr uint8_t COLOR_COMPONENT_X_COL_POS[COLOR_COMPONENT_COUNT] = {16, 8, 0};
constexpr uint8_t COLOR_COMPONENT_X_OFFSETS[COLOR_COMPONENT_COUNT] = {
    COLOR_LABEL_WIDTH, COLOR_LABEL_WIDTH + COMPONENT_WIDTH, COLOR_LABEL_WIDTH + 2 * COMPONENT_WIDTH};

ThemeView::ThemeView(GUIWindow &w, ViewData *data)
    : FieldView(w, data), colorComponentVar_(Token::VarColor_0, 0),
      themeNameVar_(Token::ActionThemeName, ThemeConstants::DEFAULT_THEME_NAME) {

  GUIPoint position = GetAnchor();

  auto config = Config::GetInstance();

  // Add import/export buttons at the top
  GUIPoint actionPos = position;

  actionPos.y_ -= 1;

  actionField_.emplace_back("Import", Token::ActionImport, actionPos);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);

  actionPos.x_ += 8;
  actionField_.emplace_back("Export", Token::ActionExport, actionPos);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);
  actionPos.y_ += 2;

  // Font selection
  position.y_ = FONT_FIELD_LINE;
  Variable *fontVar = config->FindVariable(Token::VarUIFont);
  intVarField_.emplace_back(position, *fontVar, "Font :%s", 0, ThemeConstants::THEME_FONT_COUNT - 1, 1,
                            ThemeConstants::THEME_FONT_COUNT - 1);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));
  (*intVarField_.rbegin()).AddObserver(*this);
  position.y_ += 1;

  // Get the current theme name from Config
  Variable *configThemeVar = config->FindVariable(Token::VarThemeName);
  etl::string<MAX_THEME_NAME_LENGTH> currentThemeName = "default";

  // If the theme name is set in the config, use it
  if (configThemeVar && !configThemeVar->GetString().empty()) {
    currentThemeName = configThemeVar->GetString();
  }

  // Create the label and default value as variables to avoid temporary objects
  auto label = etl::string<MAX_UITEXTFIELD_LABEL_LENGTH>("Theme ");
  auto defaultValue = etl::string<MAX_THEME_NAME_LENGTH>(currentThemeName);

  themeNameVar_.SetString(currentThemeName.c_str(), false);

  // Add the text field
  textFields_.emplace_back(themeNameVar_, position, label, Token::ActionThemeName, defaultValue);
  themeNameField_ = &(*textFields_.rbegin());
  themeNameField_->AddObserver(*this);
  fieldList_.insert(fieldList_.end(), themeNameField_);

  // Initialize the edit mode flag
  themeNameEditMode_ = false;

  // Initialize the export theme name
  exportThemeName_ = currentThemeName;

  // Colors 0..15
  position.y_ += 3;
  addColorField("Black", config->FindVariable(Token::VarColor_0), BLACK, position);
  position.y_ += 1;
  addColorField("Maroon", config->FindVariable(Token::VarColor_1), RED, position);
  position.y_ += 1;
  addColorField("Green", config->FindVariable(Token::VarColor_2), GREEN, position);
  position.y_ += 1;
  addColorField("Olive", config->FindVariable(Token::VarColor_3), YELLOW, position);
  position.y_ += 1;
  addColorField("Blue", config->FindVariable(Token::VarColor_4), BLUE, position);
  position.y_ += 1;
  addColorField("Purple", config->FindVariable(Token::VarColor_5), MAGENTA, position);
  position.y_ += 1;
  addColorField("Turqoise", config->FindVariable(Token::VarColor_6), CYAN, position);
  position.y_ += 1;
  addColorField("Silver", config->FindVariable(Token::VarColor_7), LIGHT_GRAY, position);
  position.y_ += 1;
  addColorField("Gray", config->FindVariable(Token::VarColor_8), DARK_GRAY, position);
  position.y_ += 1;
  addColorField("Red", config->FindVariable(Token::VarColor_9), LIGHT_RED, position);
  position.y_ += 1;
  addColorField("Lime", config->FindVariable(Token::VarColor_A), LIGHT_GREEN, position);
  position.y_ += 1;
  addColorField("Yellow", config->FindVariable(Token::VarColor_B), LIGHT_YELLOW, position);
  position.y_ += 1;
  addColorField("Light Blue", config->FindVariable(Token::VarColor_C), LIGHT_BLUE, position);
  position.y_ += 1;
  addColorField("Magenta", config->FindVariable(Token::VarColor_D), LIGHT_MAGENTA, position);
  position.y_ += 1;
  addColorField("Cyan", config->FindVariable(Token::VarColor_E), LIGHT_CYAN, position);
  position.y_ += 1;
  addColorField("White", config->FindVariable(Token::VarColor_F), WHITE, position);

  updateColorComponentField();
  intVarField_.emplace_back(colorComponentTargets_[0].position, colorComponentVar_, "%2.2X", 0, 248, 8, 16, 0);
  colorComponentField_ = &(*intVarField_.rbegin());
  fieldList_.insert(fieldList_.end(), colorComponentField_);
  colorComponentField_->AddObserver(*this);
  colorComponentField_->SetPosition({0, SCREEN_HEIGHT});
}

ThemeView::~ThemeView() {
}

void ThemeView::Reset() {
  exportThemeName_.clear();
  themeNameEditMode_ = false;
  forceRedraw_ = false;
  configDirty_ = false;
}

void ThemeView::DrawView() {
  Clear();

  // Draw title

  DrawTitle(char_back_s " Theme Settings");

  // Draw view contents

  drawColorComponentValues();
  FieldView::Redraw();

  // just draw the RGB column headings directly:
  SetBackgroundColor(Theme::View::bg);
  SetColor(Theme::View::inactive);
  DrawString(21, 7, "R  G  B");

  if (isColorComponentFocus()) {
    UIIntVarField &field = intVarField_.back();
    GUIPoint pos = field.GetPosition();
    focusRect_ = GUIRect(pos.x_, pos.y_, pos.x_ + 2, pos.y_);
  }
}

void ThemeView::addSwatchField(Color color, GUIPoint position) {
  position.x_ -= 5;
  swatchField_.emplace_back(position, color);
  fieldList_.insert(fieldList_.end(), &(*swatchField_.rbegin()));
}

void ThemeView::addColorField(const char *label, Variable *colorVar, Color color, GUIPoint position) {
  staticField_.emplace_back(position, label);
  UIStaticField &labelField = *staticField_.rbegin();
  fieldList_.insert(fieldList_.end(), &labelField);

  for (uint8_t i = 0; i < COLOR_COMPONENT_COUNT; ++i) {
    GUIPoint componentPosition = position;
    componentPosition.x_ += COLOR_COMPONENT_X_OFFSETS[i];

    colorComponentTargets_.emplace_back();
    auto &target = colorComponentTargets_.back();
    target.colorVar = colorVar;
    target.position = componentPosition;
    target.shift = COLOR_COMPONENT_X_COL_POS[i];
  }

  addSwatchField(color, position);
}

ThemeView::ColorComponentTarget *ThemeView::selectedColorComponentTarget() {
  uint8_t targetIndex = selectedColor_ * COLOR_COMPONENT_COUNT + selectedColorComponent_;
  if (targetIndex >= colorComponentTargets_.size()) {
    return nullptr;
  }

  return &colorComponentTargets_[targetIndex];
}

bool ThemeView::isColorComponentFocus() {
  return colorComponentField_ != nullptr && FieldView::GetFocus() == colorComponentField_;
}

void ThemeView::updateColorComponentField() {
  ColorComponentTarget *target = selectedColorComponentTarget();
  if (target == nullptr || target->colorVar == nullptr) {
    return;
  }

  uint32_t colorValue = static_cast<uint32_t>(target->colorVar->GetInt());
  uint32_t componentValue = (colorValue >> target->shift) & static_cast<uint32_t>(0xFF);
  colorComponentVar_.SetInt(static_cast<int>(componentValue), false);

  if (colorComponentField_ == nullptr) {
    return;
  }

  GUIPoint position = target->position;
  colorComponentField_->SetPosition(position);

  // update ranges and steps to cleanly map for RGB565
  if (selectedColorComponent_ == 1) {
    colorComponentField_->SetRange(0, 252, 4, 16);
  } else {
    colorComponentField_->SetRange(0, 248, 8, 16);
  }
}

void ThemeView::drawColorComponentValues() {
  SetBackgroundColor(Theme::View::bg);
  SetColor(Theme::View::fg);

  for (uint8_t colorIndex = 0; colorIndex < COLOR_COUNT; ++colorIndex) {
    for (uint8_t componentIndex = 0; componentIndex < COLOR_COMPONENT_COUNT; ++componentIndex) {
      uint8_t targetIndex = colorIndex * COLOR_COMPONENT_COUNT + componentIndex;
      ColorComponentTarget &target = colorComponentTargets_[targetIndex];
      uint32_t colorValue = static_cast<uint32_t>(target.colorVar->GetInt());
      uint32_t componentValue = (colorValue >> target.shift) & static_cast<uint32_t>(0xFF);
      char buffer[3];
      npf_snprintf(buffer, sizeof(buffer), "%2.2X", static_cast<unsigned>(componentValue));
      DrawString(target.position.x_, target.position.y_, buffer);
    }
  }
}

void ThemeView::moveColorComponentFocus(int8_t colorDelta, int8_t componentDelta) {
  if (colorDelta != 0) {
    selectedColor_ = (selectedColor_ + COLOR_COUNT + colorDelta) % COLOR_COUNT;
  }
  if (componentDelta != 0) {
    selectedColorComponent_ =
        (selectedColorComponent_ + COLOR_COMPONENT_COUNT + componentDelta) % COLOR_COMPONENT_COUNT;
  }
  updateColorComponentField();
  SetDirty(true);
}

void ThemeView::syncColorComponentVars(Variable *colorVar) {
  if (colorVar == nullptr) {
    return;
  }

  ColorComponentTarget *target = selectedColorComponentTarget();
  if (target != nullptr && target->colorVar == colorVar) {
    updateColorComponentField();
  }

  if (colorVar->GetID() == Token::VarColor_0) {
    // If the background color changed, we need to force a
    // redraw to update all the colors on the screen
    forceRedraw_ = true;
  }
}

void ThemeView::syncFieldsFromConfig() {
  // Get the current theme name from Config
  Config *config = Config::GetInstance();
  Variable *themeNameVar = config->FindVariable(Token::VarThemeName);

  if (themeNameVar && !themeNameVar->GetString().empty()) {
    // Get the theme name from Config
    etl::string<MAX_THEME_NAME_LENGTH> themeName = themeNameVar->GetString();

    // Update the theme name field
    themeNameVar_.SetString(themeName.c_str());
    themeNameField_->SetVariable(themeNameVar_);
    exportThemeName_ = themeName;
  }

  updateColorComponentField();
}

void ThemeView::Update(Observable &o, I_ObservableData *d) {
  if (!hasFocus_) {
    return;
  }
  UIField *focus = GetFocus();
  focus->ClearFocus();
  focus->Draw(w_);
  focus->SetFocus();
  focus->Draw(w_);
  isDirty_ = true;

  uintptr_t token = (uintptr_t)d;

  if (&o == colorComponentField_) {
    ColorComponentTarget *componentField = selectedColorComponentTarget();
    if (componentField == nullptr || componentField->colorVar == nullptr) {
      return;
    }

    Variable *colorVar = componentField->colorVar;
    uint32_t colorValue = colorVar->GetInt();
    uint32_t newComponentValue = colorComponentVar_.GetInt() & 0xFF;
    colorValue &= ~(static_cast<uint32_t>(0xFF) << componentField->shift);
    colorValue |= newComponentValue << componentField->shift;
    colorVar->SetInt(static_cast<int>(colorValue));
    syncColorComponentVars(colorVar);
    token = colorVar->GetID();
  }

  switch (token) {
    // Handle theme import action
    case Token::ActionImport:
      {
        // Switch to the ThemeImportView
        Navigate(VT_THEME_IMPORT, vtRevealFromCenter);
        return;
      }
    // Handle theme export action
    case Token::ActionExport:
      {
        // Get the theme name from the text field
        exportThemeName_ = themeNameField_->GetString();

        // Check if the theme name is empty
        if (exportThemeName_.empty()) {
          exportThemeName_ = ThemeConstants::DEFAULT_THEME_NAME;
          themeNameVar_.SetString(exportThemeName_.c_str());
          themeNameField_->SetVariable(themeNameVar_);
        }

        // Export the theme
        handleThemeExport();
        return;
      }
    // Handle theme name field
    case Token::ActionThemeName:
      {
        // Update the export theme name
        exportThemeName_ = themeNameField_->GetString();

        // Update the theme name in the Config
        Config *config = Config::GetInstance();
        Variable *themeNameVar = config->FindVariable(Token::VarThemeName);
        if (themeNameVar) {
          themeNameVar->SetString(exportThemeName_.c_str());
          configDirty_ = true;
        }
        themeNameVar_.SetString(exportThemeName_.c_str());
        return;
      }
    // if font changes call redraw all fields
    case Token::VarUIFont:
      {
        // need to force redraw of entire screen to update for font change
        Clear();
        DrawView();
        configDirty_ = true;
        break;
      }
    // Handle color variable changes
    case Token::VarColor_0:
    case Token::VarColor_1:
    case Token::VarColor_2:
    case Token::VarColor_3:
    case Token::VarColor_4:
    case Token::VarColor_5:
    case Token::VarColor_6:
    case Token::VarColor_7:
    case Token::VarColor_A:
    case Token::VarColor_8:
    case Token::VarColor_9:
    case Token::VarColor_B:
    case Token::VarColor_C:
    case Token::VarColor_D:
    case Token::VarColor_E:
    case Token::VarColor_F:
      {
        // Update the AppWindow's color values from Config
        ((AppWindow &)w_).UpdateColorsFromConfig();

        // Force a redraw of the entire screen to update all colors
        forceRedraw_ = true;
        configDirty_ = true;
        break;
      }
    default:
      NInvalid;
      break;
  };
}

void ThemeView::ProcessButtonMask(uint16_t mask, bool pressed) {
  if (!pressed)
    return;

  bool wasColorComponentFocus = isColorComponentFocus();

  if (wasColorComponentFocus && !(mask & (BM_ENTER | BM_EDIT | BM_ALT | BM_NAV | BM_PLAY))) {
    if (mask & BM_DOWN) {
      if (selectedColor_ < COLOR_COUNT - 1) {
        moveColorComponentFocus(1, 0);
        return;
      }
    } else if (mask & BM_UP) {
      if (selectedColor_ > 0) {
        moveColorComponentFocus(-1, 0);
        return;
      }
    } else if (mask & BM_RIGHT) {
      moveColorComponentFocus(0, 1);
      return;
    } else if (mask & BM_LEFT) {
      moveColorComponentFocus(0, -1);
      return;
    }
  }

  FieldView::ProcessButtonMask(mask, pressed);

  if (mask == (BM_NAV | BM_LEFT)) {
    // Go back to Device view with NAV+LEFT
    Navigate(VT_DEVICE, vtCollapse);
  } else if (mask & BM_PLAY) {
    Player *player = Player::GetInstance();
    player->OnStartButton(PM_SONG, viewData_->songX_, false, viewData_->songX_);
  }

  if (!isColorComponentFocus()) {
    colorComponentField_->SetPosition({0, SCREEN_HEIGHT}); // move off screen when not focused
  } else if (!wasColorComponentFocus) {
    colorComponentField_->SetPosition(selectedColorComponentTarget()->position);
    colorComponentField_->SetChanged();
  }
}

void ThemeView::handleThemeExport() {
  // Check if the theme name is valid
  if (exportThemeName_.empty()) {
    exportThemeName_ = ThemeConstants::DEFAULT_THEME_NAME;
  }

  // Build the path to check if the theme already exists
  char pathBuffer[MAX_THEME_EXPORT_PATH_LENGTH + 1];
  memset(pathBuffer, 0, sizeof(pathBuffer));

  strcpy(pathBuffer, THEMES_DIR);
  strcat(pathBuffer, "/");
  strcat(pathBuffer, exportThemeName_.c_str());
  strcat(pathBuffer, THEME_FILE_EXTENSION);

  // Check if theme exists
  auto fs = FileSystem::GetInstance();
  if (fs->exists(pathBuffer)) {
    // Theme exists, ask for confirmation
    MessageBox *mb = MessageBox::Create(*this, "Theme", "Theme already exists", "     Overwrite?", MBBF_YES | MBBF_NO);

    DoModal(mb, ModalViewCallback::create<ThemeView, &ThemeView::onConfirmThemeOverwrite>(*this));
  } else {
    // Theme doesn't exist, export directly
    exportThemeWithName(exportThemeName_.c_str(), false);
  }
}

void ThemeView::onConfirmThemeOverwrite(View &, ModalView &dialog) {
  if (dialog.GetReturnCode() == MBL_YES) {
    exportThemeWithName(exportThemeName_.c_str(), true);
  }
}

void ThemeView::exportThemeWithName(const char *themeName, bool overwrite) {
  // Export the theme using Config
  Config *config = Config::GetInstance();
  bool result = config->ExportTheme(themeName, overwrite);

  if (result) {
    // Update the theme name in the Config
    Variable *themeNameVar = config->FindVariable(Token::VarThemeName);
    if (themeNameVar) {
      themeNameVar->SetString(themeName);
      configDirty_ = true;
    }

    // Update the theme name field
    themeNameVar_.SetString(themeName);
    themeNameField_->SetVariable(themeNameVar_);
    exportThemeName_ = themeName;
  }

  // Show result message
  MessageBox *resultMb =
      MessageBox::Create(*this, "Theme", result ? "Theme exported successfully " : "Failed to export theme", MBBF_OK);
  DoModal(resultMb);
}

void ThemeView::OnFocus() {
  // Refresh local field state from Config when returning from theme import.
  syncFieldsFromConfig();
  forceRedraw_ = true;
  isDirty_ = true;
}

void ThemeView::OnFocusLost() {
  if (!configDirty_) {
    return;
  }

  Config *config = Config::GetInstance();
  if (!config->Save()) {
    Trace::Error("THEMEVIEW", "Failed to save theme config on focus lost");
    return;
  }

  Trace::Log("THEMEVIEW", "Saved theme config on focus lost");
  configDirty_ = false;
}

// Keep this method for backward compatibility
void ThemeView::exportTheme() {
  // This now just calls handleThemeExport
  handleThemeExport();
}

// We've replaced the static callbacks with lambdas and direct methods

void ThemeView::importTheme() {
  // Switch to the theme import view
  Navigate(VT_THEME_IMPORT, vtRevealFromCenter);
}

void ThemeView::AnimationUpdate() {
  if (forceRedraw_) {
    Clear();
    DrawView();
    forceRedraw_ = false;
  }

  ScreenView::AnimationUpdate();
}
