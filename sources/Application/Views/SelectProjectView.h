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

#ifndef _SELECTPROJECT_VIEW_H_
#define _SELECTPROJECT_VIEW_H_

#include "BaseClasses/FileListView.h"
#include "ViewData.h"

/**
 * This view allows users to browse, load, and delete projects.
 * Features two action tabs: Load and Delete.
 */
class SelectProjectView : public FileListView {
public:
  SelectProjectView(GUIWindow &w, ViewData *viewData);
  ~SelectProjectView();

  // Required FileListView overrides
  const char *GetEmptyStateMessage() const override;

  // Tab action handler
  virtual void OnTabAction(int tabIndex, const char *filename) override;
  virtual void ConfirmedStop(Token source) override;

  // Public methods
  void getHighlightedProjectName(char *name);

protected:
  virtual void PrepareItemDrawing(int index, bool isSelected, Color *fg, Color *bg, char *buffer,
                                  size_t bufferSize) override;

private:
  // Internal helpers
  void DeleteSelectedProject();
  void LoadSelectedProject();
  bool SelectionIsCurrentProject();
};

#endif // _SELECTPROJECT_VIEW_H_
