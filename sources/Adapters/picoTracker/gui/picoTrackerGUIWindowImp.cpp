/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#include "picoTrackerGUIWindowImp.h"
#include "Application/Commands/EventDispatcher.h"
#include "Application/Model/Config.h"
#include "Application/Utils/char.h"
#include "System/Console/Trace.h"
#include "UIFramework/BasicDatas/GUIPoint.h"
#include "UIFramework/BasicDatas/GUIEvent.h"
#include "UIFramework/Interfaces/I_GUIWindowFactory.h"
#include "pico/stdlib.h"
#include "mirrorUI.h"
#include <stdio.h>
#include <string.h>
#include <string>

// Keep track of the last RGB values set for each palette index
static uint16_t lastPaletteRGB[16] = {0};

// Keep track of the last "SetColor()" call to track the current color palette
// index, used by DrawRect() to know which color to use when drawing to the
// devices LCD
static uint8_t lastRemoteColorIdx = 255;
static uint8_t lastRemoteBackgroundColorIdx = 255;

static GUIEventPadButtonType *eventMapping = eventMappingPico;

// Initialize static members
picoTrackerGUIWindowImp *picoTrackerGUIWindowImp::instance_ = NULL;

picoTrackerGUIWindowImp::picoTrackerGUIWindowImp(GUICreateWindowParams &p) {
  chargfx_init();
  instance_ = this;

  Config *config = Config::GetInstance();

  auto mirrorUIVar = (WatchedVariable *)config->FindVariable(FourCC::VarMirrorUI);

  // register to receive updates to mirrorUI setting
  mirrorUIVar->AddObserver(*this);
  auto mirrorUI = mirrorUIVar->GetInt();
  mirrorUIEnabled_ = mirrorUI != 0;

  auto uiFontVar = (WatchedVariable *)config->FindVariable(FourCC::VarUIFont);
  
  // register to receive updates to mirrorUI setting
  uiFontVar->AddObserver(*this);
  auto uifontIndex = uiFontVar->GetInt();
  chargfx_set_font_index(uifontIndex);
  
  if (mirrorUIEnabled_) {
    SendFont(uifontIndex);
    SendPalette();
  }
};

picoTrackerGUIWindowImp::~picoTrackerGUIWindowImp() {
}

void picoTrackerGUIWindowImp::SendFont(uint8_t uifontIndex) {
  if (mirrorUIEnabled_) {
    mirrorUICommand *command = mirrorUI_getCommand();
    mirrorUI_command_Font(command, uifontIndex);
    mirrorUI_sendCommand(command);
  }
}

void picoTrackerGUIWindowImp::SendPalette() {
  if (mirrorUIEnabled_) {
    mirrorUI_sendPalette(chargfx_get_palette());
  }
}

void picoTrackerGUIWindowImp::DrawChar(const char c, const GUIPoint &pos, bool transparent) {
  chargfx_set_cursor(pos.x_, pos.y_);
  chargfx_putc(c);
}

void picoTrackerGUIWindowImp::DrawString(const char *string, const GUIPoint &pos) {
  if (!string) {
    return;
  }

  GUIPoint drawPos = pos;
  for (const char *current = string; *current; ++current, ++drawPos.x_) {
    DrawChar(*current, drawPos);
  }
}

void picoTrackerGUIWindowImp::DrawRect(GUIRect &r) {
  // This is the local drawing command for the device's own screen.
  chargfx_fill_rect(lastRemoteColorIdx, r.Left(), r.Top(), r.Width(), r.Height());
  /*
  if (mirrorUIEnabled_) {
    // Now, send the DrawRect command with full byte-escaping.
    // Worst-case buffer: 2 (header) + 9 payload bytes * 2 (if all are escaped)
    // = 20  bytes.
    char mirrorUIBuffer[20];
    auto bufferIndex = mirrorUIDrawRectCommand(r.Left(), r.Top(), r.Width(), r.Height(), mirrorUIBuffer);
    sendToUSBCDC(mirrorUIBuffer, bufferIndex);
  }
  */
};

void picoTrackerGUIWindowImp::Clear(GUIColor &c) {
  Color backgroundColor = GetColor(c);
  chargfx_set_background(backgroundColor);
  chargfx_clear(backgroundColor);
};

