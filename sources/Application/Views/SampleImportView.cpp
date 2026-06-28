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

#include "SampleImportView.h"
#include "Application/Audio/AudioFileStreamer.h"
#include "Application/Instruments/SampleInstrument.h"
#include "Application/Instruments/SamplePool.h"
#include "Application/Persistency/PersistenceConstants.h"
#include "Application/Views/SampleEditorView.h"
#include "Application/Views/ModalDialogs/MessageBox.h"
#include "Application/Utils/char.h"
#include "Externals/etl/include/etl/string.h"
#include "Externals/etl/include/etl/to_string.h"
#include "Foundation/Constants/SpecialCharacters.h"
#include "System/Console/Trace.h"
#include "ViewUtils.h"
#include <memory>
#include <nanoprintf.h>

// Configuration for the FileListView base class
static const FileListConfig kSampleImportConfig{
    .title = "Import Sample",
    .startDirectory = SAMPLES_LIB_DIR,
    .fileExtension = ".wav",
    .listFlags = loFiles | loFolders,
    .backNavigationTarget = VT_PROJECT,
    .pageSize = SCREEN_HEIGHT - 5,
    .allowDirectoryNavigation = true,
    .showDirectories = true,
    .directoriesAreSelectable = false,
    .allowTabSelection = false,
    .directories = nullptr,
    .directoryCount = 0,
    .currentDirectoryIndex = 0,
    .buttons = nullptr,
    .buttonCount = 3,
    .useButtonSystem = true,
    .enterOnRelease = true}; // Open dir on ENTER release

// Initialize static member
ViewType SampleImportView::sourceViewType_ = VT_PROJECT;

void SampleImportView::SetSourceViewType(ViewType vt) {
  sourceViewType_ = vt;
}

SampleImportView::SampleImportView(GUIWindow &w, ViewData *viewData)
    : FileListView(w, viewData, kSampleImportConfig) {
}

SampleImportView::~SampleImportView() {
}

void SampleImportView::Reset() {
  FileListView::Reset();
  previewPlayingIndex_ = 0;
  playKeyHeld_ = false;
  editKeyHeld_ = false;
  inProjectSampleDir_ = false;
}

const char *SampleImportView::GetEmptyStateMessage() const {
  return "Sample pool is empty.";
}

void SampleImportView::OnFocus() {
  // Override OnFocus to handle dual directory mode
  // First, call base class to do initial setup
  FileListView::OnFocus();
  
  // Check if we should show project pool (set by ProjectView)
  if (viewData_->isShowingSampleEditorProjectPool) {
    inProjectSampleDir_ = true;
    
    // Set back navigation to return to ProjectView
    SetBackNavigationTarget(VT_PROJECT);
    
    // Navigate to: PROJECTS_DIR / projectName / samples
    auto fs = GetFileSystem();
    
    // Get project name using the public accessor
    // Note: GetString() is not const, so we cast away const
    StringWatchedVariable<MAX_PROJECT_NAME_LENGTH> &projectNameVar = 
      const_cast<StringWatchedVariable<MAX_PROJECT_NAME_LENGTH>&>(viewData_->project_->GetProjectNameString());
    etl::string<MAX_PROJECT_NAME_LENGTH> projectName = projectNameVar.GetString();
    
    // First go to projects directory, then to project, then to samples
    fs->chdir(PROJECTS_DIR);
    fs->chdir(projectName.c_str());
    fs->chdir(PROJECT_SAMPLES_DIR);
    
    // Reset navigation state and refresh
    ResetNavigationState();
    RefreshFileList();
  } else {
    // Not in project pool mode - ensure we're in the general library
    // Reset the inProjectSampleDir_ flag to ensure correct button display
    inProjectSampleDir_ = false;
    
    // Set back navigation to return to InstrumentView
    SetBackNavigationTarget(VT_INSTRUMENT);
    
    // Navigate back to the general samples library if we're not already there
    auto fs = GetFileSystem();
    if (!fs->chdir(SAMPLES_LIB_DIR)) {
      Trace::Error("SAMPLEIMPORT", "Failed to chdir to SAMPLES_LIB_DIR");
    }
    ResetNavigationState();
    RefreshFileList();
  }
}

