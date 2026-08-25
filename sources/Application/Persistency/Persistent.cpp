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
#include "PersistencyService.h"
#include "Foundation/Types/Types.h"

Persistent::Persistent(const char *nodeName, bool registerWithService)
    : SubService(Token::ServicePersistency), nodeName_(nodeName),
      registerWithService_(registerWithService) {
  if (registerWithService_) {
    PersistencyService::GetInstance()->Register(this);
  }
}

Persistent::~Persistent() {
  if (registerWithService_) {
    PersistencyService::GetInstance()->Unregister(this);
  }
}

void Persistent::Save(tinyxml2::XMLPrinter *printer) {
  printer->OpenElement(nodeName_);
  SaveContent(printer);
  printer->CloseElement();
}

bool Persistent::Restore(PersistencyDocument *doc) {
  if (!strcmp(doc->ElemName(), nodeName_)) {
    RestoreContent(doc);
    return true;
  }
  return false;
}
