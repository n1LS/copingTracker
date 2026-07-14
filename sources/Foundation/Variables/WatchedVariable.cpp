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

#include "WatchedVariable.h"

bool WatchedVariable::enabled_ = true;

WatchedVariable::WatchedVariable(Token id, int value) : Variable(id, value) {
  updating_ = false;
}

WatchedVariable::WatchedVariable(Token id, bool value) : Variable(id, value) {
  updating_ = false;
}

WatchedVariable::WatchedVariable(Token id, const char *const *list, int size, int index)
    : Variable(id, list, size, index) {
  updating_ = false;
}

void WatchedVariable::onChange() {
  if (!updating_ && enabled_) {
    updating_ = true;
    SetChanged();
    // Cast the Token value to I_ObservableData* as done in other parts of the
    // codebase
    NotifyObservers((I_ObservableData *)(uintptr_t)id_);
    updating_ = false;
  }
}

void WatchedVariable::Enable() {
  enabled_ = true;
}

void WatchedVariable::Disable() {
  enabled_ = false;
}
