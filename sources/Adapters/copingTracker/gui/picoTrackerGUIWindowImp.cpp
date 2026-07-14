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
#include "UIFramework/BasicDatas/GUIEvent.h"
#include "UIFramework/BasicDatas/GUIPoint.h"
#include "UIFramework/Framework/GUIColor.h"
#include "UIFramework/Interfaces/I_GUIWindowFactory.h"
#include "mirrorUI.h"
#include "mirrorUIProtocol.h"
#include "pico/stdlib.h"
#include "picoTrackerEventManager.h"
#include <stdio.h>
#include <string.h>
#include <string>

// Keep track of the last RGB values set for each palette index
static uint16_t lastPaletteRGB[16] = {0};

static GUIEventPadButtonType *eventMapping = eventMappingPico;

// Initialize static members
picoTrackerGUIWindowImp *picoTrackerGUIWindowImp::instance_ = NULL;

picoTrackerGUIWindowImp::picoTrackerGUIWindowImp(GUICreateWindowParams &p) {
  chargfx_init();
  instance_ = this;

  Config *config = Config::GetInstance();

  auto mirrorUIVar = (WatchedVariable *)config->FindVariable(Token::VarMirrorUI);

  // register to receive updates to mirrorUI setting
  mirrorUIVar->AddObserver(*this);
  auto mirrorUI = mirrorUIVar->GetInt();
  mirrorUIEnabled_ = mirrorUI != 0;

  auto uiFontVar = (WatchedVariable *)config->FindVariable(Token::VarUIFont);

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

void picoTrackerGUIWindowImp::mirrorUIConnectionChanged(bool connected) {
  if (connected && mirrorUIEnabled_) {
    mirrorUI_connected();
  }
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

void picoTrackerGUIWindowImp::SetPalette(const GUIColor *palette, int colorCount) {
  if (!palette) {
    return;
  }

  bool paletteChanged = false;

  for (int i = 0; i < colorCount; ++i) {
    int paletteIndex = palette[i].paletteIndex_;
    if (paletteIndex < 0 || paletteIndex >= 16) {
      continue;
    }

    uint16_t rgb565 = GUIColorToRGB565(palette[i]);
    if (lastPaletteRGB[paletteIndex] != rgb565) {
      chargfx_set_palette_color(paletteIndex, rgb565);
      lastPaletteRGB[paletteIndex] = rgb565;
      paletteChanged = true;
    }
  }

  SendPalette();

  if (paletteChanged) {
    chargfx_draw_screen();
  }
}

void picoTrackerGUIWindowImp::DrawChar(const char c, const GUIPoint &pos, bool transparent) {
  chargfx_set_cursor(pos.x_, pos.y_);
  chargfx_putc(c, transparent);
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

void picoTrackerGUIWindowImp::DrawRect(const GUIRect &r) {
  // This is the local drawing command for the device's own screen.
  chargfx_fill_rect(r.Left(), r.Top(), r.Width(), r.Height());

  if (mirrorUIEnabled_) {
    mirrorUI_sendRect(r.Left(), r.Top(), r.Width(), r.Height(), chargfx_get_foreground());
  }
};

void picoTrackerGUIWindowImp::Clear() {
  chargfx_clear();
};

void picoTrackerGUIWindowImp::ClearTextRect(GUIRect &r) {
  Trace::Debug("GUI ClearTextRect call");
};

void picoTrackerGUIWindowImp::SetColor(Color color) {
  chargfx_set_foreground(color);
};

void picoTrackerGUIWindowImp::SetBackgroundColor(Color color) {
  chargfx_set_background(color);
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
    mirrorUI_flush(scr, col, chg);
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
        auto uiFontVar = config->FindVariable(Token::VarUIFont);
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
    case Token::VarMirrorUI:
      {
        auto mirrorUI = v.GetInt();
        mirrorUIEnabled_ = mirrorUI != 0;
        break;
      }
    case Token::VarUIFont:
      {
        auto uifont = v.GetInt();
        chargfx_set_font_index(uifont);
        if (mirrorUIEnabled_) {
          SendFont(uifont);
        }
        break;
      }
  }
}
