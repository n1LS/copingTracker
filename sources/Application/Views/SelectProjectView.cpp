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
#include "Application/Views/ModalDialogs/TextInputModal.h"
#include "Application/Views/ToastView.h"
#include "Foundation/Constants/SpecialCharacters.h"
#include "System/FileSystem/FileSystem.h"
#include "System/System/System.h"
#include <cctype>
#include <cstring>
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
    .actionTabs = {{"Load", 0}, {"Rename", 0}, {"Delete", 0}},
    .allowTabSelection = true};

// ============================================================================
// Callbacks
// ============================================================================

static void LoadProjectCallback(View &v, ModalView &dialog) {
  if (dialog.GetReturnCode() == MBL_YES) {
    // User accepted losing changes. Write the selected project name to the
    // boot-state file (/.current) and reboot so the project loads through the
    // exact same code path as on boot (Application::initProject ->
    // PersistencyService::LoadCurrentProjectName). This guarantees project
    // loading behaves identically to a fresh boot.
    SelectProjectView &view = (SelectProjectView &)v;

    char name[MAX_PROJECT_NAME_LENGTH + 1];
    view.getHighlightedProjectName(name);
    if (strlen(name) == 0) {
      Trace::Log("SELECTPROJECTVIEW", "Cannot load: no valid project name");
      return;
    }

    // Mirror PersistencyService::SaveProjectState byte-for-byte: write only the
    // bare project name (no newline, no terminator) to SD_BASE_DIR "/.current".
    auto fs = FileSystem::GetInstance();
    auto current = fs->Open(PROJECT_STATE_FILE, "w");
    if (!current) {
      Trace::Log("SELECTPROJECTVIEW", "Could not open %s", PROJECT_STATE_FILE);
      return;
    }
    current->Write(name, 1, strlen(name));
    current->Sync();

    Trace::Log("SELECTPROJECTVIEW", "Saved '%s' to .current, rebooting", name);
    System *sys = System::GetInstance();
    sys->SystemReboot(); // watchdog reset; does not return
  }
}

static void RenameProjectCallback(View &v, ModalView &dialog) {
  if (dialog.GetReturnCode() == TextInputModal::TIM_ACCEPT) {
    SelectProjectView &view = (SelectProjectView &)v;
    view.RenameProject(static_cast<TextInputModal &>(dialog).GetValue().c_str());
  }
}

