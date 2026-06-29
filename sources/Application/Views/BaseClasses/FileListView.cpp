/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "FileListView.h"
#include "Application/AppWindow.h"
#include "Application/Model/ThemeConstants.h"
#include "Foundation/Constants/SpecialCharacters.h"
#include "ViewUtils.h"
#include <cstdio>
#include <nanoprintf.h>

// Default page size constant
#ifndef FILE_LIST_PAGE_SIZE
#define FILE_LIST_PAGE_SIZE (SCREEN_HEIGHT - 4)
#endif

FileListView::FileListView(GUIWindow &w, ViewData *viewData, const FileListConfig &config)
    : ScreenView(w, viewData), config_(config), fs_(FileSystem::GetInstance()) {
}

FileListView::~FileListView() {
}

void FileListView::Reset() {
  topIndex_ = 0;
  currentIndex_ = 0;
  selectedTab_ = 0;
  selectedButton_ = 0;
  fileIndexList_.clear();
  dirIndexStack_.clear();
  atLocalRoot_ = true;
  enterKeyHeld_ = false;
  pendingDirEnterOnRelease_ = false;
}

void FileListView::OnFocus() {
  Trace::Debug("[FileListView] OnFocus: startDirectory=%s\n", config_.startDirectory ? config_.startDirectory : "(null)");

  // Handle multiple directory configuration
  if (config_.directoryCount > 0 && config_.directories) {
    // Find and switch to default directory
    for (size_t i = 0; i < config_.directoryCount; i++) {
      if (config_.directories[i].isDefault) {
        SwitchToDirectory(i);
        break;
      }
    }
  } else if (config_.startDirectory) {
    // Navigate to start directory (single directory mode)
    Trace::Debug("[FileListView] >>> Changing to directory: %s\n", config_.startDirectory);

    // Check if directory exists first
    bool exists = fs_->exists(config_.startDirectory);
    Trace::Debug("[FileListView] Directory exists: %d\n", exists ? 1 : 0);

    if (!exists) {
      Trace::Debug("[FileListView] ERROR: Directory does not exist!\n");
    }

    bool result = fs_->chdir(config_.startDirectory);
    Trace::Debug("[FileListView] chdir result: %d\n", result ? 1 : 0);

    if (!result) {
      Trace::Debug("[FileListView] ERROR: chdir failed!\n");
    }
  }

  // Refresh the file list
  RefreshFileList();

  // Call subclass hook
  OnDirectorySetup();
}

void FileListView::RefreshFileList() {
  fileIndexList_.clear();

  // Get directory listing
  fs_->list(&fileIndexList_, config_.fileExtension, config_.listFlags);

  // Filter out "." and apply directory visibility filter
  // Keep ".." when not at local root for navigation
  for (auto it = fileIndexList_.begin(); it != fileIndexList_.end();) {
    char name[PFILENAME_SIZE];
    fs_->getFileName(*it, name, PFILENAME_SIZE);

    bool isDot = (strcmp(name, ".") == 0);
    bool isDotDot = (strcmp(name, "..") == 0);
    bool isDirectory = fs_->getFileType(*it) == PFT_DIR;

    // Always remove "." entry
    if (isDot) {
      it = fileIndexList_.erase(it);
      continue;
    }

    // Remove ".." entry if we're at local root (no parent to navigate to)
    if (isDotDot && atLocalRoot_) {
     it = fileIndexList_.erase(it);
      continue;
    }

    // Filter directories if configured
    if (isDirectory && !ShouldShowDirectories()) {
      it = fileIndexList_.erase(it);
      continue;
    }

    ++it;
  }

  // Reset selection to top
  currentIndex_ = 0;
  topIndex_ = 0;

  // Ensure selection is valid
  if (!fileIndexList_.empty() && currentIndex_ >= fileIndexList_.size()) {
    currentIndex_ = fileIndexList_.size() - 1;
  }

  // Mark view as dirty to trigger redraw
  isDirty_ = true;
}

