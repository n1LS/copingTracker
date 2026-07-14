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

#include "ServiceRegistry.h"
#include "System/Console/Trace.h"

void ServiceRegistry::Register(Service *s) {
  if (services_.full()) {
    Trace::Error("ServiceRegistry full");
    return;
  }
  services_.push_back(s);
}

void ServiceRegistry::Register(SubService *s) {
  for (auto *svc : services_) {
    if (svc && svc->GetToken() == s->GetToken()) {
      svc->Register(s);
    };
  };
}

void ServiceRegistry::Unregister(SubService *s) {
  for (auto *svc : services_) {
    if (svc && svc->GetToken() == s->GetToken()) {
      svc->Unregister(s);
    };
  };
}
