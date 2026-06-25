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

#include "ImportView.h"
#include "Application/Audio/AudioFileStreamer.h"
#include "Application/Instruments/SampleInstrument.h"
#include "Application/Instruments/SamplePool.h"
#include "Application/Views/SampleEditorView.h"
#include "Externals/etl/include/etl/string.h"
#include "Externals/etl/include/etl/to_string.h"
#include "ModalDialogs/MessageBox.h"
#include "System/FileSystem/FileSystem.h"
#include "ViewUtils.h"
#include <memory>
#include <nanoprintf.h>

// -4 to allow for title, filesize & spacers
#define LIST_PAGE_SIZE SCREEN_HEIGHT - 5

// is single cycle macro, checks for FILE size of LGPT and AKWF file formats
// AKWF "nes" pack 1376, AKWF "standard" 1344, LGPT pack 300
#define IS_SINGLE_CYCLE(x) (x == 1376 || x == 1344 || x == 300)

// Initialize static member
ViewType ImportView::sourceViewType_ = VT_PROJECT;

namespace {
enum ImportButton : uint8_t {
  kImportButtonImport = 0,
  kImportButtonEdit,
  kImportButtonVolume,
  kImportButtonCount,
};

const int kProjectButtonEdit = 0;
const int kProjectButtonRemove = 1;
const int kProjectButtonVolume = 2;
const int kProjectPoolButtonCount = 3;

} // namespace

ImportView::ImportView(GUIWindow &w, ViewData *viewData) : ScreenView(w, viewData) {
}

ImportView::~ImportView() {
}

void ImportView::Reset() {
  topIndex_ = 0;
  currentIndex_ = 0;
  previewPlayingIndex_ = 0;
  selectedButton_ = 0;
  toInstr_ = 0;
  dirIndexStack_.clear();
  playKeyHeld_ = false;
  editKeyHeld_ = false;
  enterKeyHeld_ = false;
  pendingDirEnterOnRelease_ = false;
  inProjectSampleDir_ = false;
  fileIndexList_.clear();
}

// Static method to set the source view type before opening ImportView
void ImportView::SetSourceViewType(ViewType vt) {
  sourceViewType_ = vt;
}

const char *ImportView::emptyStateMessage() const {
  return "Sample pool is empty.";
}