void FileListView::ProcessButtonMask(uint16_t mask, bool pressed) {
  // Give subclass first chance to handle
  if (OnButtonOverride(mask, pressed)) {
    return;
  }

  // Handle key release events
  if (!pressed) {
    // Handle ENTER release for directory navigation
    if (enterKeyHeld_ && !(mask & BM_ENTER)) {
      enterKeyHeld_ = false;
      if (pendingDirEnterOnRelease_ && !fileIndexList_.empty()) {
        auto fs = FileSystem::GetInstance();
        unsigned fileIndex = fileIndexList_[currentIndex_];
        if (fs->getFileType(fileIndex) == PFT_DIR) {
          char name[PFILENAME_SIZE];
          fs->getFileName(fileIndex, name, PFILENAME_SIZE);
          if (strcmp(name, "..") == 0) {
            NavigateToParent();
          } else {
            NavigateToDirectory(name);
          }
          isDirty_ = true;
        }
      }
      pendingDirEnterOnRelease_ = false;
      return;
    }
    return;
  }

  // Handle ENTER key press
  if (mask & BM_ENTER) {
    if (!enterKeyHeld_) {
      enterKeyHeld_ = true;
      if (EnterOnRelease()) {
        pendingDirEnterOnRelease_ = true;
      }
    }
    // If not in enter-on-release mode, handle immediately
    if (!EnterOnRelease()) {
      HandleEnter();
    }
    // Consume arrow keys to prevent directory enter on release
    if (mask & (BM_LEFT | BM_RIGHT | BM_UP | BM_DOWN)) {
      pendingDirEnterOnRelease_ = false;
    }
    return;
  }

  // Handle navigation
  if (mask & BM_UP) {
    HandleUp(mask & BM_EDIT); // SHIFT+UP for page up
  } else if (mask & BM_DOWN) {
    HandleDown(mask & BM_EDIT); // SHIFT+DOWN for page down
  } else if ((mask & BM_LEFT) && (mask & BM_NAV)) {
    HandleBackNavigation();
  } else if (HasTabSelection()) {
    // Tab navigation (only when tab selection is enabled)
    if (mask & BM_LEFT) {
      HandleTabLeft();
    } else if (mask & BM_RIGHT) {
      HandleTabRight();
    }
  } else if (HasButtonSystem()) {
    // Button navigation (when button system is enabled)
    if (mask & BM_LEFT) {
      SetSelectedButton(selectedButton_ - 1);
    } else if (mask & BM_RIGHT) {
      SetSelectedButton(selectedButton_ + 1);
    }
  }
}

void FileListView::DrawView() {
  Clear();

  // Draw title bar (with dynamic title support)
  DrawTitleBar();

  // Check for empty state
  if (fileIndexList_.empty()) {
    DrawEmptyState();
    return;
  }

  // Draw file list
  DrawFileList();

  // Draw scrollbar if enabled
  DrawScrollBar();

  // Draw action tabs if configured
  if (!config_.actionTabs.empty()) {
    DrawActionTabs(SCREEN_HEIGHT - 1, selectedTab_);
  } else if (config_.useButtonSystem) {
    // Draw buttons if button system is enabled
    DrawButtons(selectedButton_);
  }
}

void FileListView::DrawTitleBar() {
  const char *title = config_.title;

  // Use dynamic title if available
  if (config_.directoryCount > 0) {
    const char *dynamicTitle = GetDynamicTitle();
    if (dynamicTitle) {
      title = dynamicTitle;
    }
  }

  if (title) {
    DrawTitle(char_back_s " %s", title);
  }
}

void FileListView::DrawFileList() {
  int x = 1;
  int y = 2;

  // Ensure current selection is visible
  EnsureVisible();

  // Draw visible items
  size_t pageSize = config_.pageSize;
  size_t total = fileIndexList_.size();

  char buffer[32];

  for (size_t i = topIndex_; i < topIndex_ + pageSize && i < total; i++) {
    bool isSelected = (i == currentIndex_);

    Color bg = isSelected ? Theme::View::Selection::bg : Theme::View::bg;
    Color fg = isSelected ? Theme::View::Selection::fg : Theme::FileList::file;

    PrepareItemDrawing(i, isSelected, &fg, &bg, buffer);
    SetColor(fg);
    SetBackgroundColor(bg);
    DrawString(x, y, buffer);

    // draw selection ends if selected
    if (isSelected) {
      SwapColors();
      DrawString(0, y, char_button_border_left_s);
      DrawString(SCREEN_WIDTH - 2, y, char_button_border_right_s);
    }

    y++;
  }
}