static void DeleteProjectCallback(View &v, ModalView &dialog) {
  if (dialog.GetReturnCode() == MBL_YES) {
    SelectProjectView &view = (SelectProjectView &)v;

    PersistencyService *ps = PersistencyService::GetInstance();
    char buffer[MAX_PROJECT_NAME_LENGTH + 1];
    view.getHighlightedProjectName(buffer);
    const bool deleted = ps->DeleteProject(buffer);

    // DeleteProject walks into the project dir (and on failure may leave us
    // there) and reuses the shared file list, so go back to the projects dir
    // and rebuild the list whether or not the delete worked
    FileSystem::GetInstance()->chdir(PROJECTS_DIR);
    view.RefreshFileList();

    if (!deleted) {
      MessageBox *mb = MessageBox::Create(view, "Delete", "Project could not be deleted", MBBF_OK);
      view.DoModal(mb);
    }
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
  SetBackgroundColor(Theme::View::Selection::bg(isSelected));
  SetColor(Theme::View::Selection::fg(isSelected));

  // Check if this is the current project
  auto var = viewData_->project_->FindVariable(Token::VarProjectName);
  etl::string<MAX_PROJECT_NAME_LENGTH> projectName = var->GetString();
  const char *currentProject = projectName.c_str();

  bool isCurrent = strcmp(temp, currentProject) == 0;

  uint8_t symbol = isCurrent ? CHAR(char_playback_play_s) : ' ';

  int len = FILE_LIST_LINE_LENGTH - 2;
  npf_snprintf(buffer, bufferSize, "%c %-*.*s", symbol, len, len, temp);
}

void SelectProjectView::OnTabAction(int tabIndex, const char *filename) {
  (void)filename; // Unused - we use current selection

  switch (tabIndex) {
    case 0: // Load tab
      ConfirmStopPlayback(Token::ActionImport);
      break;
    case 1: // Rename tab
      RenameSelectedProject();
      break;
    case 2: // Delete tab
      ConfirmStopPlayback(Token::ActionDelete);
      break;
  }
}

void SelectProjectView::getHighlightedProjectName(char *name) {
  name[0] = '\0';
  if (GetCurrentIndex() >= ListView::GetItemCount()) {
    return;
  }

  GetFileName(GetCurrentIndex(), name, MAX_PROJECT_NAME_LENGTH + 1);
}

bool SelectProjectView::SelectionIsCurrentProject() {
  char selected[MAX_PROJECT_NAME_LENGTH + 1];
  getHighlightedProjectName(selected);

  auto var = viewData_->project_->FindVariable(Token::VarProjectName);
  etl::string<MAX_PROJECT_NAME_LENGTH> projectName = var->GetString();
  const char *current = projectName.c_str();

  return strcmp(current, selected) == 0;
}

void SelectProjectView::LoadSelectedProject() {
  if (GetCurrentIndex() >= ListView::GetItemCount()) {
    return;
  }

  MessageBox *mb = MessageBox::Create(*this, "Load", "Load song and lose changes?", MBBF_YES | MBBF_NO);
  DoModal(mb, ModalViewCallback::create<&LoadProjectCallback>());
}

void SelectProjectView::ConfirmedStop(Token source) {
  switch (source) {
    case Token::ActionImport:
      LoadSelectedProject();
      break;
    case Token::ActionDelete:
      DeleteSelectedProject();
      break;
  }
}

void SelectProjectView::RenameSelectedProject() {
  if (GetCurrentIndex() >= GetItemCount()) {
    return;
  }

  char selected[MAX_PROJECT_NAME_LENGTH + 1];
  getHighlightedProjectName(selected);
  if (strlen(selected) == 0) {
    return;
  }

  // the same dialog as the project name in the project view, with the name of the project in it
  TextInputModal *modal = TextInputModal::Create(*this, "Rename project", selected, MAX_PROJECT_NAME_LENGTH);
  DoModal(modal, ModalViewCallback::create<&RenameProjectCallback>());
}

// FAT does not tell "Song" and "song" apart
static bool sameNameIgnoringCase(const char *a, const char *b) {
  while (*a && *b) {
    if (tolower(static_cast<unsigned char>(*a)) != tolower(static_cast<unsigned char>(*b))) {
      return false;
    }
    ++a;
    ++b;
  }
  return *a == *b;
}

// A name that is a directory on the card: not empty, not . or .., none of the characters FAT does not allow
static bool isProjectName(const char *name) {
  if (name[0] == '\0' || strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
    return false;
  }
  return strpbrk(name, "\\/:*?\"<>|") == nullptr;
}

void SelectProjectView::RenameProject(const char *requestedName) {
  char oldName[MAX_PROJECT_NAME_LENGTH + 1];
  getHighlightedProjectName(oldName);

  // without the spaces around it
  char newName[MAX_PROJECT_NAME_LENGTH + 1];
  while (*requestedName == ' ') {
    ++requestedName;
  }
  strncpy(newName, requestedName, MAX_PROJECT_NAME_LENGTH);
  newName[MAX_PROJECT_NAME_LENGTH] = '\0';
  for (size_t length = strlen(newName); length > 0 && newName[length - 1] == ' '; --length) {
    newName[length - 1] = '\0';
  }

  auto fs = FileSystem::GetInstance();

  const char *problem = nullptr;
  if (strlen(oldName) == 0) {
    problem = "No project selected.";
  } else if (SelectionIsCurrentProject()) {
    // the project that is loaded saves to its directory, and the boot state names it
    problem = "Cannot rename the active project.";
  } else if (!isProjectName(newName)) {
    problem = "That is not a project name.";
  } else if (strcmp(newName, oldName) == 0) {
    problem = "The name did not change.";
  } else if (!sameNameIgnoringCase(newName, oldName) && PersistencyService::GetInstance()->Exists(newName)) {
    problem = "A project with that name exists.";
  } else {
    etl::string<128> from(PROJECTS_DIR);
    from.append("/");
    from.append(oldName);
    etl::string<128> to(PROJECTS_DIR);
    to.append("/");
    to.append(newName);
    if (!fs->MoveFile(from.c_str(), to.c_str())) {
      problem = "The project could not be renamed.";
    }
  }

  // The list is built again whether or not it worked, MoveFile may have left us somewhere else, and the
  // renamed project is at another place of the list now.
  fs->chdir(PROJECTS_DIR);
  RefreshFileList();

  if (problem) {
    ToastView::GetInstance()->Show(problem, &ttError, ToastDuration::regular);
    return;
  }

  // select the project with its new name
  for (size_t i = 0; i < GetItemCount(); ++i) {
    char name[MAX_PROJECT_NAME_LENGTH + 1];
    GetFileName(static_cast<int>(i), name, sizeof(name));
    if (strcmp(name, newName) == 0) {
      SetCurrentIndex(i);
      break;
    }
  }
  isDirty_ = true;

  ToastView::GetInstance()->Show("Project renamed.", &ttSuccess, ToastDuration::regular);
}

void SelectProjectView::DeleteSelectedProject() {
  if (GetCurrentIndex() >= GetItemCount()) {
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
