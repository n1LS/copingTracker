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

#include "Application/Utils/TintChar.h"
#include <cstdio>
#include <cstring>

struct CommandHelp {
  etl::array<TintChar, 27> line1;
  etl::array<TintChar, 27> line2;
  etl::array<TintChar, 27> line3;
  etl::array<TintChar, 27> line4;
};

#define LEGEND(N, A, B, C, D)                                                                                          \
  static constexpr CommandHelp legend_##N = {A, B, C, D};                                                              \
  return legend_##N;

// The command help data (getCommandHelp) is generated from the on-device
// Commands manual page, tools/manual/raw_data/Commands.copingDoc, which is
// the single source of truth for both the manual page and the command
// legend. Run tools/manual/raw_data/convert-commandhelp.py or ./build.py to
// regenerate the CommandHelp.generated.h that defines getCommandHelp().
#include "Application/Utils/CommandHelp.generated.h"