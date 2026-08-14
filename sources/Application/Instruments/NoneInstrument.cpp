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

#include "NoneInstrument.h"
#include "Application/Persistency/PersistenceConstants.h"
#include "Externals/etl/include/etl/string.h"

NoneInstrument::NoneInstrument() : I_Instrument(&variables_) {
}

NoneInstrument::~NoneInstrument() {
}

bool NoneInstrument::Init() {
  return true;
}

void NoneInstrument::OnStart() {
}

bool NoneInstrument::Start(int channel, unsigned char note, uint8_t volume, bool retrigger) {
  return true;
}

void NoneInstrument::Stop(int channel) {
}

bool NoneInstrument::Render(int channel, fixed *buffer, int size, bool updateTick) {
  return false;
}

bool NoneInstrument::IsInitialized() {
  return true; // Always initialised
}

void NoneInstrument::ProcessCommand(int channel, Token token, uint16_t value) {
}

int NoneInstrument::GetTable() {
  return 0;
}

bool NoneInstrument::GetTableAutomation() {
  return false;
}

void NoneInstrument::GetTableState(TableSaveState &state) {
}

void NoneInstrument::SetTableState(TableSaveState &state) {
}
