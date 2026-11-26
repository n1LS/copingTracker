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

#include "PersistencyService.h"
#include "../Instruments/SamplePool.h"
#include "PersistenceConstants.h"

#include "Application/Model/UndoJournal.h"
#include "Foundation/Constants/Version.h"
#include "Foundation/Types/Types.h"
#include "Persistent.h"
#include "System/Console/Trace.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Memory/MemoryPool.h"
#include <cstring>

#define MAX_DELETE_DEPTH 3

PersistencyService::PersistencyService() : Service(Token::ServicePersistency) {};

PersistencyResult PersistencyService::CreateProject() {
  Trace::Log("APPLICATION", "create new project");
  // create  project
  CreateProjectDirs_(UNNAMED_PROJECT_NAME);
  return PersistencyService::GetInstance()->Save(UNNAMED_PROJECT_NAME, "", false);
}

bool PersistencyService::PurgeUnnamedProject() {
  return DeleteProject(UNNAMED_PROJECT_NAME);
}

bool PersistencyService::DeleteProject(const char *projectName) {
  auto fs = FileSystem::GetInstance();

  Trace::Debug("PERSISTENCYSERVICE", "Deleting project: %s", projectName);

  if (!fs->chdir(PROJECTS_DIR)) {
    Trace::Error("PERSISTENCYSERVICE: Could not change to projects dir");
    return false;
  }

  if (!fs->chdir(projectName)) {
    Trace::Error("PERSISTENCYSERVICE: Could not change to project dir");
    return false;
  }

  if (!DeleteDirectoryContents_(0)) {
    Trace::Error("PERSISTENCYSERVICE: Could not delete project contents");
    // attempt to recover to /projects for caller consistency
    fs->chdir("..");
    return false;
  }

  if (!fs->chdir("..")) { // up to projects dir
    Trace::Error("PERSISTENCYSERVICE: Could not change back to projects dir");
    return false;
  }

  if (!fs->DeleteDir(projectName)) {
    Trace::Error("PERSISTENCYSERVICE: Could not delete the project dir");
    return false;
  }

  return true;
}

bool PersistencyService::DeleteDirectoryContents_(uint8_t depth) {
  auto fs = FileSystem::GetInstance();
  if (depth > MAX_DELETE_DEPTH) {
    Trace::Error("PERSISTENCYSERVICE: delete depth exceeded");
    return false;
  }

  while (true) {
    MemoryPool::FileList().clear();
    fs->list(&MemoryPool::FileList(), "", loFiles | loFolders | loHidden);

    bool foundEntry = false;
    bool deletedEntry = false;
    for (size_t i = 0; i < MemoryPool::FileList().size(); ++i) {
      fs->getFileName(MemoryPool::FileList()[i], deleteNameBuffer_, sizeof(deleteNameBuffer_));

      if ((strcmp(deleteNameBuffer_, ".") == 0) || (strcmp(deleteNameBuffer_, "..") == 0)) {
        continue;
      }

      foundEntry = true;

      const PicoFileType type = fs->getFileType(MemoryPool::FileList()[i]);
      if (type == PFT_FILE) {
        if (!fs->DeleteFile(deleteNameBuffer_)) {
          Trace::Error("PERSISTENCYSERVICE: Could not delete file: %s", deleteNameBuffer_);
          return false;
        }
      } else if (type == PFT_DIR) {
        if (!DeleteDirectoryTree_(deleteNameBuffer_, depth + 1)) {
          return false;
        }
      } else {
        Trace::Error("PERSISTENCYSERVICE: Unknown file type for %s", deleteNameBuffer_);
        return false;
      }

      deletedEntry = true;
      break;
    }

    if (!foundEntry) {
      return true;
    }

    if (!deletedEntry) {
      Trace::Error("PERSISTENCYSERVICE: Unable to delete all entries");
      return false;
    }
  }
}

