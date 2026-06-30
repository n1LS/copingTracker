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

#ifndef _INSTRUMENT_IMPORT_VIEW_H_
#define _INSTRUMENT_IMPORT_VIEW_H_

#include "Application/Instruments/I_Instrument.h"
#include "BaseClasses/FileListView.h"
#include "ModalDialogs/MessageBox.h"
#include "ViewData.h"

/**
 * InstrumentImportView - Migrated to use FileListView base class
 *
 * This view allows users to browse and import instrument files.
 * Shows instrument type prefixes for each file.
 */
class InstrumentImportView : public FileListView {
public:
  InstrumentImportView(GUIWindow &w, ViewData *viewData);
  ~InstrumentImportView();

  // Required FileListView overrides
  const char *GetEmptyStateMessage() const override;

  // File selection handler
  void OnItemSelected(const char *filename) override;

  // Custom methods
  void Reset();

protected:
  void PrepareItemDrawing(int index, bool isSelected, Color *fg, Color *bg, char *buffer) override;
  // Called after directory setup completes
  void OnDirectorySetup() override;

private:
  int toInstrID_ = 0;

  // Instrument type cache for files
  etl::vector<InstrumentType, MAX_FILE_INDEX_SIZE> instrumentTypeList_;

  // Internal helpers
  void warpToNextInstrument(bool goUp);
  void importInstrument(const char *name);
  void detectInstrumentTypes();
  void onImportSuccess(View &view, ModalView &dialog);
};

#endif // _INSTRUMENT_IMPORT_VIEW_H_