void picoTrackerGUIWindowImp::ClearTextRect(GUIRect &r) {
  Trace::Debug("GUI ClearTextRect call");
};

Color picoTrackerGUIWindowImp::GetColor(GUIColor &c) {
  // Palette index should always be < 16
  if (c.paletteIndex_ >= 16) {
    return WHITE; // Default to normal color if index is invalid
  }

  // Convert the color to RGB565 format
  uint16_t rgb565 = GUIColorToRGB565(c);

  // Only update the palette if the color has changed
  if (lastPaletteRGB[c.paletteIndex_] != rgb565) {
    chargfx_set_palette_color(c.paletteIndex_, rgb565);
    lastPaletteRGB[c.paletteIndex_] = rgb565;
  }

  return (Color)c.paletteIndex_;
}

void picoTrackerGUIWindowImp::SetColor(GUIColor &color) {
  Color gColor = GetColor(color);
  lastRemoteColorIdx = gColor;

  NAssert(c.r_ < 255);
  NAssert(c.g_ < 255);
  NAssert(c.b_ < 255);
  chargfx_set_foreground(gColor);
};

void picoTrackerGUIWindowImp::SetBackgroundColor(GUIColor &color) {
  Color gColor = GetColor(color);
  lastRemoteBackgroundColorIdx = gColor;

  NAssert(c.r_ < 255);
  NAssert(c.g_ < 255);
  NAssert(c.b_ < 255);
  chargfx_set_background(gColor);
};

void picoTrackerGUIWindowImp::Lock() {
}

void picoTrackerGUIWindowImp::Unlock() {
}

void picoTrackerGUIWindowImp::Flush() {
  if (mirrorUIEnabled_) {
    uint8_t *scr, *col;
    bool *chg;
    chargfx_get_screen_storage(&scr, &col, &chg);
    mirrorUI_Flush(scr, col, chg);
  }

  chargfx_draw_changed();
}

void picoTrackerGUIWindowImp::Invalidate() {
  picoTrackerEventQueue::GetInstance()->push(picoTrackerEvent(PICO_FLUSH));
};

void picoTrackerGUIWindowImp::PushEvent(GUIEvent &event) {
  Trace::Debug("GUI PushEvent");
};

GUIRect picoTrackerGUIWindowImp::GetRect() {
  Trace::Debug("GUI GetRect");
  return GUIRect(0, 0, 320, 240);
}

void picoTrackerGUIWindowImp::ProcessEvent(picoTrackerEvent &event) {
  switch (event.type_) {
    case PICO_REDRAW:
      instance_->_window->Update(true);
      // send font update
      if (instance_->mirrorUIEnabled_) {
        Config *config = Config::GetInstance();
        auto uiFontVar = config->FindVariable(FourCC::VarUIFont);
        int uifontIndex = uiFontVar->GetInt();
        instance_->SendFont(uifontIndex);
      }
      break;
    case PICO_FLUSH:
      instance_->_window->Update(false);
      break;
    case PICO_CLOCK:
      instance_->_window->ClockTick();
      break;
    case LAST:
      break;
  }
}

void picoTrackerGUIWindowImp::ProcessButtonChange(uint16_t changeMask, uint16_t buttonMask) {
  int e = 1;
  System *system = System::GetInstance();
  unsigned long now = system->GetClock();
  for (int i = 0; i < 10; i++) {
    if (changeMask & e) {
      GUIEventType type = (buttonMask & e) ? ET_PADBUTTONDOWN : ET_PADBUTTONUP;

      GUIEvent event(eventMapping[i], type, now, 0, 0, 0);
      instance_->_window->DispatchEvent(event);
    }
    e = e << 1;
  }
}

void picoTrackerGUIWindowImp::Update(Observable &o, I_ObservableData *d) {
  WatchedVariable &v = (WatchedVariable &)o;
  switch (v.GetID()) {
    case FourCC::VarMirrorUI: {
      auto mirrorUI = v.GetInt();
      mirrorUIEnabled_ = mirrorUI != 0;
      break;
    }
    case FourCC::VarUIFont: {
      auto uifont = v.GetInt();
      chargfx_set_font_index(uifont);
      if (mirrorUIEnabled_) {
        SendFont(uifont);
      }
      break;
    }
  }
}