bool PersistencyService::DeleteDirectoryTree_(const char *dirname, uint8_t depth) {
  auto fs = FileSystem::GetInstance();
  char dirnameCopy[PFILENAME_SIZE];
  strncpy(dirnameCopy, dirname, sizeof(dirnameCopy));
  dirnameCopy[sizeof(dirnameCopy) - 1] = '\0';

  if (depth > MAX_DELETE_DEPTH) {
    Trace::Error("PERSISTENCYSERVICE: delete depth exceeded");
    return false;
  }

  if (!fs->chdir(dirnameCopy)) {
    Trace::Error("PERSISTENCYSERVICE: Could not chdir into dir: %s", dirnameCopy);
    return false;
  }

  bool success = DeleteDirectoryContents_(depth);
  if (!fs->chdir("..")) {
    Trace::Error("PERSISTENCYSERVICE: Could not return to parent dir");
    return false;
  }
  if (!success) {
    return false;
  }

  if (!fs->DeleteDir(dirnameCopy)) {
    Trace::Error("PERSISTENCYSERVICE: Could not delete dir: %s", dirnameCopy);
    return false;
  }

  return true;
}

PersistencyResult PersistencyService::CreateProjectDirs_(const char *projectName) {
  auto fs = FileSystem::GetInstance();

  // create samples sub dir as well as project dir containing it
  etl::vector<const char *, 3> segments = {PROJECTS_DIR, projectName, PROJECT_SAMPLES_DIR};
  CreatePath(pathBufferA, segments);

  auto result = fs->makeDir(pathBufferA.c_str(), true);
  Trace::Log("PERSISTENCYSERVICE", "created samples subdir: %s [%b]", pathBufferA.c_str(), result);

  return result ? PersistencyResult::PERSIST_SAVED : PersistencyResult::PERSIST_ERROR;
}

PersistencyResult PersistencyService::Save(const char *projectName, const char *oldProjectName, bool saveAs) {
  auto fs = FileSystem::GetInstance();

  if (saveAs && !Exists(projectName)) {
    CreateProjectDirs_(projectName);

    // copy across the samples from the old project
    fs->chdir(PROJECTS_DIR);
    fs->chdir(oldProjectName);
    fs->chdir(PROJECT_SAMPLES_DIR);

    Trace::Debug("get list of samples to copy from old project: %s", oldProjectName);

    fs->list(&MemoryPool::FileList(), ".wav");
    char filenameBuffer[PFILENAME_SIZE];
    for (size_t i = 0; i < MemoryPool::FileList().size(); i++) {
      fs->getFileName(MemoryPool::FileList()[i], filenameBuffer, sizeof(filenameBuffer));

      // ignore . and .. entries as using *.wav doesnt filter them out
      if (strcmp(filenameBuffer, ".") == 0 || strcmp(filenameBuffer, "..") == 0)
        continue;

      etl::vector<const char *, 4> filePathSegments = {PROJECTS_DIR, oldProjectName, PROJECT_SAMPLES_DIR,
                                                       filenameBuffer};
      CreatePath(pathBufferA, filePathSegments);

      filePathSegments = {PROJECTS_DIR, projectName, PROJECT_SAMPLES_DIR, filenameBuffer};
      CreatePath(pathBufferB, filePathSegments);

      fs->CopyFile(pathBufferA.c_str(), pathBufferB.c_str());
    };
  }
  return SaveProjectData(projectName, false);
}

PersistencyResult PersistencyService::AutoSaveProjectData(const char *projectName) {
  return SaveProjectData(projectName, true);
}

