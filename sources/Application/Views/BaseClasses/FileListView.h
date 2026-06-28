/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _FILE_LIST_VIEW_H_
#define _FILE_LIST_VIEW_H_

#include "Externals/etl/include/etl/stack.h"
#include "Externals/etl/include/etl/vector.h"
#include "ScreenView.h"
#include "System/FileSystem/FileSystem.h"
#include "ViewData.h"
#include <algorithm>
#include <cstring>

#define FILE_LIST_LINE_LENGTH (SCREEN_WIDTH - 3)
#define MAX_DIRECTORY_STACK_DEPTH 8

// Forward declaration for file type
using FileType = PicoFileType;

/**
 * Action tab configuration
 */
struct ActionTab {
  const char *label;   // Button label (e.g., "Open", "Delete", "Import")
  uint8_t shortcutKey; // Optional shortcut key (kKeyNone, kKeyPlay, etc.)
};

/**
 * Button configuration for flexible button system
 */
struct ButtonConfig {
  const char *label;   // Button label
  uint8_t shortcutKey; // Optional shortcut key
};

/**
 * Multiple directory configuration for views that need to switch between directories
 */
struct DirectoryConfig {
  const char *name; // Human-readable name for this directory
  const char *path; // Directory path
  bool isDefault;   // Is this the default directory on focus?
};

/**
 * Configuration structure for FileListView
 */
struct FileListConfig {
  const char *title;             // Title displayed at top of view
  const char *startDirectory;    // Initial directory to browse (used if directoryCount == 0)
  const char *fileExtension;     // File extension filter (e.g., ".wav", "")
  uint8_t listFlags;             // FileSystem list flags (loFiles, loFolders, etc.)
  ViewType backNavigationTarget; // ViewType to navigate to on NAV+LEFT
  size_t pageSize;               // Number of items per page

  // Directory navigation control
  bool allowDirectoryNavigation; // If false, directories shown but not enterable
  bool showDirectories;          // If false, hide directories entirely
  bool directoriesAreSelectable; // If true, directories trigger OnItemSelected instead of navigation

  // Action tabs configuration
  etl::vector<ActionTab, 4> actionTabs; // Up to 4 action tabs at bottom
  bool allowTabSelection;               // If true, LEFT/RIGHT cycles through tabs

  // Multiple directory support (for views like SampleImportView)
  DirectoryConfig *directories; // Array of directory configurations
  size_t directoryCount;        // Number of directories in the array
  size_t currentDirectoryIndex; // Current active directory index

  // Button system configuration (alternative to action tabs)
  ButtonConfig *buttons; // Array of button configurations
  size_t buttonCount;    // Number of buttons
  bool useButtonSystem;  // If true, use button system instead of action tabs

  // ENTER release handling
  bool enterOnRelease; // If true, enter directory on ENTER release (not press)
};

/**
 * Base class for file listing views
 *
 * Provides common functionality for browsing files/directories:
 * - Directory scanning and file listing
 * - Navigation with UP/DOWN arrows
 * - Automatic scrolling
 * - Selection highlighting
 * - Action tabs at bottom
 * - Custom item drawing via override
 */
class FileListView : public ScreenView {
public:
  FileListView(GUIWindow &w, ViewData *viewData, const FileListConfig &config);
  virtual ~FileListView();

  // Core view methods (implemented by base)
  void ProcessButtonMask(uint16_t mask, bool pressed) override;
  void DrawView() override;
  void OnFocus() override;

  // Note: Reset() is not in ScreenView, kept for compatibility
  virtual void Reset();

  /// Refresh the file list from current directory (public for callback access)
  void RefreshFileList();

protected:
  // === Required virtual hooks ===

  /// Get empty state message when no files found
  virtual const char *GetEmptyStateMessage() const = 0;

  /// Called when user confirms selection (ENTER on a file)
  /// Default implementation calls OnTabAction with current tab if tabs exist
  virtual void OnItemSelected(const char *filename);

  // === Optional virtual hooks ===

  /// Called when a tab action is triggered
  /// Default implementation calls OnItemSelected
  virtual void OnTabAction(int tabIndex, const char *filename);

  /// Override for custom button handling (return true if handled)
  virtual bool OnButtonOverride(uint16_t mask, bool pressed);

  /// Override for custom item rendering
  /// Default: draws filename with selection highlight
  virtual void PrepareItemDrawing(int index, bool isSelected, Color *fg, Color *bg, char *buffer);

  /// Override for custom action tab drawing
  /// Default: draws standard buttons
  virtual void DrawActionTabs(int y, int selectedTab);

  /// Override for custom button drawing (when useButtonSystem is true)
  /// Default: draws standard buttons with labels from button config
  virtual void DrawButtons(int y, int selectedButton);

  /// Called after directory setup completes
  virtual void OnDirectorySetup();

  /// Called when switching between directories (for multiple directory views)
  /// Return true to allow the switch, false to cancel
  virtual bool OnDirectorySwitch(size_t newDirectoryIndex);

