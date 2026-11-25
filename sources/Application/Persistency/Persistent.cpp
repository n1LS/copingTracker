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

#include "Persistent.h"
#include "Foundation/Types/Types.h"
#include "PersistencyService.h"

Persistent::Persistent(const char *nodeName, bool registerWithService)
    : SubService(Token::ServicePersistency), nodeName_(nodeName), registerWithService_(registerWithService) {
  if (registerWithService_) {
    PersistencyService::GetInstance()->Register(this);
  }
}

Persistent::~Persistent() {
  if (registerWithService_) {
    PersistencyService::GetInstance()->Unregister(this);
  }
}

void Persistent::Save(JsonWriter &writer) {
  writer.BeginObject(nodeName_);
  SaveContent(writer);
  writer.EndObject();
}

void Persistent::SaveDocument(JsonWriter &writer) {
  writer.BeginObject();
  SaveContent(writer);
  writer.EndObject();
}

bool Persistent::Restore(JsonReader &reader) {
  if (!reader.BeginObject()) {
    return false;
  }
  RestoreContent(reader);
  return !reader.HadError();
}
