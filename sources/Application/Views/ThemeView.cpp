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
    : FieldView(w, data), colorComponentVar_(FourCC::VarColor_0_Black, 0),
      themeNameVar_(FourCC::ActionThemeName, ThemeConstants::DEFAULT_THEME_NAME) {

  GUIPoint position = GetAnchor();

  auto config = Config::GetInstance();

  // Add import/export buttons at the top
  GUIPoint actionPos = position;

  actionPos.y_ -= 1;

  actionField_.emplace_back("Import", FourCC::ActionImport, actionPos);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);

  actionPos.x_ += 8;
  actionField_.emplace_back("Export", FourCC::ActionExport, actionPos);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);
  actionPos.y_ += 2;

  // Font selection
  position.y_ = FONT_FIELD_LINE;
  Variable *fontVar = config->FindVariable(FourCC::VarUIFont);
  intVarField_.emplace_back(position, *fontVar, "Font :%s", 0, ThemeConstants::THEME_FONT_COUNT - 1, 1,
                            ThemeConstants::THEME_FONT_COUNT - 1);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));
  (*intVarField_.rbegin()).AddObserver(*this);
  position.y_ += 1;

  // Get the current theme name from Config
  Variable *configThemeVar = config->FindVariable(FourCC::VarThemeName);
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
  textFields_.emplace_back(themeNameVar_, position, label, FourCC::ActionThemeName, defaultValue);
  themeNameField_ = &(*textFields_.rbegin());
  themeNameField_->AddObserver(*this);
  fieldList_.insert(fieldList_.end(), themeNameField_);

  // Initialize the edit mode flag
  themeNameEditMode_ = false;

  // Initialize the export theme name
  exportThemeName_ = currentThemeName;

  // Colors 0..15
  position.y_ += 3;
  addColorField("Black", config->FindVariable(FourCC::VarColor_0_Black), BLACK, position);
  position.y_ += 1;
  addColorField("Maroon", config->FindVariable(FourCC::VarColor_1_Maroon), RED, position);
  position.y_ += 1;
  addColorField("Green", config->FindVariable(FourCC::VarColor_2_Green), GREEN, position);
  position.y_ += 1;
  addColorField("Olive", config->FindVariable(FourCC::VarColor_3_Olive), YELLOW, position);
  position.y_ += 1;
  addColorField("Blue", config->FindVariable(FourCC::VarColor_4_Blue), BLUE, position);
  position.y_ += 1;
  addColorField("Purple", config->FindVariable(FourCC::VarColor_5_Purple), MAGENTA, position);
  position.y_ += 1;
  addColorField("Turqoise", config->FindVariable(FourCC::VarColor_6_Turqoise), CYAN, position);
  position.y_ += 1;
  addColorField("Silver", config->FindVariable(FourCC::VarColor_7_LightyGray), LIGHT_GRAY, position);
  position.y_ += 1;
  addColorField("Gray", config->FindVariable(FourCC::VarColor_8_Gray), DARK_GRAY, position);
  position.y_ += 1;
  addColorField("Red", config->FindVariable(FourCC::VarColor_9_Red), LIGHT_RED, position);
  position.y_ += 1;
  addColorField("Lime", config->FindVariable(FourCC::VarColor_A_Lime), LIGHT_GREEN, position);
  position.y_ += 1;
  addColorField("Yellow", config->FindVariable(FourCC::VarColor_B_Yellow), LIGHT_YELLOW, position);
  position.y_ += 1;
  addColorField("Light Blue", config->FindVariable(FourCC::VarColor_C_LightBlue), LIGHT_BLUE, position);
  position.y_ += 1;
  addColorField("Magenta", config->FindVariable(FourCC::VarColor_D_Magenta), LIGHT_MAGENTA, position);
  position.y_ += 1;
  addColorField("Cyan", config->FindVariable(FourCC::VarColor_E_Cyan), LIGHT_CYAN, position);
  position.y_ += 1;
  addColorField("White", config->FindVariable(FourCC::VarColor_F_White), WHITE, position);

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

  GUIPoint pos = GetTitlePosition();

  // Draw title
  SetBackgroundColor(cccccBackground);
  SetColor(cccccNormal);
  DrawString(pos.x_, pos.y_, "Theme Settings");

  // bit of a hack needed for font change as going from "standard" to "bold"
  // will leave behind partial characters due to different width of those string
  // labels
  DrawString(5, FONT_FIELD_LINE, "                            ");

  drawColorComponentValues();
  FieldView::Redraw();

  // just draw the RGB column headings directly:
  SetBackgroundColor(cccccBackground);
  SetColor(cccccConsole);
  DrawString(21, 7, "R  G  B");
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
  SetBackgroundColor(cccccBackground);
  SetColor(cccccNormal);

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
    selectedColor_ = static_cast<uint8_t>((selectedColor_ + COLOR_COUNT + colorDelta) % COLOR_COUNT);
  }
  if (componentDelta != 0) {
    selectedColorComponent_ = static_cast<uint8_t>((selectedColorComponent_ + COLOR_COMPONENT_COUNT + componentDelta) %
                                                   COLOR_COMPONENT_COUNT);
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

  if (colorVar->GetID() == FourCC::VarColor_0_Black) {
    // If the background color changed, we need to force a
    // redraw to update all the colors on the screen
    forceRedraw_ = true;
  }
}

