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

#include "GUIEvent.h"
#include "System/Console/Trace.h"

// Typed constructor

GUIEvent::GUIEvent(GUIPoint &point, GUIEventType type) : _position(point), _type(type) {};

GUIEvent::GUIEvent(int value, GUIEventType type) : _position(), _type(type), _value(value) {};

// Position accessor

void GUIEvent::SetPosition(GUIPoint &point) {
  _position = point;
}

// Position accessor

GUIPoint GUIEvent::GetPosition() {
  return _position;
}

// Type accessor

GUIEventType GUIEvent::GetType() {
  return _type;
}
