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

#include "I_Action.h"

I_Action::I_Action(char *name) {
  name_ = name;
}

I_Action::~I_Action() {};

char *I_Action::GetName() {
  return name_;
}