void SampleImportView::OnDirectorySetup() {
  // No additional setup needed after directory switch
}

void SampleImportView::PrepareItemDrawing(int index, bool isSelected, Color *fg, Color *bg, char *buffer) {
  auto fs = GetFileSystem();

  // Get filename
  char temp[PFILENAME_SIZE];
  GetFileName(index, temp, PFILENAME_SIZE);

  // Set colors based on selection
  if (isSelected) {
    *bg = Theme::View::Selection::bg;
    *fg = Theme::View::Selection::fg;
  } else {
    *bg = Theme::View::bg;
  }

  bool isDirectory = IsDirectory(index);

  // Determine the symbol to use
  char symbol;
  if (isDirectory) {
    // Use return symbol for ".." directory
    symbol = (strcmp(temp, "..") == 0) ? CHAR(char_symbol_return_s) : CHAR(char_file_folder_s);
    if (!isSelected) {
      *fg = Theme::FileList::directory;
    }
  } else {
    symbol = CHAR(char_file_file_s);
    if (!isSelected) {
      *fg = Theme::FileList::file;
    }
  }

  // Draw the item with symbol and filename
  int len = FILE_LIST_LINE_LENGTH - 2;
  npf_snprintf(buffer, SCREEN_WIDTH, "%c %-*.*s", symbol, len, len, temp);
}

void SampleImportView::GetStatusInfo(char *buffer, size_t bufferSize) {
  auto fs = GetFileSystem();
  uint32_t availableSpace = SamplePool::GetInstance()->GetAvailableSampleStorageSpace();

  uint32_t filesize = 0;
  if (!IsEmpty()) {
    unsigned fileIndex = GetFileList()[GetCurrentIndex()];
    // only get file size if it's a file not a dir
    if (fs->getFileType(fileIndex) == PFT_FILE) {
      filesize = fs->getFileSize(fileIndex);
    }
  }

  npf_snprintf(buffer, bufferSize, "Size: %i/%i", filesize, availableSpace);
}

void SampleImportView::DrawButtons(int y, int selectedButton) {
  int x = 0;

  // ensure selectedButton is valid
  uint8_t buttonCount = (uint8_t)(inProjectSampleDir_ ? 3 : 3);
  if (selectedButton_ < 0 || selectedButton_ >= buttonCount) {
    selectedButton_ = 0;
  }

  int previewVolume = 0;
  Variable *v = viewData_->project_->FindVariable(FourCC::VarPreviewVolume);
  if (v) {
    previewVolume = v->GetInt();
  }

  if (!inProjectSampleDir_) {
    DrawButton(x, y, "Import", selectedButton_ == 0);
    DrawButton(x + 8, y, "Edit", selectedButton_ == 1);
  } else {
    DrawButton(x, y, "Edit", selectedButton_ == 0);
    DrawButton(x + 6, y, "N/A", selectedButton_ == 1); // todo: make remove available or remove the button
  }

  char volField[12];
  npf_snprintf(volField, sizeof(volField), "Vol:%2d", previewVolume);
  DrawButton(x + 24, y, volField, selectedButton_ == 2);
}