void ImportView::ProcessButtonMask(uint16_t mask, bool pressed) {
  // Check for key release events
  if (!pressed) {
    // Open selected directory only when ENTER is released, unless ENTER was
    // consumed by another action (e.g. volume edit).
    if (enterKeyHeld_ && !(mask & BM_ENTER)) {
      enterKeyHeld_ = false;
      if (pendingDirEnterOnRelease_ && !fileIndexList_.empty()) {
        auto fs = FileSystem::GetInstance();
        unsigned fileIndex = fileIndexList_[currentIndex_];
        if (fs->getFileType(fileIndex) == PFT_DIR) {
          char name[PFILENAME_SIZE];
          fs->getFileName(fileIndex, name, PFILENAME_SIZE);
          if (strcmp(name, "..") == 0) {
            goToParentDirectory(fs);
          } else {
            enterDirectory(fs, name);
          }
          isDirty_ = true;
        }
      }
      pendingDirEnterOnRelease_ = false;
      return;
    }

    // Check if play key was released
    if (playKeyHeld_ && !(mask & BM_PLAY)) {
      // Play key no longer pressed so should stop playback
      playKeyHeld_ = false;
      if (Player::GetInstance()->IsPlaying()) {
        Player::GetInstance()->StopStreaming();
        previewPlayingIndex_ = (size_t)-1;
      }
      return;
    }

    // Check if edit key was released
    if (editKeyHeld_ && !(mask & BM_EDIT)) {
      // Edit key no longer pressed, redraw the view to clear status message
      editKeyHeld_ = false;
      isDirty_ = true; // Mark view as dirty to force redraw
      return;
    }
  }

  // Handle key press events
  if (pressed) {
    auto fs = FileSystem::GetInstance();
    const bool hasFiles = !fileIndexList_.empty();

    // EDIT+LEFT: go to parent directory within the import file browser.
    // NAV+LEFT remains reserved for leaving the ImportView entirely.
    if (mask == (BM_EDIT | BM_LEFT) && !atLocalRoot_) {
      goToParentDirectory(fs);
      isDirty_ = true;
      return;
    }

    if (mask & BM_EDIT) {
      // Set flag to track that edit key is being held
      editKeyHeld_ = true;
    }

    if (mask & BM_PLAY) {
      if (!hasFiles) {
        return;
      }
      unsigned fileIndex = fileIndexList_[currentIndex_];
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
      return; // We've handled the play button, so return
    }

    if (mask & BM_NAV && mask & BM_EDIT) {
      // toggle from sdcard "import sample" & project pool listing
      if (inProjectSampleDir_) {
        inProjectSampleDir_ = false;
        jumpToDirectory(fs, SAMPLES_LIB_DIR);
      } else {
        inProjectSampleDir_ = true;
        jumpToDirectory(fs, PROJECT_SAMPLES_DIR);
      }
      selectedButton_ = 0;
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
        if (selectedButton_ == kProjectButtonVolume) {
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
          return;
        }
        if (!hasFiles) {
          pendingDirEnterOnRelease_ = false;
          return; // Do nothing if the list is empty
        }
        if (selectedButton_ == kProjectButtonEdit) {
          unsigned fileIndex = fileIndexList_[currentIndex_];
          char name[PFILENAME_SIZE];
          fs->getFileName(fileIndex, name, PFILENAME_SIZE);
          showSampleEditor(name, true);
        } else if (selectedButton_ == kProjectButtonRemove) {
          // note yet supported on pico
        }
        return;
      }
      if (selectedButton_ == kImportButtonVolume) {
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
        return;
      }
      if (!hasFiles) {
        pendingDirEnterOnRelease_ = false;
        return;
      }
      unsigned fileIndex = fileIndexList_[currentIndex_];
      // we can't import or edit dirs!
      if (fs->getFileType(fileIndex) != PFT_DIR) {
        if (selectedButton_ == kImportButtonImport) {
          import();
        } else if (selectedButton_ == kImportButtonEdit) {
          char name[PFILENAME_SIZE];
          fs->getFileName(fileIndex, name, PFILENAME_SIZE);
          showSampleEditor(name, false);
        }
      }
    }

    // handle changing selected "bottom button", note: ignore if this is a
    // nav+arrow combo
    if ((mask & BM_LEFT || mask & BM_RIGHT) && !(mask & BM_NAV)) {
      if (inProjectSampleDir_ && fileIndexList_.empty()) {
        return; // Do nothing if the list is empty
      }
      uint8_t buttonCount = inProjectSampleDir_ ? static_cast<uint8_t>(kProjectPoolButtonCount)
                                                : static_cast<uint8_t>(kImportButtonCount);
      if (mask & BM_LEFT) {
        selectedButton_ = (selectedButton_ + buttonCount - 1) % buttonCount;
      } else {
        selectedButton_ = (selectedButton_ + 1) % buttonCount;
      }
      DrawView();
    }
  }

  // Only process other buttons when pressed
  if (!pressed)
    return;

  // handle moving up and down the file list
  if (mask & BM_UP) {
    if (inProjectSampleDir_ && fileIndexList_.empty()) {
      return; // Do nothing if the list is empty
    }
    warpToNextSample(true);
  } else if (mask & BM_DOWN) {
    if (inProjectSampleDir_ && fileIndexList_.empty()) {
      return; // Do nothing if the list is empty
    }
    warpToNextSample(false);
  } else if ((mask & BM_LEFT) && (mask & BM_NAV)) {
    // clear this flag on leaving this screen
    viewData_->isShowingSampleEditorProjectPool = false;

    // Go back to the source view that opened the ImportView
    Navigate(sourceViewType_);
  } else {
    // A modifier
  }
}

