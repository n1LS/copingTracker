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

#ifndef _I_GUIGRAPHICS_H_
#define _I_GUIGRAPHICS_H_

#include "UIFramework/BasicDatas/GUIRect.h"
#include "Foundation/Types/Colors.h"

// #include "Engine/ENGBitmap.h"

// Interface definition for a graphical port.

class I_GUIGraphics {
public:
  virtual ~I_GUIGraphics() {};
  virtual void Clear() = 0;
  virtual void SetColor(Color color) = 0;
  virtual void SetBackgroundColor(Color color) = 0;
  virtual void ClearTextRect(GUIRect &) = 0;
  virtual void DrawString(const char *string, const GUIPoint &pos) = 0;
  virtual void DrawChar(const char c, const GUIPoint &pos, bool transparent = false) = 0;

  virtual GUIRect GetRect() = 0;
  virtual void Invalidate() = 0;
  virtual void Lock() = 0;
  virtual void Unlock() = 0;
  virtual void Flush() = 0;
};

#endif
