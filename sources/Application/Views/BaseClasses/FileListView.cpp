/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "FileListView.h"
#include "ViewUtils.h"
#include "Foundation/Constants/SpecialCharacters.h"
#include "Application/Model/ThemeConstants.h"
#include "Application/AppWindow.h"
#include <nanoprintf.h>
#include <cstdio>

// Default page size constant
#ifndef FILE_LIST_PAGE_SIZE
#define FILE_LIST_PAGE_SIZE (SCREEN_HEIGHT - 4)
#endif

FileListView::FileListView(GUIWindow& w, ViewData* viewData, const FileListConfig& config)
    : ScreenView(w, viewData), config_(config), fs_(FileSystem::GetInstance()) {
}

FileListView::~FileListView() {
}

void FileListView::Reset() {
    topIndex_ = 0;
    currentIndex_ = 0;
    selectedTab_ = 0;
    fileIndexList_.clear();
}

void FileListView::OnFocus() {
    printf("[FileListView] OnFocus: startDirectory=%s\n", 
           config_.startDirectory ? config_.startDirectory : "(null)");
    
    // Navigate to start directory
    if (config_.startDirectory) {
        printf("[FileListView] >>> Changing to directory: %s\n", config_.startDirectory);
        
        // Check if directory exists first
        bool exists = fs_->exists(config_.startDirectory);
        printf("[FileListView] Directory exists: %d\n", exists ? 1 : 0);
        
        if (!exists) {
            printf("[FileListView] ERROR: Directory does not exist!\n");
        }
        
        bool result = fs_->chdir(config_.startDirectory);
        printf("[FileListView] chdir result: %d\n", result ? 1 : 0);
        
        if (!result) {
            printf("[FileListView] ERROR: chdir failed!\n");
        }
    }
    
    // Refresh the file list
    RefreshFileList();
    
    // Call subclass hook
    OnDirectorySetup();
}

void FileListView::RefreshFileList() {
    fileIndexList_.clear();
    
    printf("[FileListView] RefreshFileList: extension=%s, flags=%d\n", 
           config_.fileExtension ? config_.fileExtension : "(none)", 
           config_.listFlags);
    
    // Get directory listing
    fs_->list(&fileIndexList_, config_.fileExtension, config_.listFlags);
    
    printf("[FileListView] After list(): count=%zu\n", fileIndexList_.size());
    
    // Filter out "." and ".." entries and apply directory visibility filter
    for (auto it = fileIndexList_.begin(); it != fileIndexList_.end();) {
        char name[PFILENAME_SIZE];
        fs_->getFileName(*it, name, PFILENAME_SIZE);
        
        bool isDotEntry = (strcmp(name, ".") == 0) || (strcmp(name, "..") == 0);
        bool isDirectory = fs_->getFileType(*it) == PFT_DIR;
        
        printf("[FileListView] Item: %s (dir=%d)\n", name, isDirectory ? 1 : 0);
        
        // Remove dot entries
        if (isDotEntry) {
            printf("[FileListView] Removing dot entry\n");
            it = fileIndexList_.erase(it);
            continue;
        }
        
        // Filter directories if configured
        if (isDirectory && !ShouldShowDirectories()) {
            printf("[FileListView] Filtering directory\n");
            it = fileIndexList_.erase(it);
            continue;
        }
        
        ++it;
    }
    
    printf("[FileListView] After filtering: count=%zu\n", fileIndexList_.size());
    
    // Reset selection to top
    currentIndex_ = 0;
    topIndex_ = 0;
    
    // Ensure selection is valid
    if (!fileIndexList_.empty() && currentIndex_ >= fileIndexList_.size()) {
        currentIndex_ = fileIndexList_.size() - 1;
    }
}

void FileListView::ProcessButtonMask(uint16_t mask, bool pressed) {
    // Give subclass first chance to handle
    if (OnButtonOverride(mask, pressed)) {
        return;
    }
    
    if (!pressed) {
        return;
    }
    
    // Handle navigation
    if (mask & BM_UP) {
        HandleUp(mask & BM_EDIT);  // SHIFT+UP for page up
    } else if (mask & BM_DOWN) {
        HandleDown(mask & BM_EDIT);  // SHIFT+DOWN for page down
    } else if (mask & BM_ENTER) {
        HandleEnter();
    } else if ((mask & BM_LEFT) && (mask & BM_NAV)) {
        HandleBackNavigation();
    } else if (HasTabSelection()) {
        // Tab navigation (only when tab selection is enabled)
        if (mask & BM_LEFT) {
            HandleTabLeft();
        } else if (mask & BM_RIGHT) {
            HandleTabRight();
        }
    }
}

void FileListView::DrawView() {
    Clear();
    
    // Draw title bar
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
    }
}

void FileListView::DrawTitleBar() {
    if (config_.title) {
        DrawTitle("%s", config_.title);
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
    
    printf("[FileListView] DrawFileList: topIndex=%zu, pageSize=%zu, total=%zu\n", 
           topIndex_, pageSize, total);
    
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
    char prefix = isDirectory ? GLYPH(char_file_folder_s) : GLYPH(char_file_file_s);
    npf_snprintf(buffer, 32, "%c %*.s", prefix, FILE_LIST_LINE_LENGTH - 2, temp);
}

void FileListView::DrawActionTabs(int y, int selectedTab) {
    int x = 0;
    size_t tabCount = config_.actionTabs.size();
    
    for (size_t i = 0; i < tabCount; i++) {
        const ActionTab& tab = config_.actionTabs[i];
        bool isSelected = (static_cast<int>(i) == selectedTab);
        
        SetColor(Theme::View::fg);
        SetBackgroundColor(Theme::View::bg);
        
        if (isSelected) {
            SwapColors();
        }
        
        DrawString(x, y, tab.label);
        x += 2 + static_cast<int>(strlen(tab.label));
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

void FileListView::OnTabAction(int tabIndex, const char* filename) {
    // Default: just call OnItemSelected
    OnItemSelected(filename);
}

bool FileListView::OnButtonOverride(uint16_t mask, bool pressed) {
    return false;  // Not handled by default
}

void FileListView::OnDirectorySetup() {
    // Default implementation does nothing
}

void FileListView::OnItemSelected(const char* filename) {
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
        RefreshFileList();
        return true;
    }
    return false;
}

bool FileListView::NavigateToDirectory(const char* name) {
    if (fs_->chdir(name)) {
        RefreshFileList();
        return true;
    }
    return false;
}

bool FileListView::IsDirectory(size_t index) const {
    if (index >= fileIndexList_.size()) {
        return false;
    }
    return fs_->getFileType(fileIndexList_[index]) == PFT_DIR;
}

void FileListView::GetFileName(size_t index, char* buffer, size_t bufferSize) const {
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