// the project is written to a file next to the real one and only replaces it when all of it was written, a card
// that is full or removed in the middle of a save leaves the old file as it was
PersistencyResult PersistencyService::SaveProjectData(const char *projectName, bool autosave) {
  const char *filename = autosave ? AUTO_SAVE_FILENAME : PROJECT_FILENAME;

  etl::vector<const char *, 3> segments = {PROJECTS_DIR, projectName, filename};
  CreatePath(pathBufferA, segments);

  etl::string<MAX_PROJECT_SAMPLE_PATH_LENGTH> tempPath(pathBufferA);
  tempPath.append(".tmp");

  auto fs = FileSystem::GetInstance();
  {
    auto fp = fs->Open(tempPath.c_str(), "w");
    if (!fp) {
      Trace::Error("PERSISTENCYSERVICE: Could not open file for writing: %s", tempPath.c_str());
      return PERSIST_ERROR;
    }
    Trace::Log("PERSISTENCYSERVICE", "Opened Proj File: %s", tempPath.c_str());

    JsonWriter writer(*fp);
    writer.BeginObject();
    writer.String(JSON_KEY_VERSION, PRODUCT_VERSION);

    // Loop on all registered persistable subservices
    for (auto *sub : SubServices()) {
      auto *currentItem = static_cast<Persistent *>(static_cast<void *>(sub));
      currentItem->Save(writer);
    }
    writer.EndObject();

    if (!writer.Finish() || !fp->Sync()) {
      Trace::Error("PERSISTENCYSERVICE: Could not write the project to %s", tempPath.c_str());
      fp.reset();
      fs->DeleteFile(tempPath.c_str());
      return PERSIST_ERROR;
    }
  } // the file is closed

  if (!ReplaceFile(tempPath.c_str(), pathBufferA.c_str())) {
    fs->DeleteFile(tempPath.c_str());
    return PERSIST_ERROR;
  }

  // if we are doing an explicit save (ie nto a autosave), then we need to
  // delete the existing autosave file so that this explicit save will be loaded
  // in case subsequent autosave has changes the user doesn't want to keep
  if (!autosave) {
    if (!ClearAutosave(projectName)) {
      Trace::Log("PERSISTENCYSERVICE", "Error Deleting Autosave File: %s", pathBufferA.c_str());
      // the autosave file may not have been created yet, eg. if this is a new
      // project or a project beign "saved as" so just keep going
    }
    Trace::Log("PERSISTENCYSERVICE", "Deleted Autosave File: %s", pathBufferA.c_str());
  }

  return PERSIST_SAVED;
}

// moves a finished file over the one it replaces, the old one is kept until the new one is in place
bool PersistencyService::ReplaceFile(const char *from, const char *to) {
  auto fs = FileSystem::GetInstance();
  etl::string<MAX_PROJECT_SAMPLE_PATH_LENGTH> backupPath(to);
  backupPath.append(".bak");

  bool hadOld = fs->exists(to);
  if (hadOld) {
    fs->DeleteFile(backupPath.c_str());
    if (!fs->MoveFile(to, backupPath.c_str())) {
      Trace::Error("PERSISTENCYSERVICE: Could not move the old file out of the way: %s", to);
      return false;
    }
  }
  if (!fs->MoveFile(from, to)) {
    Trace::Error("PERSISTENCYSERVICE: Could not move %s to %s", from, to);
    if (hadOld && !fs->MoveFile(backupPath.c_str(), to)) {
      Trace::Error("PERSISTENCYSERVICE: Could not put the old file back: %s", to);
    }
    return false;
  }
  if (hadOld) {
    fs->DeleteFile(backupPath.c_str());
  }
  return true;
}

// return true if existing proj with the given name already exists
bool PersistencyService::Exists(const char *projectName) {
  etl::string<128> projectFilePath(PROJECTS_DIR);
  projectFilePath.append("/");
  projectFilePath.append(projectName);

  auto fs = FileSystem::GetInstance();
  return fs->exists(projectFilePath.c_str());
}

PersistencyResult PersistencyService::Load(const char *projectName) {
  // the history belongs to the project that is gone
  UndoJournal::Instance().Clear();

  // first check if autosave exists
  etl::string<128> autoSavePath(PROJECTS_DIR);
  autoSavePath.append("/");
  autoSavePath.append(projectName);
  autoSavePath.append("/");
  autoSavePath.append(AUTO_SAVE_FILENAME);

  auto fs = FileSystem::GetInstance();
  bool useAutosave = (fs->exists(autoSavePath.c_str()));

  Trace::Log("PERSISTENCYSERVICE", "using autosave: %b", useAutosave);
  // if autosave exists, then we load it instead of the normal project file
  const char *filename = useAutosave ? AUTO_SAVE_FILENAME : PROJECT_FILENAME;

  etl::string<128> projectFilePath(PROJECTS_DIR);
  projectFilePath.append("/");
  projectFilePath.append(projectName);
  projectFilePath.append("/");
  projectFilePath.append(filename);

  auto fp = fs->Open(projectFilePath.c_str(), "r");
  if (!fp) {
    return PERSIST_LOAD_FAILED;
  }

  JsonReader reader(*fp);
  if (!reader.BeginObject()) {
    Trace::Error("could not find the master object");
    return PERSIST_LOAD_FAILED;
  }
  while (reader.NextKey()) {
    Persistent *target = nullptr;
    for (auto *sub : SubServices()) {
      auto *currentItem = static_cast<Persistent *>(static_cast<void *>(sub));
      if (!strcmp(reader.Key(), currentItem->NodeName())) {
        target = currentItem;
        break;
      }
    }
    // the version of the file and what this firmware does not know are skipped
    if (!target || !target->Restore(reader)) {
      reader.Skip();
    }
  }
  if (reader.HadError()) {
    Trace::Error("JSON errors detected while loading project '%s'", projectName);
    return PERSIST_LOAD_FAILED;
  }
  return PERSIST_LOADED;
}

