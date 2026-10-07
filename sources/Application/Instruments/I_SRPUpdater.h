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

#ifndef _I_SRP_UPDATER_H_
#define _I_SRP_UPDATER_H_

#include "Application/Utils/fixed.h"

struct RUParams {
  fixed volumeOffset_;
  fixed speedOffset_;
  fixed cutOffset_;
  fixed resOffset_;
  fixed panOffset_;
  fixed fbMixOffset_;
  fixed fbTunOffset_;
};

// Identities of the per-voice Sample Rendering Parameter updaters. Voices keep
// a small tagged list of which updaters are currently active (instead of a
// vector of polymorphic pointers), which keeps sample_voice_t free of vtable
// overhead. The updater classes live in SRPUpdaters.h; tag-based dispatch
// helpers are defined in SampleRenderingParams.h.
enum class UpdaterKind : uint8_t { Volume = 0, Pan, Cut, Res, Speed, Legato, Pfin, Arp, Vibrato, Count };

#endif
