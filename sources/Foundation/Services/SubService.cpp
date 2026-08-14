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

#include "SubService.h"
#include "ServiceRegistry.h"

SubService::SubService(int token, bool registerWithService) : token_(token), registerWithService_(registerWithService) {
  if (registerWithService_) {
    ServiceRegistry::GetInstance()->Register(this);
  }
}

SubService::~SubService() {
  if (registerWithService_) {
    ServiceRegistry::GetInstance()->Unregister(this);
  }
}