PersistencyResult PersistencyService::LoadCurrentProjectName(char *projectName) {
  auto fs = FileSystem::GetInstance();
  if (fs->exists(PROJECT_STATE_FILE)) {
    auto current = fs->Open(PROJECT_STATE_FILE, "r");
    if (!current) {
      Trace::Error("PERSISTENCYSERVICE: Could not open project state file");
      return PERSIST_LOAD_FAILED;
    }
    int len = current->Read(projectName, MAX_PROJECT_NAME_LENGTH);
    projectName[len] = '\0';
    Trace::Log("APPLICATION", "read [%d] load proj name: %s", len, projectName);
    if (Exists(projectName)) {
      return PERSIST_LOADED;
    } else {
      Trace::Log("APPLICATION", "Project '%s' not found, loading untitled project", projectName);
      fs->DeleteFile(PROJECT_STATE_FILE);
      strcpy(projectName, UNNAMED_PROJECT_NAME);
      return PERSIST_LOADED;
    }
  } else {
    return PERSIST_LOAD_FAILED;
  }
}

PersistencyResult PersistencyService::SaveProjectState(const char *projectName) {
  auto fs = FileSystem::GetInstance();
  auto current = fs->Open(PROJECT_STATE_FILE, "w");
  if (!current) {
    return PERSIST_ERROR;
  }
  current->Write(projectName, 1, (int)strlen(projectName));
  return PERSIST_SAVED;
}

void PersistencyService::CreatePath(etl::istring &path, const etl::ivector<const char *> &segments) {
  // concatenate path segments into a single path
  path.clear();
  // iterate over segments and concatenate using iterator
  for (auto it = segments.begin(); it != segments.end(); ++it) {
    path.append(*it);
    if (it != segments.end() - 1) {
      path.append("/");
    }
  }
}

bool PersistencyService::ClearAutosave(const char *projectName) {
  auto fs = FileSystem::GetInstance();
  etl::vector<const char *, 3> segments = {PROJECTS_DIR, projectName, AUTO_SAVE_FILENAME};
  CreatePath(pathBufferA, segments);
  // TODO: check if file exists before deleting and only return false if it does
  // exist and deleting fails but this can only be done once Open() return
  // values are improved and we can implement a Exists() function on top of it
  return fs->DeleteFile(pathBufferA.c_str());
}

PersistencyResult PersistencyService::ExportInstrument(I_Instrument *instrument,
                                                       etl::string<MAX_INSTRUMENT_NAME_LENGTH> name, bool overwrite) {
  auto fs = FileSystem::GetInstance();

  // Add .ins extension to the filename
  etl::string<MAX_INSTRUMENT_FILENAME_LENGTH> filename = name;
  filename.append(INSTRUMENT_FILE_EXTENSION);

  etl::vector<const char *, 2> segments = {INSTRUMENTS_DIR, filename.c_str()};
  CreatePath(pathBufferA, segments);

  // check if file already exists
  if (fs->exists(pathBufferA.c_str())) {
    if (!overwrite) {
      return PERSIST_EXISTS;
    }
    // Delete the existing file if overwrite is true
    if (!fs->DeleteFile(pathBufferA.c_str())) {
      Trace::Error("PERSISTENCYSERVICE: Failed to delete existing file: %s", pathBufferA.c_str());
      return PERSIST_ERROR;
    }
  }

  auto fp = fs->Open(pathBufferA.c_str(), "w");
  if (!fp) {
    Trace::Error("PERSISTENCYSERVICE: Could not open file for writing: %s", pathBufferA.c_str());
    return PERSIST_ERROR;
  }

  JsonWriter writer(*fp);
  // Use the instrument's Persistent interface to save its data
  instrument->SaveDocument(writer);
  if (!writer.Finish() || !fp->Sync()) {
    Trace::Error("PERSISTENCYSERVICE: Could not write the instrument: %s", pathBufferA.c_str());
    return PERSIST_ERROR;
  }

  return PERSIST_SAVED;
}