bool SampleImportView::OnButtonOverride(uint16_t mask, bool pressed) {
  // Check for key release events
  if (!pressed) {
    // Open selected directory only when ENTER is released
    if (enterKeyHeld_ && !(mask & BM_ENTER)) {
      enterKeyHeld_ = false;
      if (pendingDirEnterOnRelease_ && !IsEmpty()) {
        unsigned fileIndex = GetFileList()[GetCurrentIndex()];
        auto fs = GetFileSystem();
        if (fs->getFileType(fileIndex) == PFT_DIR) {
          char name[PFILENAME_SIZE];
          fs->getFileName(fileIndex, name, PFILENAME_SIZE);
          if (strcmp(name, "..") == 0) {
            NavigateToParent();
          } else {
            NavigateToDirectory(name);
          }
        }
      }
      pendingDirEnterOnRelease_ = false;
      return true;
    }

    // Check if play key was released
    if (playKeyHeld_ && !(mask & BM_PLAY)) {
      playKeyHeld_ = false;
      if (Player::GetInstance()->IsPlaying()) {
        Player::GetInstance()->StopStreaming();
        previewPlayingIndex_ = (size_t)-1;
      }
      return true;
    }

    // Check if edit key was released
    if (editKeyHeld_ && !(mask & BM_EDIT)) {
      editKeyHeld_ = false;
      isDirty_ = true; // Mark view as dirty to force redraw
      return true;
    }
  }

  // Handle key press events
  if (pressed) {
    auto fs = GetFileSystem();
    const bool hasFiles = !IsEmpty();

    // EDIT+LEFT: go to parent directory
    if (mask == (BM_EDIT | BM_LEFT) && !AtLocalRoot()) {
      NavigateToParent();
      isDirty_ = true;
      return true;
    }

    if (mask & BM_EDIT) {
      editKeyHeld_ = true;
    }

    if (mask & BM_PLAY) {
      if (!hasFiles) {
        return false;
      }
      unsigned fileIndex = GetFileList()[GetCurrentIndex()];
      char name[PFILENAME_SIZE];
      fs->getFileName(fileIndex, name, PFILENAME_SIZE);

      // Set flag to track that play key is being held
      playKeyHeld_ = true;

      if (mask & BM_ALT) {
        Trace::Log("IMPORT", "SHIFT play - import");
        import();
      } else {
        Trace::Log("IMPORT", "play key pressed - start preview");
        preview(name);
      }
      return true;
    }

    if (mask & BM_NAV && mask & BM_EDIT) {
      // toggle from sdcard "import sample" & project pool listing
      if (inProjectSampleDir_) {
        inProjectSampleDir_ = false;
        NavigateToDirectory(SAMPLES_LIB_DIR);
      } else {
        inProjectSampleDir_ = true;
        NavigateToDirectory(PROJECT_SAMPLES_DIR);
      }
      selectedButton_ = 0;
      return true;
    }

    if (mask & BM_ENTER) {
      if (!enterKeyHeld_) {
        enterKeyHeld_ = true;
        pendingDirEnterOnRelease_ = true;
      }
      if (mask & (BM_LEFT | BM_RIGHT | BM_UP | BM_DOWN)) {
        pendingDirEnterOnRelease_ = false;
      }

      if (inProjectSampleDir_) {
        if (selectedButton_ == 2) { // Volume
          int volumeOffset = 0;
          if (mask & BM_LEFT) {
            volumeOffset -= 1;
          }
          if (mask & BM_RIGHT) {
            volumeOffset += 1;
          }
          if (mask & BM_DOWN) {
            volumeOffset -= 5;
          }
          if (mask & BM_UP) {
            volumeOffset += 5;
          }
          if (volumeOffset != 0) {
            adjustPreviewVolume(volumeOffset);
          }
          return true;
        }
        if (!hasFiles) {
          pendingDirEnterOnRelease_ = false;
          return false;
        }
        if (selectedButton_ == 0) { // Edit
          unsigned fileIndex = GetFileList()[GetCurrentIndex()];
          char name[PFILENAME_SIZE];
          fs->getFileName(fileIndex, name, PFILENAME_SIZE);
          // showSampleEditor would be called here
        }
        return true;
      }

      if (selectedButton_ == 2) { // Volume
        int volumeOffset = 0;
        if (mask & BM_LEFT) {
          volumeOffset -= 1;
        }
        if (mask & BM_RIGHT) {
          volumeOffset += 1;
        }
        if (mask & BM_DOWN) {
          volumeOffset -= 5;
        }
        if (mask & BM_UP) {
          volumeOffset += 5;
        }
        if (volumeOffset != 0) {
          adjustPreviewVolume(volumeOffset);
        }
        return true;
      }
      if (!hasFiles) {
        pendingDirEnterOnRelease_ = false;
        return false;
      }
      unsigned fileIndex = GetFileList()[GetCurrentIndex()];
      // we can't import or edit dirs!
      if (fs->getFileType(fileIndex) != PFT_DIR) {
        if (selectedButton_ == 0) { // Import
          import();
        } else if (selectedButton_ == 1) { // Edit
          char name[PFILENAME_SIZE];
          fs->getFileName(fileIndex, name, PFILENAME_SIZE);
          // showSampleEditor would be called here
        }
      }
      return true;
    }

    // handle changing selected "bottom button"
    if ((mask & BM_LEFT || mask & BM_RIGHT) && !(mask & BM_NAV)) {
      if (inProjectSampleDir_ && IsEmpty()) {
        return false;
      }
      uint8_t buttonCount = (uint8_t)(inProjectSampleDir_ ? 3 : 3);
      if (mask & BM_LEFT) {
        selectedButton_ = (selectedButton_ + buttonCount - 1) % buttonCount;
      } else {
        selectedButton_ = (selectedButton_ + 1) % buttonCount;
      }
      isDirty_ = true;
      return true;
    }
  }

  return false;
}

