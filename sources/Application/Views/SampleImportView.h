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

#ifndef _SAMPLE_IMPORT_VIEW_H_
#define _SAMPLE_IMPORT_VIEW_H_

#include "Application/Views/BaseClasses/FileListView.h"
#include "ViewData.h"

/**
 * SampleImportView - Migrated to use FileListView base class
 *
 * This view allows users to browse and import sample files.
 * Supports dual directory mode (project samples vs SD card library).
 */
class SampleImportView : public FileListView {
public:
  SampleImportView(GUIWindow &w, ViewData *viewData);
  ~SampleImportView();

  // Required FileListView overrides
  const char *GetEmptyStateMessage() const override;

  // File selection handler
  void OnItemSelected(const char *filename) override;

  // Custom methods
  void Reset() override;

  // Static method to set which view will open the SampleImportView
  static void SetSourceViewType(ViewType vt);

  // Track which view opened the SampleImportView (default to project view)
  static ViewType sourceViewType_;

protected:
  // Custom item drawing
  void PrepareItemDrawing(int index, bool isSelected, Color *fg, Color *bg, char *buffer) override;

  // Custom status info (file size + storage indicator)
  void GetStatusInfo(char *buffer, size_t bufferSize) override;

  // Called after directory setup completes
  void OnDirectorySetup() override;

  // Override OnFocus to handle dual directory mode
  void OnFocus() override;

  // Custom button drawing (for import/project mode buttons)
  void DrawButtons(int y, int selectedButton) override;

  // Handle custom button actions (return true if handled)
  bool OnButtonOverride(uint16_t mask, bool pressed) override;

private:
  // State
  size_t previewPlayingIndex_ = 0;
  bool playKeyHeld_ = false;
  bool editKeyHeld_ = false;
  bool inProjectSampleDir_ = false;

  // Internal helpers
  void warpToNextSample(bool goUp);
  void import();
  void preview(char *name);
  void adjustPreviewVolume(int offset);
  void showSampleEditor(etl::string<MAX_INSTRUMENT_FILENAME_LENGTH> filename, bool isProjectSample);
  void onConfirmRemoveProjectSample(View &view, ModalView &dialog);
};

#endif // _SAMPLE_IMPORT_VIEW_H_