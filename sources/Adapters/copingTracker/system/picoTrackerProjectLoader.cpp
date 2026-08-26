/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "picoTrackerProjectLoader.h"
#include "Adapters/copingTracker/audio/picoTrackerAudioDriver.h"
#include "Adapters/copingTracker/mutex/picoTrackerMutex.h"
#include "Application/Instruments/SamplePool.h"
#include "System/Console/Trace.h"
#include "pico/multicore.h"
#include <cstring>

namespace {
  struct LoadState {
    char projectName[MAX_PROJECT_NAME_LENGTH + 1] = {0};
    volatile bool inProgress = false;
    volatile bool complete = false;
    uint32_t progressIndex = 0;
    uint32_t progressTotal = 0;
    char progressMessage[64] = {0};
  };

  LoadState g_state;
  picoTrackerMutex g_loaderMutex;
}

bool picoTrackerProjectLoader::StartLoad(const char *projectName) {
  g_loaderMutex.Lock();
  if (g_state.inProgress) {
    g_loaderMutex.Unlock();
    return false;  // re-entrancy guard
  }
  strncpy(g_state.projectName, projectName, MAX_PROJECT_NAME_LENGTH);
  g_state.projectName[MAX_PROJECT_NAME_LENGTH] = '\0';
  g_state.inProgress = true;
  g_state.complete = false;
  g_state.progressIndex = 0;
  g_state.progressTotal = 0;
  g_state.progressMessage[0] = '\0';
  g_loaderMutex.Unlock();

  picoTrackerAudioDriver::SuspendAudioThreadForLoad();
  multicore_launch_core1(LoadThreadEntry);
  return true;
}

bool picoTrackerProjectLoader::IsLoadInProgress() {
  g_loaderMutex.Lock();
  bool inProgress = g_state.inProgress;
  g_loaderMutex.Unlock();
  return inProgress;
}

bool picoTrackerProjectLoader::IsLoadComplete() {
  g_loaderMutex.Lock();
  bool complete = g_state.complete;
  g_loaderMutex.Unlock();
  return complete;
}

void picoTrackerProjectLoader::GetProgress(uint32_t *index, uint32_t *total, char *messageBuf, size_t bufSize) {
  g_loaderMutex.Lock();
  if (index) *index = g_state.progressIndex;
  if (total) *total = g_state.progressTotal;
  if (messageBuf && bufSize > 0) {
    strncpy(messageBuf, g_state.progressMessage, bufSize - 1);
    messageBuf[bufSize - 1] = '\0';
  }
  g_loaderMutex.Unlock();
}

void picoTrackerProjectLoader::UpdateProgress(uint32_t index, uint32_t total, const char *message) {
  g_loaderMutex.Lock();
  g_state.progressIndex = index;
  g_state.progressTotal = total;
  if (message) {
    strncpy(g_state.progressMessage, message, sizeof(g_state.progressMessage) - 1);
    g_state.progressMessage[sizeof(g_state.progressMessage) - 1] = '\0';
  }
  g_loaderMutex.Unlock();
}

void picoTrackerProjectLoader::AcknowledgeLoadComplete() {
  g_loaderMutex.Lock();
  g_state.inProgress = false;
  g_state.complete = false;
  g_loaderMutex.Unlock();
}

void picoTrackerProjectLoader::LoadThreadEntry() {
  // Runs on core1. The actual sample loading happens here.
  Trace::Log("PROJECTLOADER", "Core1: Loading samples for project: %s", g_state.projectName);
  SamplePool::GetInstance()->Load(g_state.projectName);

  // Signal completion to core0.
  g_loaderMutex.Lock();
  g_state.complete = true;
  g_loaderMutex.Unlock();

  Trace::Log("PROJECTLOADER", "Core1: Load complete, parking");
  // Park here forever; core0 will multicore_reset_core1() and relaunch
  // AudioThread once it observes completion and calls
  // picoTrackerAudioDriver::ResumeAudioThread().
  while (true) {
    tight_loop_contents();
  }
}
