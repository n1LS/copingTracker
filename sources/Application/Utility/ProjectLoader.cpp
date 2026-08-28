/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "ProjectLoader.h"

#include "Adapters/copingTracker/audio/picoTrackerAudioDriver.h"
#include "Adapters/copingTracker/system/picoTrackerProjectLoader.h"
#include "Application/Commands/ApplicationCommandDispatcher.h"
#include "Application/Instruments/InstrumentBank.h"
#include "Application/Instruments/SamplePool.h"
#include "Application/Model/Mixer.h"
#include "Application/Persistency/PersistencyService.h"
#include "Application/Player/Player.h"
#include "Application/Player/TablePlayback.h"
#include "System/Console/Trace.h"
#include "System/Console/nanoprintf.h"
#include "System/FileSystem/FileSystem.h"
#include <cstring>

ProjectLoader::ProjectLoader(ProjectLoaderProtocol &listener, Project &project)
    : listener_(listener), project_(project) {
  projectName_[0] = '\0';
}

bool ProjectLoader::LoadProject(const char *projectName, bool createNew) {
  RunPhaseA(projectName, createNew);

  if (picoTrackerProjectLoader::StartLoad(projectName)) {
    loadInFlight_ = true;
    return true;
  }

  Trace::Error("Failed to start load (load already in progress)");
  return false;
}

void ProjectLoader::Update() {
  if (!loadInFlight_) {
    return;
  }

  if (picoTrackerProjectLoader::IsLoadComplete()) {
    picoTrackerProjectLoader::AcknowledgeLoadComplete();
    picoTrackerAudioDriver::ResumeAudioThread();
    loadInFlight_ = false;
    sampleLoadComplete_ = true;
  }
}

void ProjectLoader::FinalizeLoad() {
  if (!sampleLoadComplete_) {
    Trace::Error("PROJECTLOADER", "FinalizeLoad() called before sample load done");
    return;
  }
  sampleLoadComplete_ = false;
  RunPhaseC(projectName_);
  listener_.onPhaseCComplete(phaseCDone_, projectName_);
}

bool ProjectLoader::IsLoadInProgress() const {
  return loadInFlight_;
}

bool ProjectLoader::IsSampleLoadDone() const {
  return sampleLoadComplete_;
}

void ProjectLoader::SetProjectName(const char *name) {
  npf_snprintf(projectName_, sizeof(projectName_), "%s", name);
}

// ---------------------------------------------------------------------------
// Phase A: fast, synchronous preparatory work for project load
// ---------------------------------------------------------------------------
void ProjectLoader::RunPhaseA(const char *projectName, bool createNew) {
  Trace::Log("PROJECTLOADER", "Phase A: preparing load for '%s'", projectName);

  SetProjectName(projectName);
  createOnLoad_ = createNew;

  PersistencyService *persist = PersistencyService::GetInstance();

  // Reset FileSystem to a known state before touching any resources
  // This ensures SelectProjectView's directory state doesn't interfere
  FileSystem::GetInstance()->chdir("/");

  Player *player = Player::GetInstance();
  if (player->IsRunning()) {
    player->Stop();
    // Give the audio thread time to acknowledge the stop and park before
    // we attempt to reset core1. This is especially important when loading
    // from SelectProjectView where the audio thread is fully active.
    System::GetInstance()->Sleep(50);
  }

  TablePlayback::Reset();
  TableHolder::GetInstance()->Reset();
  Mixer::GetInstance()->Clear();

  SamplePool *pool = SamplePool::GetInstance();
  pool->Reset();

  project_.Load(projectName);

  if (createOnLoad_) {
    PersistencyResult created = persist->CreateProject();
    if (created != PERSIST_SAVED) {
      Trace::Error("PROJECTLOADER", "Failed to create new project '%s'", projectName);
    }
    createOnLoad_ = false;
  }

  listener_.onLoadPhaseAComplete();
}

// ---------------------------------------------------------------------------
// Phase C: fast, synchronous completion work after sample loading
// ---------------------------------------------------------------------------
void ProjectLoader::RunPhaseC(const char *projectName) {
  Trace::Log("PROJECTLOADER", "Phase C: completing load for '%s'", projectName);

  PersistencyService *persist = PersistencyService::GetInstance();
  Project *project = &project_;

  bool succeeded = (persist->Load(projectName) == PERSIST_LOADED);
  if (!succeeded) {
    Trace::Error("PROJECTLOADER", "Failed to load project '%s'", projectName);
    SamplePool::GetInstance()->Reset();
    TableHolder::GetInstance()->Reset();
    phaseCDone_ = false;
    return;
  }

  // Init instruments
  project->GetInstrumentBank()->Init();

  // Init command dispatcher
  ApplicationCommandDispatcher::GetInstance()->Init(project);

  // Save project state
  if (persist->SaveProjectState(projectName) != PERSIST_SAVED) {
    Trace::Error("PROJECTLOADER", "Failed to save project state for '%s'", projectName);
  }

  phaseCDone_ = true;
}