void FileListView::PrepareItemDrawing(int index, bool isSelected, Color *fg, Color *bg, char *buffer) {
  // Default implementation: draw filename with selection highlight
  char temp[32];
  GetFileName(index, temp, PFILENAME_SIZE);

  bool isDirectory = IsDirectory(index);

  // Set colors based on selection and type
  if (isSelected) {
    *fg = Theme::View::Selection::fg;
    *bg = Theme::View::Selection::bg;
  } else {
    *fg = isDirectory ? Theme::FileList::directory : Theme::FileList::file;
    *bg = Theme::View::bg;
  }

  // Draw directory indicator
  char prefix = isDirectory ? CHAR(char_file_folder_s) : CHAR(char_file_file_s);
  int len = FILE_LIST_LINE_LENGTH - 2;
  npf_snprintf(buffer, 32, "%c %-*.*s", prefix, len, len, temp);
}

void FileListView::DrawActionTabs(int y, int selectedTab) {
  if (config_.actionTabs.empty()) {
    return;
  }

  int x = 0;

  for (int i = 0; i < (int)config_.actionTabs.size(); i++) {
    x += DrawTab(x, SCREEN_HEIGHT - 1, config_.actionTabs[i].label, i == selectedTab);
  }
}

void FileListView::DrawButtons(int selectedButton) {
  if (config_.buttons.empty()) {
    return;
  }

  int x = 0;
  
  for (int i = 0; i < (int)config_.buttons.size(); i++) {
    x += DrawButton(x, SCREEN_HEIGHT - 1, config_.buttons[i].label, i == selectedButton);
  }
}

void FileListView::DrawScrollBar() {
  if (fileIndexList_.empty()) {
    return;
  }

  size_t pageSize = config_.pageSize;
  size_t total = fileIndexList_.size();

  drawScrollBar(SCREEN_WIDTH - 1, 2, pageSize, topIndex_, total);
}

void FileListView::DrawEmptyState() {
  drawEmptyState();
}

void FileListView::EnsureVisible() {
  size_t pageSize = config_.pageSize;

  // Ensure current selection is in visible range
  if (currentIndex_ < topIndex_) {
    topIndex_ = currentIndex_;
  } else if (currentIndex_ >= topIndex_ + pageSize) {
    topIndex_ = currentIndex_ - pageSize + 1;
  }
}

void FileListView::HandleUp(bool page) {
  if (fileIndexList_.empty()) {
    return;
  }

  size_t delta = page ? config_.pageSize : 1;

  if (currentIndex_ > 0) {
    int newIndex = static_cast<int>(currentIndex_) - static_cast<int>(delta);
    currentIndex_ = (newIndex < 0) ? 0 : static_cast<size_t>(newIndex);
  }

  isDirty_ = true;
}

void FileListView::HandleDown(bool page) {
  if (fileIndexList_.empty()) {
    return;
  }

  size_t delta = page ? config_.pageSize : 1;
  size_t maxIndex = fileIndexList_.size() - 1;

  if (currentIndex_ < maxIndex) {
    currentIndex_ = std::min(maxIndex, currentIndex_ + delta);
  }

  isDirty_ = true;
}

void FileListView::HandleEnter() {
  if (fileIndexList_.empty()) {
    return;
  }

  char name[PFILENAME_SIZE];
  GetFileName(currentIndex_, name, PFILENAME_SIZE);

  // Check if selected item is a directory
  bool isDirectory = IsDirectory(currentIndex_);

  if (isDirectory) {
    // If directories are selectable, treat them as items
    if (config_.directoriesAreSelectable) {
      OnItemSelected(name);
      return;
    }
    // Handle directory navigation
    if (CanNavigateDirectories()) {
      NavigateToDirectory(name);
    }
    return;
  }

  // Call subclass handler for file selection
  OnItemSelected(name);
}

void FileListView::HandleTabLeft() {
  if (selectedTab_ > 0) {
    selectedTab_--;
    isDirty_ = true;
  }
}

void FileListView::HandleTabRight() {
  if (selectedTab_ < static_cast<int>(config_.actionTabs.size()) - 1) {
    selectedTab_++;
    isDirty_ = true;
  }
}

void FileListView::HandleBackNavigation() {
  Navigate(config_.backNavigationTarget);
}

void FileListView::OnTabAction(int tabIndex, const char *filename) {
  // Default: just call OnItemSelected
  OnItemSelected(filename);
}

bool FileListView::OnButtonOverride(uint16_t mask, bool pressed) {
  return false; // Not handled by default
}

void FileListView::OnDirectorySetup() {
  // Default implementation does nothing
}

