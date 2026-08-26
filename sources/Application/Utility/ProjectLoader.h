/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _PROJECT_LOADER_H
#define _PROJECT_LOADER_H

#include "Application/Model/Project.h"
#include <cstdint>
#include <cstddef>

// Result codes for project loading phases
enum class LoadProjectResult { LOAD_FAILED = -1, LOAD_OK = 0 };

// Protocol interface for AppWindow (or any caller) to receive callbacks
// from ProjectLoader during the async load lifecycle.
class ProjectLoaderProtocol {
public:
  virtual ~ProjectLoaderProtocol() = default;

  // Called after Phase A (synchronous prep: stop player, reset state, load
  // project from persistence) completes successfully. The caller can then
  // switch to BootView if needed.
  virtual void onLoadPhaseAComplete() = 0;

  // Called after Phase C (load project data, init instruments/dispatcher)
  // completes. The caller should handle view setup. Returns false if the
  // load failed.
  virtual void onPhaseCComplete(bool success, const char *projectName) = 0;

  // Progress updates during the async sample-loading phase.
  virtual void onLoadProgress(uint32_t index, uint32_t total, const char *message) = 0;
};

class ProjectLoader {
public:
  ProjectLoader(ProjectLoaderProtocol &listener, Project &project);

  // Initiate the full load sequence:
  //   1. Phase A (synchronous prep)
  //   2. Start async sample load on core1 (via picoTrackerProjectLoader)
  //   3. FinalizeLoad() must be called when sample load is done and the
  //      caller's animation/gate condition (e.g. BootView animation) is met.
  //
  // Returns false if Phase A fails (cannot even start).
  bool LoadProject(const char *projectName, bool createNew);

  // Must be called every frame from the caller's animation loop.
  // Polls sample load completion and performs ack/resume, but does NOT
  // run Phase C — call FinalizeLoad() once IsSampleLoadDone() is true
  // and the caller's additional gate condition (e.g. BootView animation)
  // is satisfied.
  void Update();

  // Runs Phase C (data load, instrument init, dispatcher init, save state)
  // and notifies the listener via onPhaseCComplete(). Only call this when
  // IsSampleLoadDone() is true.
  void FinalizeLoad();

  // Query current load state
  bool IsLoadInProgress() const;
  bool IsSampleLoadDone() const;

  const char *GetProjectName() const { return projectName_; }

  // Wipe the project name buffer (used when falling back to untitled)
  void SetProjectName(const char *name);

private:
  void RunPhaseA(const char *projectName, bool createNew);
  void RunPhaseC(const char *projectName);

  ProjectLoaderProtocol &listener_;
  Project &project_;

  char projectName_[MAX_PROJECT_NAME_LENGTH + 1];

  bool loadInFlight_ = false;
  bool sampleLoadComplete_ = false;
  bool createOnLoad_ = false;
  bool phaseCDone_ = false;
};

#endif // _PROJECT_LOADER_H