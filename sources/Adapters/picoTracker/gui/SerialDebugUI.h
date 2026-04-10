/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#ifndef SERIAL_DEBUG_UI_H_
#define SERIAL_DEBUG_UI_H_

#include "pico/stdlib.h"

class SerialDebugUI {
public:
  SerialDebugUI();
  bool readSerialIn(char *buffer, short size);
  void dispatchCmd(char *cmd);
  void catFile(const char *path);
  void listFiles(const char *path);
  void rmFile(const char *path);
  void saveConfig();
  void mkdir(const char *path);
  void rmdir(const char *path);

private:
  int lp_ = 0;
};

#endif