void ImportView::DrawBottomBar() {
  int y = SCREEN_HEIGHT - 2;
  int x = 0;

  // ensure selectedButton is valid (TODO: how does it even get set to something invalid...)
  uint8_t buttonCount = (uint8_t)(inProjectSampleDir_ ? kProjectPoolButtonCount : kImportButtonCount);
  if (selectedButton_ < 0 || selectedButton_ >= buttonCount) {
    selectedButton_ = 0;
  }

  int previewVolume = 0;
  Variable *v = viewData_->project_->FindVariable(FourCC::VarPreviewVolume);
  if (v) {
    previewVolume = v->GetInt();
  }

  if (!inProjectSampleDir_) {
    DrawButton(x, y, "Import", selectedButton_ == kImportButtonImport);
    DrawButton(x + 8, y, "Edit", selectedButton_ == kImportButtonEdit);
  } else {
    DrawButton(x, y, "Edit", selectedButton_ == 0);
    DrawButton(x + 6, y, "N/A", selectedButton_ == 1); // todo: make remove available or remove the button
  }

  char volField[12];
  npf_snprintf(volField, sizeof(volField), "Vol:%2d", previewVolume);
  DrawButton(x + 24, y, volField, selectedButton_ == kImportButtonVolume);

  // draw current selected file size and available storage indicator
  auto fs = FileSystem::GetInstance();
  uint32_t availableSpace = SamplePool::GetInstance()->GetAvailableSampleStorageSpace();
  y = 0;

  SetColor(Theme::View::fg);
  uint32_t filesize = 0;
  if (!fileIndexList_.empty()) {
    auto currentFileIndex = fileIndexList_[currentIndex_];
    // only get file size if it's a file not a dir
    if (fs->getFileType(currentFileIndex) == PFT_FILE) {
      filesize = fs->getFileSize(currentFileIndex);
      // if file size is larger than available space, set color to warning
      if (filesize > availableSpace) {
        SetColor(Theme::View::warning);
      }
    }
  }

  // Create a temporary buffer for formatting
  char buffer[SCREEN_WIDTH];
  buffer[SCREEN_WIDTH - 1] = 0;

  npf_snprintf(buffer, sizeof(buffer), "Size: %i/%i", filesize, availableSpace);

  // pad status line buffer with trailing space chars to ensure the invert
  // color is applied to entire line
  int32_t padWidth = (SCREEN_WIDTH - 2) - static_cast<int32_t>(strlen(buffer));
  if (padWidth < 0) {
    padWidth = 0;
  }
  npf_snprintf(buffer, sizeof(buffer), "%s%*s", buffer, padWidth, " ");

  x = 1;  // align with rest screen title & file list
  y = 23; // bottom line
  SetBackgroundColor(Theme::View::bg);
  SetColor(Theme::View::fg);
  DrawString(x, y, buffer);
}

