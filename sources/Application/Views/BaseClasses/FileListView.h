/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _FILE_LIST_VIEW_H_
#define _FILE_LIST_VIEW_H_

#include "ScreenView.h"
#include "Externals/etl/include/etl/vector.h"
#include "System/FileSystem/FileSystem.h"
#include "ViewData.h"
#include <algorithm>
#include <cstring>

#define FILE_LIST_LINE_LENGTH (SCREEN_WIDTH - 3)

// Forward declaration for file type
using FileType = PicoFileType;

/**
 * Action tab configuration
 */
struct ActionTab {
    const char* label;        // Button label (e.g., "Open", "Delete", "Import")
    uint8_t shortcutKey;      // Optional shortcut key (kKeyNone, kKeyPlay, etc.)
};

/**
 * Configuration structure for FileListView
 */
struct FileListConfig {
    const char* title;                    // Title displayed at top of view
    const char* startDirectory;           // Initial directory to browse
    const char* fileExtension;            // File extension filter (e.g., ".wav", "")
    uint8_t listFlags;                    // FileSystem list flags (loFiles, loFolders, etc.)
    ViewType backNavigationTarget;        // ViewType to navigate to on NAV+LEFT
    size_t pageSize;                      // Number of items per page
    
    // Directory navigation control
    bool allowDirectoryNavigation;        // If false, directories shown but not enterable
    bool showDirectories;                 // If false, hide directories entirely
    
    // Action tabs configuration
    etl::vector<ActionTab, 4> actionTabs; // Up to 4 action tabs at bottom
    bool allowTabSelection;               // If true, LEFT/RIGHT cycles through tabs
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
    FileListView(GUIWindow& w, ViewData* viewData, const FileListConfig& config);
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
    virtual const char* GetEmptyStateMessage() const = 0;
    
    /// Called when user confirms selection (ENTER on a file)
    /// Default implementation calls OnTabAction with current tab if tabs exist
    virtual void OnItemSelected(const char* filename);
    
    // === Optional virtual hooks ===
    
    /// Called when a tab action is triggered
    /// Default implementation calls OnItemSelected
    virtual void OnTabAction(int tabIndex, const char* filename);
    
    /// Override for custom button handling (return true if handled)
    virtual bool OnButtonOverride(uint16_t mask, bool pressed);
    
    /// Override for custom item rendering
    /// Default: draws filename with selection highlight
    virtual void PrepareItemDrawing(int index, bool isSelected, Color *fg, Color *bg, char *buffer);
    
    /// Override for custom action tab drawing
    /// Default: draws standard buttons
    virtual void DrawActionTabs(int y, int selectedTab);
    
    /// Called after directory setup completes
    virtual void OnDirectorySetup();
    
    // === Protected helpers ===
    
    /// Get the current file system instance
    FileSystem* GetFileSystem() const { return fs_; }
    
    /// Get current directory listing
    const etl::vector<int, MAX_FILE_INDEX_SIZE>& GetFileList() const { return fileIndexList_; }
    
    /// Get current selection index
    size_t GetCurrentIndex() const { return currentIndex_; }
    
    /// Set current selection index
    void SetCurrentIndex(size_t index);
    
    /// Navigate to parent directory
    bool NavigateToParent();
    
    /// Navigate to a specific directory
    bool NavigateToDirectory(const char* name);
    
    /// Check if item at index is a directory
    bool IsDirectory(size_t index) const;
    
    /// Get filename at index
    void GetFileName(size_t index, char* buffer, size_t bufferSize) const;
    
    /// Get file type at index
    FileType GetFileType(size_t index) const;
    
    /// Get file size at index
    uint32_t GetFileSize(size_t index) const;
    
    /// Check if list is empty
    bool IsEmpty() const { return fileIndexList_.empty(); }
    
    /// Get total item count
    size_t GetItemCount() const { return fileIndexList_.size(); }
    
    /// Get page size
    size_t GetPageSize() const { return config_.pageSize; }
    
    /// Get selected tab index
    int GetSelectedTab() const { return selectedTab_; }
    
    /// Set selected tab index
    void SetSelectedTab(int index);
    
    /// Get number of action tabs
    size_t GetTabCount() const { return config_.actionTabs.size(); }
    
    /// Get action tab at index
    const ActionTab& GetActionTab(size_t index) const { return config_.actionTabs[index]; }
    
    /// Check if tab selection is enabled
    bool HasTabSelection() const { return config_.allowTabSelection; }
    
    /// Check if directory navigation is allowed
    bool CanNavigateDirectories() const { return config_.allowDirectoryNavigation; }
    
    /// Check if directories should be shown
    bool ShouldShowDirectories() const { return config_.showDirectories; }
    
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
};

#endif // _FILE_LIST_VIEW_H_