void SampleImportView::OnItemSelected(const char *filename) {
  // For now, just import the sample
  // This can be customized later
  Trace::Log("SAMPLEIMPORT", "Selected: %s", filename);
}

void SampleImportView::warpToNextSample(bool goUp) {
  if (IsEmpty()) {
    return;
  }

  if (goUp) {
    if (GetCurrentIndex() > 0) {
      SetCurrentIndex(GetCurrentIndex() - 1);
    }
  } else {
    if (GetCurrentIndex() < GetItemCount() - 1) {
      SetCurrentIndex(GetCurrentIndex() + 1);
    }
  }
}

void SampleImportView::import() {
  if (IsEmpty()) {
    return;
  }

  auto fs = GetFileSystem();
  unsigned fileIndex = GetFileList()[GetCurrentIndex()];
  char name[PFILENAME_SIZE];
  fs->getFileName(fileIndex, name, PFILENAME_SIZE);

  Trace::Log("SAMPLEIMPORT", "Importing sample: %s", name);
  
  // Get project name for import
  StringWatchedVariable<MAX_PROJECT_NAME_LENGTH> &projectNameVar = 
    const_cast<StringWatchedVariable<MAX_PROJECT_NAME_LENGTH>&>(viewData_->project_->GetProjectNameString());
  etl::string<MAX_PROJECT_NAME_LENGTH> projectName = projectNameVar.GetString();
  
  // Import the sample using SamplePool
  int result = SamplePool::GetInstance()->ImportSample(name, projectName.c_str());
  if (result >= 0) {
    Trace::Log("SAMPLEIMPORT", "Sample imported successfully at index: %d", result);
  } else {
    Trace::Error("SAMPLEIMPORT", "Failed to import sample, error code: %d", result);
  }
}

void SampleImportView::preview(char *name) {
  Trace::Log("SAMPLEIMPORT", "Previewing sample: %s", name);
  
  // Stop any currently playing preview
  if (Player::GetInstance()->IsPlaying()) {
    Player::GetInstance()->StopStreaming();
  }
  
  // Start preview using Player's StartStreaming method
  Player::GetInstance()->StartStreaming(name);
  previewPlayingIndex_ = GetCurrentIndex();
}

void SampleImportView::adjustPreviewVolume(int offset) {
  Variable *v = viewData_->project_->FindVariable(FourCC::VarPreviewVolume);
  if (v) {
    int newVolume = v->GetInt() + offset;
    if (newVolume < 0) newVolume = 0;
    if (newVolume > 100) newVolume = 100;
    v->SetInt(newVolume);
    Trace::Log("SAMPLEIMPORT", "Preview volume adjusted to: %d", newVolume);
  }
}

void SampleImportView::showSampleEditor(etl::string<MAX_INSTRUMENT_FILENAME_LENGTH> filename, bool isProjectSample) {
  Trace::Log("SAMPLEIMPORT", "Opening sample editor for: %s", filename.c_str());
  
  // Navigate to the sample editor view using ScreenView's Navigate method
  // The SampleEditorView will handle loading the sample from the current directory
  Navigate(VT_SAMPLE_EDITOR);
}

void SampleImportView::onConfirmRemoveProjectSample(View &view, ModalView &dialog) {
  Trace::Log("SAMPLEIMPORT", "Confirm remove project sample");
  // Remove logic would go here
}