void ImportView::DrawView() {
  const int listWidth = SCREEN_WIDTH - 5; // spacing, scrollbar, file indicator, usage indicator, space

  Clear();

  auto fs = FileSystem::GetInstance();

  // Draw title with available storage space
  DrawTitle(char_back_s " %s", inProjectSampleDir_ ? "Project Pool" : "Import Sample");

  if (fileIndexList_.empty()) {
    drawEmptyState();
    return;
  }

  // ensure selected item is in visible range
  const size_t pageSize = LIST_PAGE_SIZE;
  if (currentIndex_ < topIndex_) {
    topIndex_ = currentIndex_;
  } else if (currentIndex_ >= topIndex_ + pageSize) {
    topIndex_ = currentIndex_ - pageSize + 1;
  }

  // Draw samples
  int x = 1;
  int y = 2;

  // Loop through visible files in the list
  for (size_t i = topIndex_; i < topIndex_ + LIST_PAGE_SIZE && (i < fileIndexList_.size()); i++) {
    uint32_t fileIndex = fileIndexList_[i];
    bool isDir = fs->getFileType(fileIndex) == PFT_DIR;
    bool isSelected = (i == currentIndex_);

    if (isSelected) {
      // draw selection ends
      SetColor(Theme::View::Selection::bg);
      SetBackgroundColor(Theme::View::bg);
      DrawString(0, y, char_button_border_left_s);
      DrawString(SCREEN_WIDTH - 2, y, char_button_border_right_s);

      SetBackgroundColor(Theme::View::Selection::bg);
    } else {
      SetBackgroundColor(Theme::View::bg);
    }

    // get filename
    etl::string<SCREEN_WIDTH> displayName;
    char buffer[SCREEN_WIDTH];
    fs->getFileName(fileIndex, buffer, SCREEN_WIDTH);
    displayName += buffer;

    if (isDir) {
      SetColor(isSelected ? Theme::View::Selection::fg : Theme::FileList::directory);
      bool upDir = strcmp(buffer, "..") == 0;
      const char prefix[3] = {GLYPH(upDir ? char_symbol_return_s : char_file_folder_s), ' ', 0};
      DrawString(x, y, prefix);
    } else {
      SetColor(isSelected ? Theme::View::Selection::fg : Theme::FileList::file);
      // regular file, check the fype
      int filesize = fs->getFileSize(fileIndex);
      bool isSingleCycle = IS_SINGLE_CYCLE(filesize);
      const char fileTypeSymbol = GLYPH(isSingleCycle ? char_file_cycle_s : char_file_file_s);

      // Format the display name with appropriate prefix
      const bool inUse = (inProjectSampleDir_ && viewData_->project_->SampleInUse(buffer));
      const char usageSymbol = GLYPH(inUse ? char_symbol_indicatorFull_s : " ");
      const char prefix[3] = {fileTypeSymbol, usageSymbol, 0};
      DrawString(x, y, prefix);
    }

    // truncate filename to fit display width
    while (displayName.size() < listWidth) {
      displayName += " ";
    }

    if (displayName.size() > listWidth) {
      displayName.resize(listWidth - 1);
      displayName += char_indicator_ellipsis_s;
    }

    DrawString(x + 2, y, displayName.c_str());
    y += 1;
  }

  // draw bottom menu
  DrawBottomBar();

  // draw scroll bar
  drawScrollBar(SCREEN_WIDTH - 1, 2, pageSize, topIndex_, fileIndexList_.size());
}

void ImportView::OnPlayerUpdate(PlayerEventType, unsigned int tick) {};

void ImportView::OnFocus() {
  // clear stale flags
  enterKeyHeld_ = false;
  pendingDirEnterOnRelease_ = false;
  editKeyHeld_ = false;
  playKeyHeld_ = false;

  auto fs = FileSystem::GetInstance();

  toInstr_ = viewData_->currentInstrumentID_;

  inProjectSampleDir_ = viewData_->isShowingSampleEditorProjectPool;

  if (inProjectSampleDir_) {
    goProjectSamplesDir(viewData_);
    jumpToDirectory(fs, ".");
  } else {
    jumpToDirectory(fs, viewData_->importViewStartDir);
  }
}

void ImportView::warpToNextSample(bool goUp) {
  if (goUp) {
    if (currentIndex_ > 0) {
      currentIndex_--;
      // if we have scrolled off the top, page the file list up if not
      // already at  very top of the list
      if (currentIndex_ < topIndex_) {
        topIndex_ = currentIndex_;
      }
    }
  } else {
    if (currentIndex_ < fileIndexList_.size() - 1) {
      currentIndex_++;
      // if we have scrolled off the bottom, page the file list down if not
      // at end of the list
      if (currentIndex_ >= (topIndex_ + LIST_PAGE_SIZE)) {
        topIndex_++;
      }
    }
  }
  isDirty_ = true;
}