void ThemeView::syncFieldsFromConfig() {
  // Get the current theme name from Config
  Config *config = Config::GetInstance();
  Variable *themeNameVar = config->FindVariable(FourCC::VarThemeName);

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
  w_.Flush();
  focus->SetFocus();
  focus->Draw(w_);
  isDirty_ = true;

  uintptr_t fourcc = (uintptr_t)d;

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
    fourcc = colorVar->GetID();
  }

  switch (fourcc) {
    // Handle theme import action
    case FourCC::ActionImport:
      {
        // Switch to the ThemeImportView
        Navigate(VT_THEME_IMPORT);
        return;
      }
    // Handle theme export action
    case FourCC::ActionExport:
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
    case FourCC::ActionThemeName:
      {
        // Update the export theme name
        exportThemeName_ = themeNameField_->GetString();

        // Update the theme name in the Config
        Config *config = Config::GetInstance();
        Variable *themeNameVar = config->FindVariable(FourCC::VarThemeName);
        if (themeNameVar) {
          themeNameVar->SetString(exportThemeName_.c_str());
          configDirty_ = true;
        }
        themeNameVar_.SetString(exportThemeName_.c_str());
        return;
      }
    // if font changes call redraw all fields
    case FourCC::VarUIFont:
      {
        // need to force redraw of entire screen to update for font change
        ForceClear();
        DrawView();
        configDirty_ = true;
        break;
      }
    // Handle color variable changes
    case FourCC::VarColor_0_Black:
    case FourCC::VarColor_1_Maroon:
    case FourCC::VarColor_2_Green:
    case FourCC::VarColor_3_Olive:
    case FourCC::VarColor_4_Blue:
    case FourCC::VarColor_5_Purple:
    case FourCC::VarColor_6_Turqoise:
    case FourCC::VarColor_7_LightyGray:
    case FourCC::VarColor_A_Lime:
    case FourCC::VarColor_8_Gray:
    case FourCC::VarColor_9_Red:
    case FourCC::VarColor_B_Yellow:
    case FourCC::VarColor_C_LightBlue:
    case FourCC::VarColor_D_Magenta:
    case FourCC::VarColor_E_Cyan:
    case FourCC::VarColor_F_White:
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

void ThemeView::ProcessButtonMask(unsigned short mask, bool pressed) {
  if (!pressed)
    return;

  bool wasColorComponentFocus = isColorComponentFocus();

  if (wasColorComponentFocus && !(mask & (EPBM_ENTER | EPBM_EDIT | EPBM_ALT | EPBM_NAV | EPBM_SELECT | EPBM_PLAY))) {
    if (mask & EPBM_DOWN) {
      if (selectedColor_ < COLOR_COUNT - 1) {
        moveColorComponentFocus(1, 0);
        return;
      }
    } else if (mask & EPBM_UP) {
      if (selectedColor_ > 0) {
        moveColorComponentFocus(-1, 0);
        return;
      }
    } else if (mask & EPBM_RIGHT) {
      moveColorComponentFocus(0, 1);
      return;
    } else if (mask & EPBM_LEFT) {
      moveColorComponentFocus(0, -1);
      return;
    }
  }

  FieldView::ProcessButtonMask(mask, pressed);

  if (mask & EPBM_NAV) {
    if (mask & EPBM_LEFT) {
      // Go back to Device view with NAV+LEFT
      Navigate(VT_DEVICE);
    }
  } else if (mask & EPBM_PLAY) {
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
    MessageBox *mb = MessageBox::Create(*this, "Theme already exists", "     Overwrite?", MBBF_YES | MBBF_NO);

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
    Variable *themeNameVar = config->FindVariable(FourCC::VarThemeName);
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
      MessageBox::Create(*this, result ? "Theme exported successfully " : "Failed to export theme", MBBF_OK);
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
  Navigate(VT_THEME_IMPORT);
}
void ThemeView::AnimationUpdate() {
  if (forceRedraw_) {
    ForceClear();
    DrawView();
    forceRedraw_ = false;
  }
  drawBattery();
  drawPowerButtonUI();
}
