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

#ifndef _SUBSERVICE_H_
#define _SUBSERVICE_H_

class SubService {
public:
  SubService(int token);
  virtual ~SubService();
  int GetToken() {
    return token_;
  };

private:
  int token_;
};
#endif