void ImportView::preview(char *name) {
  auto fs = FileSystem::GetInstance();
  unsigned fileIndex = fileIndexList_[currentIndex_];

  // do not preview directories
  if (fs->getFileType(fileIndex) == PFT_DIR) {
    return;
  }

  // Get file size to check if it's a single cycle waveform
  int fileSize = fs->getFileSize(fileIndex);

  // check for LGPT or AKWF standard file sizes
  bool isSingleCycle = IS_SINGLE_CYCLE(fileSize);

  // If something is already playing, stop it first
  if (Player::GetInstance()->IsPlaying()) {
    Player::GetInstance()->StopStreaming();
  }

  WavFile wav;
  auto wavRes = wav.Open(name);
  MessageBox *mb = nullptr;
  if (!wavRes) {
    auto error = wavRes.error();
    switch (error) {
      case INVALID_FILE:
        mb = MessageBox::Create(*this, "Preview Failed", "Could not open file", MBBF_OK);
        break;
      case UNSUPPORTED_FILE_FORMAT:
      case INVALID_HEADER:
      case UNSUPPORTED_WAV_FORMAT:
        mb = MessageBox::Create(*this, "Preview Failed", "Invalid file", MBBF_OK);
        break;
      case UNSUPPORTED_AUDIO_FORMAT:
      case UNSUPPORTED_BITDEPTH:
      case UNSUPPORTED_SAMPLERATE:
        mb = MessageBox::Create(*this, "Preview Failed", "Unsupported format", MBBF_OK);
        break;
    }
  } else {
    wav.Close();
  }

  if (mb != nullptr) {
    DoModal(mb);
    return;
  }

  // Start playing the selected sample
  Trace::Debug("Starting preview of %s (single cycle: %d)", name, isSingleCycle);
  previewPlayingIndex_ = currentIndex_;

  // Use looping for single cycle waveforms
  if (isSingleCycle) {
    Trace::Debug("Looping single cycle waveform: %s (size: %d bytes)", name, fileSize);
    Player::GetInstance()->StartLoopingStreaming(name);
  } else {
    Player::GetInstance()->StartStreaming(name);
  }
}

void ImportView::import() {
  // stop playing before trying to import
  if (Player::GetInstance()->IsPlaying()) {
    MessageBox *mb = MessageBox::Create(*this, "Can't import while previewing", MBBF_OK);
    DoModal(mb);
    return;
  }

  auto fs = FileSystem::GetInstance();
  char name[PFILENAME_SIZE];
  unsigned fileIndex = fileIndexList_[currentIndex_];
  fs->getFileName(fileIndex, name, PFILENAME_SIZE);

  // Get current project name
  char projName[MAX_PROJECT_NAME_LENGTH + 1];
  viewData_->project_->GetProjectName(projName);

  // Check if we're in the project's sample directory
  if (inProjectSampleDir_) {
    MessageBox *mb = MessageBox::Create(*this, "Can't import from project!", MBBF_OK);
    DoModal(mb);
    return;
  }

  SamplePool *pool = SamplePool::GetInstance();

  // Check if we've reached the maximum number of samples
  int currentCount = pool->GetNameListSize();
  if (currentCount >= MAX_SAMPLES) {
    // Show error dialog to inform the user
    char message[SCREEN_WIDTH];
    npf_snprintf(message, sizeof(message), "Maximum of %d samples reached", MAX_SAMPLES);
    // pad with trailing spaces as dialog size based on title
    MessageBox *mb = MessageBox::Create(*this, "Cannot Import Sample      ", message, MBBF_OK);
    DoModal(mb);
    return;
  }

  // Check if the sample would exceed available flash storage
  int fileSize = fs->getFileSize(fileIndex);

  // Check if the sample would fit in available storage
  if (!pool->CheckSampleFits(fileSize)) {
    // Get available flash space for the message
    uint32_t availableFlash = SamplePool::GetInstance()->GetAvailableSampleStorageSpace();

    // Show error dialog to inform the user
    char message[SCREEN_WIDTH];

    uint32_t availBytes = availableFlash;
    npf_snprintf(message, sizeof(message), "Only %d bytes free", availBytes);
    // pad with trailing spaces as dialog width based on title length
    MessageBox *mb = MessageBox::Create(*this, "Sample Too Large       ", message, MBBF_OK);
    DoModal(mb);
    return;
  }

  // Check if wave file is in a supported format
  WavFile wav;
  auto wavRes = wav.Open(name);
  MessageBox *mb = nullptr;
  if (!wavRes) {
    auto error = wavRes.error();
    switch (error) {
      case INVALID_FILE:
        mb = MessageBox::Create(*this, "Import Failed", "Could not open file", MBBF_OK);
        break;
      case UNSUPPORTED_FILE_FORMAT:
      case INVALID_HEADER:
      case UNSUPPORTED_WAV_FORMAT:
        mb = MessageBox::Create(*this, "Import Failed", "invalid file", MBBF_OK);
        break;
      case UNSUPPORTED_AUDIO_FORMAT:
      case UNSUPPORTED_BITDEPTH:
      case UNSUPPORTED_SAMPLERATE:
        mb = MessageBox::Create(*this, "Import Failed", "unsupported format", MBBF_OK);
        break;
    }
  } else {
    wav.Close();
  }

  if (mb != nullptr) {
    DoModal(mb);
    return;
  }

  int sampleID = pool->ImportSample(name, projName);

  if (sampleID >= 0) {
    I_Instrument *instr = viewData_->project_->GetInstrumentBank()->GetInstrument(toInstr_);
    if (instr->GetType() == IT_SAMPLE) {
      SampleInstrument *sinstr = (SampleInstrument *)instr;
      sinstr->AssignSample(sampleID);
      sinstr->ClearSlices();
    };

    // check if we had to truncate filename
    size_t nameLength = strlen(name);
    if (nameLength > MAX_INSTRUMENT_FILENAME_LENGTH) {
      Trace::Log("IMPORT", "Filename too long: %s (%zu chars, max is %d)", name, nameLength,
                 MAX_INSTRUMENT_FILENAME_LENGTH);
      MessageBox *mb = MessageBox::Create(*this, "Sample name Truncated!", "Max filename length:24", MBBF_OK);
      DoModal(mb);
    }
  } else {
    Trace::Error("failed to import sample");
    // Show a generic error message if import failed for other reasons
    MessageBox *mb = MessageBox::Create(*this, "Import Failed", "Could not import sample", MBBF_OK);
    DoModal(mb);
  };
  isDirty_ = true;
}

