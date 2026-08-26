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

#include "SelectProjectView.h"
#include "Application/AppWindow.h"
#include "Application/Persistency/PersistencyService.h"
#include "Application/Utils/DrawUtils.h"
#include "Application/Views/ModalDialogs/DeleteProjectConfirmModal.h"
#include "Application/Views/ModalDialogs/MessageBox.h"
#include "BaseClasses/ViewEvent.h"
#include "Foundation/Constants/SpecialCharacters.h"
#include <nanoprintf.h>
#include <new>

#define LIST_PAGE_SIZE (SCREEN_HEIGHT - 4)
#define INVALID_PROJECT_NAME "INVALID NAME"

// Configuration for the FileListView base class
static const FileListConfig kSelectProjectConfig{
    .title = "Browse Projects",
    .startDirectory = PROJECTS_DIR,
    .fileExtension = "", // All folders
    .listFlags = loFolders,
    .backNavigationTarget = VT_PROJECT,
    .pageSize = SCREEN_HEIGHT - 4,
    .allowDirectoryNavigation = true, // Projects are folders
    .showDirectories = true,
    .directoriesAreSelectable = true, // Projects are directories but should be selected, not navigated into
    .actionTabs = {{"Load", 0}, {"Delete", 0}},
    .allowTabSelection = true};

// ============================================================================
// Callbacks
// ============================================================================

static void LoadProjectCallback(View &v, ModalView &dialog) {
  if (dialog.GetReturnCode() == MBL_YES) {
    // User accepted losing changes; clear autosave for the current project.
    ((SelectProjectView &)v).ClearAutoSave();
    ((SelectProjectView &)v).LoadProject();
  }
}

static void DeleteProjectCallback(View &v, ModalView &dialog) {
  if (dialog.GetReturnCode() == MBL_YES) {
    SelectProjectView &view = (SelectProjectView &)v;

    PersistencyService *ps = PersistencyService::GetInstance();
    char buffer[MAX_PROJECT_NAME_LENGTH + 1];
    view.getHighlightedProjectName(buffer);
    if (!ps->DeleteProject(buffer)) {
      MessageBox *mb = MessageBox::Create(view, "Delete", "Project could not be deleted", MBBF_OK);
      view.DoModal(mb);
      return;
    }

    // reload list
    view.RefreshFileList();
  }
}

// ============================================================================
// SelectProjectView implementation
// ============================================================================

SelectProjectView::SelectProjectView(GUIWindow &w, ViewData *viewData)
    : FileListView(w, viewData, kSelectProjectConfig) {
}

SelectProjectView::~SelectProjectView() {
}

const char *SelectProjectView::GetEmptyStateMessage() const {
  return "No projects to show";
}

void SelectProjectView::PrepareItemDrawing(int index, bool isSelected, Color *fg, Color *bg, char *buffer,
                                           size_t bufferSize) {
  (void)bufferSize; // Unused - use fixed size
  // Get filename
  char temp[MAX_PROJECT_NAME_LENGTH + 1];
  memset(temp, '\0', sizeof(temp));

  bool isDirectory = IsDirectory(index);

  if (isDirectory) {
    GetFileName(index, temp, MAX_PROJECT_NAME_LENGTH + 1);
  }

  // SDFat lib doesn't truncate if filename longer than buffer as per docs but
  // instead returns empty string in buffer
  if (strlen(temp) == 0) {
    strcpy(temp, INVALID_PROJECT_NAME);
  }

  // Set colors based on selection
  if (isSelected) {
    SetBackgroundColor(Theme::View::Selection::bg);
    SetColor(Theme::View::Selection::fg);
  } else {
    SetBackgroundColor(Theme::View::bg);
    SetColor(Theme::View::fg);
  }

  // Check if this is the current project
  auto var = viewData_->project_->FindVariable(Token::VarProjectName);
  etl::string<MAX_PROJECT_NAME_LENGTH> projectName = var->GetString();
  const char *currentProject = projectName.c_str();

  bool isCurrent = strcmp(temp, currentProject) == 0;
  Trace::Debug("checking %s vs %s -> %b", temp, currentProject, isCurrent);

  char symbol = isCurrent ? CHAR(char_playback_play_s) : ' ';

  int len = FILE_LIST_LINE_LENGTH - 2;
  npf_snprintf(buffer, SCREEN_WIDTH, "%c %-*.*s", symbol, len, len, temp);
}