  /// Get additional info to display (e.g., file size, storage indicator)
  /// Called during DrawView for custom status line drawing
  virtual void GetStatusInfo(char *buffer, size_t bufferSize);

  /// Get custom title based on current state (e.g., "Project Pool" vs "Import Sample")
  virtual const char *GetDynamicTitle();

  // === Protected helpers ===

  /// Get the current file system instance
  FileSystem *GetFileSystem() const {
    return fs_;
  }

  /// Get current directory listing
  const etl::vector<int, MAX_FILE_INDEX_SIZE> &GetFileList() const {
    return fileIndexList_;
  }

  /// Get current selection index
  size_t GetCurrentIndex() const {
    return currentIndex_;
  }

  /// Set current selection index
  void SetCurrentIndex(size_t index);

  /// Navigate to parent directory
  bool NavigateToParent();

  /// Navigate to a specific directory
  bool NavigateToDirectory(const char *name);

  /// Check if item at index is a directory
  bool IsDirectory(size_t index) const;

  /// Get filename at index
  void GetFileName(size_t index, char *buffer, size_t bufferSize) const;

  /// Get file type at index
  FileType GetFileType(size_t index) const;

  /// Get file size at index
  uint32_t GetFileSize(size_t index) const;

  /// Check if list is empty
  bool IsEmpty() const {
    return fileIndexList_.empty();
  }

  /// Get total item count
  size_t GetItemCount() const {
    return fileIndexList_.size();
  }

  /// Get page size
  size_t GetPageSize() const {
    return config_.pageSize;
  }

  /// Get selected tab index
  int GetSelectedTab() const {
    return selectedTab_;
  }

  /// Set selected tab index
  void SetSelectedTab(int index);

  /// Get number of action tabs
  size_t GetTabCount() const {
    return config_.actionTabs.size();
  }

  /// Get action tab at index
  const ActionTab &GetActionTab(size_t index) const {
    return config_.actionTabs[index];
  }

  /// Check if tab selection is enabled
  bool HasTabSelection() const {
    return config_.allowTabSelection;
  }

  /// Check if directory navigation is allowed
  bool CanNavigateDirectories() const {
    return config_.allowDirectoryNavigation;
  }

  /// Check if directories should be shown
  bool ShouldShowDirectories() const {
    return config_.showDirectories;
  }

  /// Check if button system is enabled
  bool HasButtonSystem() const {
    return config_.useButtonSystem;
  }

  /// Check if ENTER on release mode is enabled
  bool EnterOnRelease() const {
    return config_.enterOnRelease;
  }

  /// Get current directory index (for multiple directory views)
  size_t GetCurrentDirectoryIndex() const {
    return config_.currentDirectoryIndex;
  }

  /// Switch to a specific directory (for multiple directory views)
  bool SwitchToDirectory(size_t index);

  /// Get directory config at index
  const DirectoryConfig *GetDirectoryConfig(size_t index) const;

  /// Get button config at index
  const ButtonConfig *GetButtonConfig(size_t index) const;

  /// Get current selected button
  int GetSelectedButton() const {
    return selectedButton_;
  }

  /// Set current selected button
  void SetSelectedButton(int index);

  /// Get directory stack depth
  size_t GetDirectoryStackDepth() const {
    return dirIndexStack_.size();
  }

  /// Check if at local root (no parent navigation)
  bool AtLocalRoot() const {
    return atLocalRoot_;
  }

  /// Set at local root flag
  void SetAtLocalRoot(bool value) {
    atLocalRoot_ = value;
  }

  /// Reset navigation state (clear directory stack, set at local root)
  void ResetNavigationState() {
    dirIndexStack_ = etl::stack<uint8_t, MAX_DIRECTORY_STACK_DEPTH>();
    atLocalRoot_ = true;
  }

  /// Set the back navigation target (for dynamic back navigation)
  void SetBackNavigationTarget(ViewType target) {
    config_.backNavigationTarget = target;
  }

private:
  // === Internal helpers ===

  void HandleUp(bool page);
  void HandleDown(bool page);
  void HandleEnter();
  void HandleTabLeft();
  void HandleTabRight();
  void HandleBackNavigation();

  void EnsureVisible();
  void DrawTitleBar();
  void DrawFileList();
  void DrawScrollBar();
  void DrawEmptyState();

  // === Configuration ===

  FileListConfig config_;
  FileSystem *fs_;

  // === State ===
  size_t topIndex_ = 0;
  size_t currentIndex_ = 0;
  int selectedTab_ = 0;
  etl::vector<int, MAX_FILE_INDEX_SIZE> fileIndexList_;

  // === Directory navigation state ===
  etl::stack<uint8_t, MAX_DIRECTORY_STACK_DEPTH> dirIndexStack_; // Track cursor position per directory level
  bool atLocalRoot_ = true;                                      // No parent navigation available

  // === Protected state for subclass access ===
protected:
  int selectedButton_ = 0;           // For button system
  bool enterKeyHeld_ = false;        // Track ENTER key state
  bool pendingDirEnterOnRelease_ = false; // Pending directory enter on release
};

#endif // _FILE_LIST_VIEW_H_