void ImportView::adjustPreviewVolume(int offset) {
  // Get the project instance
  Project *project = viewData_->project_;

  // Find the preview volume variable
  Variable *v = project->FindVariable(FourCC::VarPreviewVolume);
  if (!v) {
    Status::Set("Preview volume setting not found");
    return;
  }

  int newVolume = v->GetInt() + offset;
  v->SetInt(newVolume > 99 ? 99 : (newVolume < 0 ? 0 : newVolume));

  // Mark the view as dirty to update the status bar with the new volume
  isDirty_ = true;
}

bool ImportView::changeDirectory(FileSystem *fs, const char *name) {
  if (!fs->chdir(name)) {
    Trace::Error("FAILED to chdir to %s", name);
    return false;
  }

  // being in the project samples dir and the samples root means no going up
  atLocalRoot_ = (dirIndexStack_.size() == 0) || inProjectSampleDir_;


  refreshFileIndexList(fs);

  return true;
}

void ImportView::enterDirectory(FileSystem *fs, const char *name) {
  char projName[MAX_PROJECT_NAME_LENGTH + 1];
  viewData_->project_->GetProjectName(projName);

  if (strcmp(projName, name) == 0) {
    jumpToDirectory(fs, PROJECTS_DIR);

    Trace::Log("IMPORT", "NOT allowed to browse into current project sample directory");
    return;
  }

  if (!changeDirectory(fs, name)) {
    return;
  }

  jumpToDirectory(fs, ".", true);
}

void ImportView::goToParentDirectory(FileSystem *fs) {
  if (!changeDirectory(fs, "..")) {
    return;
  }

  currentIndex_ = dirIndexStack_.empty() ? 0 : dirIndexStack_.top();
  if (!dirIndexStack_.empty()) {
    dirIndexStack_.pop();
  }

  refreshFileIndexList(fs);
}

