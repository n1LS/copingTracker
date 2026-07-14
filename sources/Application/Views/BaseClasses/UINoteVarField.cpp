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

#include "UINoteVarField.h"
#include "Application/AppWindow.h"
#include "Application/Utils/char.h"
#include "ViewUtils.h"
#include <System/Console/nanoprintf.h>

UINoteVarField::UINoteVarField(const GUIPoint &position, Variable &v, const char *format, int min, int max, int xOffset,
                               int yOffset)
    : UIIntVarField(position, v, format, min, max, xOffset, yOffset) {};

void UINoteVarField::Draw(GUIWindow &w, int offset) {

  GUIPoint position = GetPosition();
  position.y_ += offset;

  char buffer[MAX_FIELD_WIDTH + 1];
  char note[4];

  unsigned char pitch = src_.GetInt();
  noteToString(pitch, note);
  note[3] = 0;
  npf_snprintf(buffer, sizeof(buffer), format_, note);

  DrawLabeledField(w, position, buffer, focus_);
}
