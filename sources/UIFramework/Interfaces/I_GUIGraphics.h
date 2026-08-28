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

#include "Foundation/Types/Colors.h"
#include "UIFramework/BasicDatas/GUIRect.h"

// Interface definition for a graphical port.

class I_GUIGraphics {
public:
  virtual ~I_GUIGraphics() {};
  virtual void Clear() = 0;
  virtual void SetColor(Color color) = 0;
  virtual void SetBackgroundColor(Color color) = 0;
  virtual void ClearTextRect(GUIRect &) = 0;
  virtual void DrawString(int x, int y, const char *string) = 0;
  virtual void DrawChar(int x, int y, const char c, bool transparent = false) = 0;

  virtual GUIRect GetRect() = 0;
  virtual const GUIRect &GetFocusRect() const = 0;
  virtual void Invalidate() = 0;
  virtual void Lock() = 0;
  virtual void Unlock() = 0;
  virtual void Flush() = 0;
};

#endif