static InstrumentType instrumentTypeFromName(const char *name) {
  for (int i = 0; i < IT_LAST; i++) {
    if (!strcasecmp(name, InstrumentTypeNames[i].persistence)) {
      return static_cast<InstrumentType>(i);
    }
  }
  return IT_NONE;
}

InstrumentType PersistencyService::DetectInstrumentType(const char *name) {
  auto fs = FileSystem::GetInstance();

  if (!fs->chdir(INSTRUMENTS_DIR)) {
    Trace::Error("PERSISTENCYSERVICE: Could not change to instruments directory");
    return IT_NONE;
  }

  auto fp = fs->Open(name, "r");
  if (!fp) {
    Trace::Error("PERSISTENCYSERVICE: Could not open instrument file: %s", name);
    return IT_NONE;
  }

  JsonReader reader(*fp);
  if (!reader.BeginObject()) {
    Trace::Error("PERSISTENCYSERVICE: Could not find the instrument object in file: %s", name);
    return IT_NONE;
  }

  // the type is the first member, nothing after it is read
  InstrumentType importedType = IT_NONE;
  while (reader.NextKey()) {
    if (reader.KeyIs(JSON_KEY_TYPE)) {
      char type[16];
      reader.ReadString(type, sizeof(type));
      Trace::Log("PERSISTENCYSERVICE", "Found instrument type in file: %s", type);
      importedType = instrumentTypeFromName(type);
      Trace::Log("PERSISTENCYSERVICE", "Mapped to instrument type: %d", importedType);
      break;
    }
    reader.Skip();
  }
  return importedType;
}

PersistencyResult PersistencyService::ImportInstrument(I_Instrument *instrument, const char *name) {
  auto fs = FileSystem::GetInstance();

  if (!fs->chdir(INSTRUMENTS_DIR)) {
    Trace::Error("PERSISTENCYSERVICE: Could not change to instruments directory");
    return PERSIST_ERROR;
  }

  auto fp = fs->Open(name, "r");
  if (!fp) {
    Trace::Error("PERSISTENCYSERVICE: Could not open instrument file: %s", name);
    return PERSIST_ERROR;
  }

  // Restore the instrument content: the type and version of the file are read (and logged) with the rest of it
  JsonReader reader(*fp);
  if (!instrument->Restore(reader)) {
    Trace::Error("PERSISTENCYSERVICE: Failed to restore instrument from file: %s", name);
    return PERSIST_ERROR;
  }

  // Extract instrument name from filename (minus .ins extension)
  etl::string<MAX_INSTRUMENT_NAME_LENGTH> instrumentName;
  const char *dotPos = strrchr(name, '.');
  if (dotPos) {
    // Calculate the length of the name without extension
    size_t nameLength = dotPos - name;
    // Copy only up to MAX_INSTRUMENT_NAME_LENGTH characters
    nameLength = nameLength < MAX_INSTRUMENT_NAME_LENGTH ? nameLength : MAX_INSTRUMENT_NAME_LENGTH - 1;
    instrumentName.assign(name, nameLength);
  } else {
    // No extension found, use the whole name (up to MAX_INSTRUMENT_NAME_LENGTH)
    instrumentName.assign(name,
                          strlen(name) < MAX_INSTRUMENT_NAME_LENGTH ? strlen(name) : MAX_INSTRUMENT_NAME_LENGTH - 1);
  }

  // Set the instrument name
  Variable *nameVar = instrument->FindVariable(Token::InstrumentName);
  if (nameVar) {
    nameVar->SetString(instrumentName.c_str());
  }

  // Mark the instrument as changed to trigger UI updates
  instrument->SetChanged();
  instrument->NotifyObservers();

  Trace::Log("PERSISTENCYSERVICE", "Successfully imported instrument settings");
  Trace::Log("PERSISTENCYSERVICE", "Set instrument name to: %s", instrumentName.c_str());
  return PERSIST_LOADED;
}
