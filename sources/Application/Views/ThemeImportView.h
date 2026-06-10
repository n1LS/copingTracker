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

#include "ScreenView.h"
#include "System/FileSystem/FileSystem.h"
#include "ViewData.h"
#include <string>

class ThemeImportView : public ScreenView {
public:
  ThemeImportView(GUIWindow &w, ViewData *viewData);
  ~ThemeImportView();
  void Reset();
  virtual void ProcessButtonMask(uint16_t mask, bool pressed);
  virtual void DrawView();
  virtual void OnPlayerUpdate(PlayerEventType, unsigned int tick = 0);
  virtual void OnFocus();

protected:
  virtual const char *emptyStateMessage() const override;
  void setCurrentFolder();
  void changeSelection(int delta);
  void onImportTheme(const char *filename);

private:
  void onImportThemeModalDismiss(View &view, ModalView &dialog);
  void OpenSelectedItem();

  size_t topIndex_ = 0;
  size_t currentIndex_ = 0;
  etl::vector<int, MAX_FILE_INDEX_SIZE> fileIndexList_;
};
#endif