void SelectProjectView::OnTabAction(int tabIndex, const char *filename) {
  (void)filename; // Unused - we use current selection

  switch (tabIndex) {
    case 0: // Load tab
      AttemptLoadingProject();
      break;
    case 1: // Delete tab
      AttemptDeletingSelectedProject();
      break;
  }
}

void SelectProjectView::getSelectedProjectName(char *name) {
  strcpy(name, selection_);
}

void SelectProjectView::getHighlightedProjectName(char *name) {
  name[0] = '\0';
  if (GetCurrentIndex() >= ListView::GetItemCount()) {
    return;
  }

  GetFileName(GetCurrentIndex(), name, MAX_PROJECT_NAME_LENGTH + 1);
}

void SelectProjectView::LoadProject() {
  if (GetCurrentIndex() >= ListView::GetItemCount()) {
    return;
  }

  // all subdirs directly inside /project are expected to be projects
  GetFileName(GetCurrentIndex(), selection_, MAX_PROJECT_NAME_LENGTH + 1);
  if (strlen(selection_) == 0) {
    Trace::Log("SELECTPROJECTVIEW", "skipping too long project name on Index: %zu", GetCurrentIndex());
    return;
  }

  Trace::Log("SELECTPROJECTVIEW", "Select Project:%s", selection_);

  ViewEvent ve(VET_LOAD_PROJECT, selection_);
  SetChanged();
  NotifyObservers(&ve);
}

void SelectProjectView::ClearAutoSave() {
  auto var = viewData_->project_->FindVariable(Token::VarProjectName);
  etl::string<MAX_PROJECT_NAME_LENGTH> projectName = var->GetString();
  PersistencyService *ps = PersistencyService::GetInstance();
  if (!projectName.empty()) {
    if (!ps->ClearAutosave(projectName.c_str())) {
      Trace::Log("SELECTPROJECTVIEW", "Autosave clear failed or missing for project: %s", projectName.c_str());
    }
  }
}

bool SelectProjectView::WarnPlayerRunning() {
  if (Player::GetInstance()->IsRunning()) {
    MessageBox *mb = MessageBox::Create(*this, "Error", "Not while running!", MBBF_OK);
    DoModal(mb);
    return true;
  }
  return false;
}

bool SelectProjectView::SelectionIsCurrentProject() {
  char selected[MAX_PROJECT_NAME_LENGTH + 1];
  getHighlightedProjectName(selected);

  auto var = viewData_->project_->FindVariable(Token::VarProjectName);
  etl::string<MAX_PROJECT_NAME_LENGTH> projectName = var->GetString();
  const char *current = projectName.c_str();

  return strcmp(current, selected) == 0;
}

void SelectProjectView::AttemptDeletingSelectedProject() {
  if (GetCurrentIndex() >= ListView::GetItemCount()) {
    return;
  }

  if (WarnPlayerRunning()) {
    return;
  }

  if (SelectionIsCurrentProject()) {
    MessageBox *mb = MessageBox::Create(*this, "Delete", "Cannot delete the active", "project.", MBBF_OK);
    DoModal(mb);
    return;
  }

  char selected[MAX_PROJECT_NAME_LENGTH + 1];
  getHighlightedProjectName(selected);

  ModalView *mb = DeleteProjectConfirmModal::Create(*this, selected);
  DoModal(mb, ModalViewCallback::create<&DeleteProjectCallback>());
}

void SelectProjectView::AttemptLoadingProject() {
  if (GetCurrentIndex() >= ListView::GetItemCount() || WarnPlayerRunning()) {
    return;
  }

  MessageBox *mb = MessageBox::Create(*this, "Load", "Load song and lose changes?", MBBF_YES | MBBF_NO);
  DoModal(mb, ModalViewCallback::create<&LoadProjectCallback>());
}