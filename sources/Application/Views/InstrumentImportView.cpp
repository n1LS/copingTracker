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

#include "InstrumentImportView.h"
#include "Application/AppWindow.h"
#include "Application/Persistency/PersistenceConstants.h"
#include "Application/Persistency/PersistencyService.h"
#include "Application/Views/ModalDialogs/MessageBox.h"
#include "System/Console/Trace.h"
#include <memory>
#include <nanoprintf.h>

// Configuration for the FileListView base class
static const FileListConfig kInstrumentImportConfig{.title = "Import Instrument",
                                                    .startDirectory = INSTRUMENTS_DIR,
                                                    .fileExtension = INSTRUMENT_FILE_EXTENSION,
                                                    .listFlags = loFiles,
                                                    .backNavigationTarget = VT_INSTRUMENT,
                                                    .pageSize = SCREEN_HEIGHT - 4,
                                                    .allowDirectoryNavigation = true,
                                                    .showDirectories = true,
                                                    .actionTabs = {}, // No tabs - ENTER directly imports
                                                    .allowTabSelection = false};

InstrumentImportView::InstrumentImportView(GUIWindow &w, ViewData *viewData)
    : FileListView(w, viewData, kInstrumentImportConfig) {
  // Store the current instrument ID
  toInstrID_ = viewData_->currentInstrumentID_;
}

InstrumentImportView::~InstrumentImportView() {
}

void InstrumentImportView::Reset() {
  toInstrID_ = 0;
  instrumentTypeList_.clear();
}

const char *InstrumentImportView::GetEmptyStateMessage() const {
  return "No instruments to show";
}

void InstrumentImportView::OnDirectorySetup() {
  // Detect instrument types for all files in the current directory
  detectInstrumentTypes();
}

void InstrumentImportView::detectInstrumentTypes() {
  instrumentTypeList_.clear();

  auto fs = GetFileSystem();
  auto persistency = PersistencyService::GetInstance();
  char filePath[PFILENAME_SIZE];

  // Pre-reserve space to avoid reallocation issues
  instrumentTypeList_.reserve(GetItemCount());

  for (size_t i = 0; i < GetItemCount(); i++) {
    // For directories, use IT_NONE as placeholder
    if (IsDirectory(i)) {
      instrumentTypeList_.push_back(IT_NONE);
    } else {
      // Get the file path and detect instrument type
      GetFileName(i, filePath, PFILENAME_SIZE);
      InstrumentType type = persistency->DetectInstrumentType(filePath);
      instrumentTypeList_.push_back(type);
    }
  }

  Trace::Log("INSTRUMENTIMPORT", "Detected %zu instrument types", instrumentTypeList_.size());
}

void InstrumentImportView::PrepareItemDrawing(int index, bool isSelected, Color *fg, Color *bg, char *buffer) {
  auto fs = GetFileSystem();

  // Ensure instrumentTypeList_ is properly sized before accessing
  // This can happen if DrawView() is called before OnDirectorySetup() completes
  if (instrumentTypeList_.size() != GetItemCount()) {
    Trace::Log("INSTRUMENTIMPORT", "Re-syncing instrumentTypeList_: size=%zu, itemCount=%zu", instrumentTypeList_.size(), GetItemCount());
    detectInstrumentTypes();
  }

  // Get filename
  char temp[PFILENAME_SIZE];
  GetFileName(index, temp, PFILENAME_SIZE);

  // Copy to buffer first, then trim the file extension for display
  strncpy(buffer, temp, SCREEN_WIDTH);
  buffer[SCREEN_WIDTH - 1] = '\0';
  size_t len = strlen(buffer);
  size_t extLen = strlen(INSTRUMENT_FILE_EXTENSION);
  if (len > extLen) {
    buffer[len - extLen] = '\0';
  }

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
    symbol = (strcmp(buffer, "..") == 0) ? CHAR(char_symbol_return_s) : CHAR(char_file_folder_s);
    if (!isSelected) {
      *fg = Theme::FileList::directory;
    }
  } else {
    symbol = CHAR(char_file_instrument_s);
    if (!isSelected) {
      *fg = Theme::FileList::file;
    }
  }

  // Get instrument type prefix
  char typePrefix[5];
  strcpy(typePrefix, "    "); // Default to empty

  if (isDirectory) {
    // Directories don't have instrument type
  } else if (!instrumentTypeList_.empty() && index < static_cast<int>(instrumentTypeList_.size())) {
    // Get the instrument type for this file with bounds check
    InstrumentType type = instrumentTypeList_[index];
    if (type >= IT_NONE && type < IT_LAST) {
      strncpy(typePrefix, InstrumentTypeNames[type].compact, 4);
      typePrefix[4] = '\0';
    }
  }

  // Draw the item with symbol, filename, and type prefix
  npf_snprintf(buffer, SCREEN_WIDTH, "%c %-*s %s", symbol, SCREEN_WIDTH - 10, temp, typePrefix);
}

void InstrumentImportView::OnItemSelected(const char *filename) {
  importInstrument(filename);
}

