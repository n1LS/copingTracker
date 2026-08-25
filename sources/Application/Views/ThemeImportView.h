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

#ifndef _THEME_IMPORT_VIEW_H_
#define _THEME_IMPORT_VIEW_H_

#include "BaseClasses/FileListView.h"
#include "ModalDialogs/MessageBox.h"
#include "ViewData.h"

/**
 * This view allows users to browse and import theme files (.thm)
 * from the themes directory.
 */
class ThemeImportView : public FileListView {
public:
  ThemeImportView(GUIWindow &w, ViewData *viewData);
  ~ThemeImportView();

  // Required FileListView overrides
  const char *GetEmptyStateMessage() const override;
  void OnFileSelected(const char *filename) override;

  // Custom methods
  void onImportTheme(const char *filename);

private:
  void onImportThemeModalDismiss(View &view, ModalView &dialog);
};

#endif // _THEME_IMPORT_VIEW_H_