void ImportView::jumpToDirectory(FileSystem *fs, const char *name, bool pushToStack) {
  if (!changeDirectory(fs, name)) {
    return;
  }

  if (pushToStack) {
    if (dirIndexStack_.full()) {
      Trace::Error("ImportView directory stack overflow at depth %d", DirectoryIndexStackDepth);
      char message[SCREEN_WIDTH];
      npf_snprintf(message, sizeof(message), "Max depth is %d", DirectoryIndexStackDepth);
      MessageBox *mb = MessageBox::Create(*this, "Can't enter folder", message, MBBF_OK);
      DoModal(mb);
      return;
    }
    dirIndexStack_.push(static_cast<uint8_t>(currentIndex_));
  } else {
    dirIndexStack_.clear();
  }
  topIndex_ = 0;
  currentIndex_ = 0;

  refreshFileIndexList(fs);
}

void ImportView::showSampleEditor(etl::string<MAX_INSTRUMENT_FILENAME_LENGTH> filename, bool isProjectSample) {
  viewData_->sampleEditorFilename = filename;
  viewData_->isShowingSampleEditorProjectPool = isProjectSample;

  // before going to sample editor set this view as its "source" view
  SampleEditorView::sourceViewType_ = VT_IMPORT;

  // Switch to the SampleEditorView
  Navigate(VT_SAMPLE_EDITOR);
}

void ImportView::removeProjectSample(uint8_t fileIndex, FileSystem *fs) {
  char filename[PFILENAME_SIZE];
  fs->getFileName(fileIndex, filename, PFILENAME_SIZE);

  // first check if a instrument uses this sample
  bool inUse = viewData_->project_->SampleInUse(etl::string<MAX_INSTRUMENT_FILENAME_LENGTH>(filename));

  if (inUse) {
    MessageBox *mb = MessageBox::Create(*this, "Cannot remove", "Sample in use!", MBBF_OK);
    DoModal(mb);
    return;
  }

  // add spacing for basic way to size dialog wider to give Ok/cancel
  // buttons between space
  MessageBox *mb = MessageBox::Create(*this, "    Remove sample?    ", filename, MBBF_OK | MBBF_CANCEL);
  pendingDeleteFs_ = fs;
  strncpy(pendingDeleteFilename_, filename, PFILENAME_SIZE - 1);
  pendingDeleteFilename_[PFILENAME_SIZE - 1] = '\0';
  DoModal(mb, ModalViewCallback::create<ImportView, &ImportView::onConfirmRemoveProjectSample>(*this));
}

void ImportView::onConfirmRemoveProjectSample(View &, ModalView &dialog) {
  FileSystem *fs = pendingDeleteFs_;
  pendingDeleteFs_ = nullptr;

  char filename[PFILENAME_SIZE];
  strncpy(filename, pendingDeleteFilename_, PFILENAME_SIZE - 1);
  filename[PFILENAME_SIZE - 1] = '\0';
  pendingDeleteFilename_[0] = '\0';

  if (dialog.GetReturnCode() != MBL_OK || !fs) {
    return;
  }

  int sampleIndex = SamplePool::GetInstance()->FindSampleIndexByName(filename);
  if (sampleIndex < 0) {
    Trace::Error("Failed to map sample %s to pool index", filename);
    return;
  }

  if (!fs->DeleteFile(filename)) {
    Trace::Error("Failed to delete sample %s", filename);
    return;
  }

  SamplePool::GetInstance()->unloadSample(sampleIndex);

  if (currentIndex_ > 0) {
    --currentIndex_;
  }

  refreshFileIndexList(fs);
  isDirty_ = true;
}

void ImportView::refreshFileIndexList(FileSystem *fs) {
  fs->list(&fileIndexList_, ".wav", loDefault);

  // remove .. from sample root dir to prevent leaving
  if (atLocalRoot_) {
    for (auto it = fileIndexList_.begin(); it != fileIndexList_.end(); ++it) {
      char entryName[PFILENAME_SIZE];
      fs->getFileName(*it, entryName, PFILENAME_SIZE);
      if (strcmp(entryName, "..") == 0) {
        fileIndexList_.erase(it);
        break;
      }
    }
  }

  if (fileIndexList_.empty()) {
    topIndex_ = 0;
    currentIndex_ = 0;
  } else if (currentIndex_ >= fileIndexList_.size()) {
    currentIndex_ = fileIndexList_.size() - 1;
  }
}
