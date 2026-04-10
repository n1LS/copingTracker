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

#include "ViewEvent.h"

ViewEvent::ViewEvent(ViewEventType type, void *data) {
  type_ = type;
  data_ = data;
}

ViewEventType ViewEvent::GetType() {
  return type_;
}

void *ViewEvent::GetData() {
  return data_;
}
