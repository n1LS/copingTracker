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

#define SEMITONE_FREQ_INTERVAL 1.0594630943592952645618252949461F

const char *loopTypes[SILM_LAST] = {"oneshot", "loop", "pingpong", "osciltr", "loopsnc"};
const char *interpolationTypes[] = {"lin", "non"};
const char *filterMode[] = {"orignal", "bassy", "screamo"};

enum FilterMode {
  FM_ORIGINAL = 0,
  FM_BASSY, // Same as normal but with a new frequency mapping
  FM_SCREAM,
  FM_LAST
};


