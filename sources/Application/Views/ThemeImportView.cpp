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

#include "ThemeImportView.h"
#include "Application/AppWindow.h"
#include "Application/Model/Config.h"
#include "Application/Persistency/PersistenceConstants.h"
#include "Application/Views/ModalDialogs/MessageBox.h"
#include "ModalDialogs/MessageBox.h"
#include "System/Console/Trace.h"
#include "System/FileSystem/FileSystem.h"
#include "ThemeView.h"
#include <nanoprintf.h>

// Configuration for the FileListView base class
static const FileListConfig kThemeImportConfig{.title = "Import Theme",
                                               .startDirectory = THEMES_DIR,
                                               .fileExtension = THEME_FILE_EXTENSION,
                                               .listFlags = loFiles,
                                               .backNavigationTarget = VT_THEME,
                                               .pageSize = SCREEN_HEIGHT - 4,
                                               .allowDirectoryNavigation = false,
                                               .showDirectories = false,
                                               .actionTabs = {}, // No tabs - ENTER directly imports
                                               .allowTabSelection = false};

ThemeImportView::ThemeImportView(GUIWindow &w, ViewData *viewData) : FileListView(w, viewData, kThemeImportConfig) {
}

ThemeImportView::~ThemeImportView() {
}

const char *ThemeImportView::GetEmptyStateMessage() const {
  return "No themes to show";
}

void ThemeImportView::OnFileSelected(const char *filename) {
  onImportTheme(filename);
}

void ThemeImportView::onImportTheme(const char *filename) {
  // Use Config's ImportTheme method directly
  Config *config = Config::GetInstance();
  bool result = config->ImportTheme(filename);

  if (result) {
    // Get the AppWindow to update colors
    AppWindow &app = (AppWindow &)w_;
    app.UpdateColorsFromConfig();

    // make sure we redraw everything with the new colors
    Clear();

    // Show success message
    MessageBox *mb = MessageBox::Create(*this, "Import", "Theme imported successfully", MBBF_OK);
    DoModal(mb, ModalViewCallback::create<ThemeImportView, &ThemeImportView::onImportThemeModalDismiss>(*this));
  } else {
    // Show error message
    MessageBox *mb = MessageBox::Create(*this, "Import", "Failed to import theme", MBBF_OK);
    DoModal(mb, ModalViewCallback::create<ThemeImportView, &ThemeImportView::onImportThemeModalDismiss>(*this));
  }
}

void ThemeImportView::onImportThemeModalDismiss(View &, ModalView &dialog) {
  if (dialog.GetReturnCode() != MBL_OK) {
    return;
  }

  Navigate(VT_THEME);
}