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
 * SelectProjectView - Migrated to use FileListView base class
 *
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

  // Public methods
  void getSelectedProjectName(char *name);
  void getHighlightedProjectName(char *name);
  void LoadProject();
  void ClearAutoSave();

protected:
  virtual void PrepareItemDrawing(int index, bool isSelected, Color *fg, Color *bg, char *buffer) override;

private:
  char selection_[MAX_PROJECT_NAME_LENGTH + 1] = {};

  // Internal helpers
  void AttemptDeletingSelectedProject();
  void AttemptLoadingProject();
  bool SelectionIsCurrentProject();
  bool WarnPlayerRunning();
};

#endif // _SELECTPROJECT_VIEW_H_
