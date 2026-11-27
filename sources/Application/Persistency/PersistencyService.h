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

#ifndef _PERSISTENCY_SERVICE_H_
#define _PERSISTENCY_SERVICE_H_

#include "Application/Instruments/I_Instrument.h"
#include "Externals/etl/include/etl/string.h"

#include "Application/Persistency/JsonReader.h"
#include "Application/Persistency/JsonWriter.h"
#include "Foundation/Services/Service.h"
#include "Foundation/T_Singleton.h"
#include "PersistenceConstants.h"
#include "System/FileSystem/FileSystem.h"

#define PROJECT_STATE_FILE SD_BASE_DIR "/.current"

enum PersistencyResult {
  PERSIST_SAVED,
  PERSIST_LOAD_FAILED,
  PERSIST_LOADED,
  PERSIST_ERROR,
  PERSIST_EXISTS,
};

class PersistencyService : public Service, public T_Singleton<PersistencyService> {
public:
  PersistencyService();
  PersistencyResult Save(const char *projectName, const char *oldProjectName, bool saveAs);
  PersistencyResult Load(const char *projectName, bool useAutosave = true);
  // useAutosave: false when the state asks for the project file (SelectProjectView), see BootUseAutosave
  PersistencyResult LoadCurrentProjectName(char *projectName);
  // the project that opens at the next boot is kept in the state file, with useAutosave false it asks for the
  // project file instead of the autosave file that is loaded when there is one (second line "project").
  // Neither touches the autosave file: only saving the project removes it.
  PersistencyResult SaveProjectState(const char *projectName);
  PersistencyResult WriteProjectState(const char *projectName, bool useAutosave);
  // what the state file asked for at the last LoadCurrentProjectName, true after SaveProjectState
  bool BootUseAutosave() const {
    return bootUseAutosave_;
  }
  bool HasAutosave(const char *projectName);
  PersistencyResult CreateProject();
  bool Exists(const char *projectName);
  bool PurgeUnnamedProject();
  bool DeleteProject(const char *projectName);
  PersistencyResult AutoSaveProjectData(const char *projectName);
  bool ClearAutosave(const char *projectName);

  PersistencyResult ExportInstrument(I_Instrument *instrument, etl::string<MAX_INSTRUMENT_NAME_LENGTH> name,
                                     bool overwrite = false);
  PersistencyResult ImportInstrument(I_Instrument *instrument, const char *name);
  InstrumentType DetectInstrumentType(const char *name);

private:
  bool ReplaceFile(const char *from, const char *to);
  PersistencyResult CreateProjectDirs_(const char *projectName);
  void CreatePath(etl::istring &path, const etl::ivector<const char *> &segments);
  PersistencyResult SaveProjectData(const char *projectName, bool autosave);
  bool DeleteDirectoryContents_(uint8_t depth);
  bool DeleteDirectoryTree_(const char *dirname, uint8_t depth);

  // need these as statically allocated buffers as too big for stack
  etl::string<MAX_PROJECT_SAMPLE_PATH_LENGTH> pathBufferA;
  etl::string<MAX_PROJECT_SAMPLE_PATH_LENGTH> pathBufferB;
  char deleteNameBuffer_[PFILENAME_SIZE];
  bool bootUseAutosave_ = true;
};

#endif