void InstrumentImportView::warpToNextInstrument(bool goUp) {
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

void InstrumentImportView::importInstrument(const char *name) {
  // Check if the filename exceeds the maximum allowed length
  if (strlen(name) > MAX_INSTRUMENT_FILENAME_LENGTH) {
    Trace::Error("INSTRUMENTIMPORT: Instrument filename exceeds maximum length: %s (%zu > %d)", name, strlen(name),
                 MAX_INSTRUMENT_FILENAME_LENGTH);

    char sizeMesg[32];
    npf_snprintf(sizeMesg, sizeof(sizeMesg), "Max is %d chars", MAX_INSTRUMENT_FILENAME_LENGTH);
    MessageBox *mb = MessageBox::Create(*this, "Filename too long", sizeMesg, MBBF_OK);
    DoModal(mb);
    return;
  }

  // Get the current instrument bank
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();

  // assert bank is not null
  assert(bank);

  // use current instrument ID if importing a new instrument
  toInstrID_ = viewData_->currentInstrumentID_;

  // First detect the instrument type from the file
  InstrumentType importedType = PersistencyService::GetInstance()->DetectInstrumentType(name);

  if (importedType == IT_NONE) {
    MessageBox *mb = MessageBox::Create(*this, "Unknown instrument type", MBBF_OK);
    DoModal(mb);
    return;
  }

  Trace::Log("INSTRUMENTIMPORT", "Detected instrument type: %d", importedType);
  Trace::Log("INSTRUMENTIMPORT", "Importing instrument from file: %s", name);
  Trace::Log("INSTRUMENTIMPORT", "Target instrument ID: %d", toInstrID_);

  // Get the current instrument
  I_Instrument *currentInstrument = bank->GetInstrument(toInstrID_);

  if (!currentInstrument) {
    MessageBox *mb = MessageBox::Create(*this, "Invalid instrument", MBBF_OK);
    DoModal(mb);
    return;
  }

  // Log the current instrument type for debugging
  Trace::Log("INSTRUMENTIMPORT", "Current instrument type: %d, Imported type: %d", currentInstrument->GetType(),
             importedType);

  if (currentInstrument->GetType() != importedType) {
    Trace::Log("INSTRUMENTIMPORT", "Converting instrument from type %d to %d", currentInstrument->GetType(),
               importedType);

    // Delete the current instrument
    bank->releaseInstrument(toInstrID_);

    // Create a new instrument of the correct type in the same slot
    if (bank->AssignInstrumentToSlot(importedType, toInstrID_) != InstrumentAssignResult::Success) {
      MessageBox *mb = MessageBox::Create(*this, "Failed to create instrument", MBBF_OK);
      DoModal(mb);
      return;
    }

    Trace::Log("INSTRUMENTIMPORT", "Created new instrument of type %d in slot %d", importedType, toInstrID_);
  } else {
    Trace::Log("INSTRUMENTIMPORT", "Keeping existing instrument of type %d in slot %d", importedType, toInstrID_);
  }

  // Force the ViewData to update its current instrument type
  // This ensures the InstrumentView will show the correct instrument type
  // when we return to it
  Trace::Log("INSTRUMENTIMPORT", "Created new instrument of type %d", importedType);

  // Get the updated instrument (which may be a new one if we converted)
  I_Instrument *instrument = bank->GetInstrument(toInstrID_);

  // Import the instrument settings
  PersistencyResult result = PersistencyService::GetInstance()->ImportInstrument(instrument, name);

  // debug logging to show final instrument type
  Trace::Log("INSTRUMENTIMPORT", "Imported TYPE: %d", instrument->GetType());

  if (result == PERSIST_LOADED) {
    // Force a strong notification to ensure the UI updates
    // First mark the instrument as changed
    instrument->SetChanged();

    // Then notify all observers to update their state
    instrument->NotifyObservers();

    // Log the final state of the instrument for debugging
    Variable *channelVar = instrument->FindVariable(FourCC::MidiInstrumentChannel);
    if (channelVar) {
      Trace::Log("INSTRUMENTIMPORT", "Final MIDI channel: %d", channelVar->GetInt());
    }

    // Log the final instrument type for debugging
    Trace::Log("INSTRUMENTIMPORT", "Final instrument type in importInstrument: %d", instrument->GetType());

    // Update the current instrument ID in the view data
    // This ensures the InstrumentView will display the correct instrument when
    // we return
    viewData_->currentInstrumentID_ = toInstrID_;
    Trace::Log("INSTRUMENTIMPORT", "Updated viewData_ currentInstrumentID_ to: %d", toInstrID_);

    // Show success message and return to instrument view
    MessageBox *mb = MessageBox::Create(*this, "Import successful", MBBF_OK);
    DoModal(mb, ModalViewCallback::create<InstrumentImportView, &InstrumentImportView::onImportSuccess>(*this));
  } else {
    MessageBox *mb = MessageBox::Create(*this, "Import failed", MBBF_OK);
    DoModal(mb);
  }
}

void InstrumentImportView::onImportSuccess(View &, ModalView &dialog) {
  if (dialog.GetReturnCode() != MBL_OK) {
    return;
  }

  Trace::Log("INSTRUMENTIMPORT", "Switching back to instrument view with ID: %d", toInstrID_);

  I_Instrument *instrument = viewData_->project_->GetInstrumentBank()->GetInstrument(toInstrID_);
  if (instrument) {
    instrument->SetChanged();
    instrument->NotifyObservers();
  }

  Navigate(VT_INSTRUMENT);
}