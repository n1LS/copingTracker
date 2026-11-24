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
 * This view allows users to browse, load, rename and delete projects.
 * Features three action tabs: Load, Rename and Delete.
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
  // Renames the selected project to newName, if it can be: the directory of the project is moved. Shows a toast
  // that tells how it went, rescans the projects and selects the renamed one.
  void RenameProject(const char *newName);

protected:
  virtual void PrepareItemDrawing(int index, bool isSelected, Color *fg, Color *bg, char *buffer,
                                  size_t bufferSize) override;

private:
  // Internal helpers
  void DeleteSelectedProject();
  // asks for the new name of the selected project, RenameProject does it
  void RenameSelectedProject();
  void LoadSelectedProject();
  bool SelectionIsCurrentProject();
};

#endif // _SELECTPROJECT_VIEW_H_
