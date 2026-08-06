/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "GMBank.h"
#include "GMBank_data.generated.h"

bool GMBank::sampleForNote(uint16_t instrument, uint8_t note, uint8_t velocity, const SampleEntry **entry) {
  (void)velocity; // velocity is currently unused, but may be used in the future for velocity-layered samples

  if (note >= 128 || instrument >= kGMInstrumentCount) {
    return false;
  }

  const uint16_t sampleIndex = sampleLookup[instrument][note];

  if (sampleIndex == 0xffff) {
    return false;
  }

  *entry = &sampleEntries[sampleIndex];

  return true;
}