bool FileListView::OnDirectorySwitch(size_t newDirectoryIndex) {
  // Default: allow all switches
  (void)newDirectoryIndex;
  return true;
}

const char *FileListView::GetDynamicTitle() {
  // Default: return null to use static title
  return nullptr;
}

void FileListView::OnItemSelected(const char *filename) {
  // Default implementation: if tabs exist, call OnTabAction with current tab
  // Otherwise, do nothing (subclasses should override)
  if (config_.actionTabs.size() > 0) {
    OnTabAction(selectedTab_, filename);
  }
}

void FileListView::SetCurrentIndex(size_t index) {
  if (index < fileIndexList_.size()) {
    currentIndex_ = index;
    EnsureVisible();
    isDirty_ = true;
  }
}

void FileListView::SetSelectedTab(int index) {
  if (index >= 0 && static_cast<size_t>(index) < config_.actionTabs.size()) {
    selectedTab_ = index;
    isDirty_ = true;
  }
}

bool FileListView::NavigateToParent() {
  if (fs_->chdir("..")) {
    // Restore cursor position from stack
    if (!dirIndexStack_.empty()) {
      currentIndex_ = dirIndexStack_.top();
      dirIndexStack_.pop();
    } else {
      currentIndex_ = 0;
    }

    // Update atLocalRoot_ based on stack depth
    atLocalRoot_ = (dirIndexStack_.size() == 0);

    RefreshFileList();
    return true;
  }
  return false;
}

bool FileListView::NavigateToDirectory(const char *name) {
  if (fs_->chdir(name)) {
    // Push current position to stack
    if (dirIndexStack_.size() < MAX_DIRECTORY_STACK_DEPTH) {
      dirIndexStack_.push(static_cast<uint8_t>(currentIndex_));
    }

    // Update atLocalRoot_ based on stack depth
    atLocalRoot_ = (dirIndexStack_.size() == 0);

    currentIndex_ = 0;
    topIndex_ = 0;

    RefreshFileList();
    return true;
  }
  return false;
}

bool FileListView::SwitchToDirectory(size_t index) {
  // Check if switch is allowed
  if (!OnDirectorySwitch(index)) {
    return false;
  }

  // Clear navigation stack for directory switch
  dirIndexStack_.clear();
  atLocalRoot_ = true;
  currentIndex_ = 0;
  topIndex_ = 0;

  // Navigate to the new directory
  if (index < config_.directoryCount && config_.directories) {
    const DirectoryConfig &dirConfig = config_.directories[index];

    if (fs_->chdir(dirConfig.path)) {
      config_.currentDirectoryIndex = index;
      RefreshFileList();
      return true;
    }
  }

  return false;
}

const DirectoryConfig *FileListView::GetDirectoryConfig(size_t index) const {
  if (index < config_.directoryCount && config_.directories) {
    return &config_.directories[index];
  }
  return nullptr;
}

const ButtonConfig *FileListView::GetButtonConfig(size_t index) const {
  if (index < config_.buttons.size()) {
    return &config_.buttons[index];
  }
  return nullptr;
}

void FileListView::SetSelectedButton(int index) {
  if (!config_.buttons.empty()) {
    size_t buttonCount = config_.buttons.size();
    if (index < 0) {
      selectedButton_ = static_cast<int>(buttonCount) - 1;
    } else if (index >= static_cast<int>(buttonCount)) {
      selectedButton_ = 0;
    } else {
      selectedButton_ = index;
    }
    isDirty_ = true;
  }
}

bool FileListView::IsDirectory(size_t index) const {
  if (index >= fileIndexList_.size()) {
    return false;
  }
  return fs_->getFileType(fileIndexList_[index]) == PFT_DIR;
}

void FileListView::GetFileName(size_t index, char *buffer, size_t bufferSize) const {
  if (index >= fileIndexList_.size()) {
    buffer[0] = '\0';
    return;
  }
  fs_->getFileName(fileIndexList_[index], buffer, bufferSize);
}

FileType FileListView::GetFileType(size_t index) const {
  if (index >= fileIndexList_.size()) {
    return PFT_UNKNOWN;
  }
  return static_cast<FileType>(fs_->getFileType(fileIndexList_[index]));
}

uint32_t FileListView::GetFileSize(size_t index) const {
  if (index >= fileIndexList_.size()) {
    return 0;
  }
  return static_cast<uint32_t>(fs_->getFileSize(fileIndexList_[index]));
}