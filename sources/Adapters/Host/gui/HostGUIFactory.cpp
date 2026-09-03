/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "HostGUIFactory.h"
#include "Adapters/Host/display/chargfx_host.h"
#include "Adapters/Host/system/input.h"
#include "Application/AppWindow.h"
#include "Application/Views/BaseClasses/View.h"
#include "System/System/System.h"
#include "UIFramework/Framework/GUIColor.h"
#include "UIFramework/Interfaces/I_GUIGraphics.h"
#include <SDL2/SDL.h>
#include <cstring>
#include <iostream>

static GUIEventPadButtonType eventMappingHost[11] = {
    EPBT_LEFT,   // Left
    EPBT_DOWN,   // Down
    EPBT_RIGHT,  // Right
    EPBT_UP,     // Up
    EPBT_L,      // ALT
    EPBT_B,      // EDIT
    EPBT_A,      // ENTER
    EPBT_R,      // NAV
    EPBT_START,  // PLAY
    EPBT_SELECT, // unused
    EPBT_POWER   // unused
};

class HostGUIWindowImp : public I_GUIWindowImp {
public:
  HostGUIWindowImp(GUICreateWindowParams &p);
  virtual ~HostGUIWindowImp();

  virtual void Clear() override;
  virtual void SetColor(Color color) override;
  virtual void SetBackgroundColor(Color color) override;
  virtual void ClearTextRect(GUIRect &rect) override;
  virtual void DrawString(int x, int y, const char *string) override;
  virtual void DrawChar(int x, int y, const char c, bool transparent = false) override;
  virtual GUIRect GetRect() override;
  virtual const GUIRect &GetFocusRect() const override;
  virtual void Invalidate() override;
  virtual void Lock() override;
  virtual void Unlock() override;
  virtual void Flush() override;
  virtual void PushEvent(GUIEvent &event) override;
  virtual void DrawRect(const GUIRect &r) override;
  virtual void SendFont(uint8_t uifontIndex) override;
  virtual void SendPalette() override;
  virtual void SetPalette(const GUIColor *palette, int colorCount) override;
  virtual void mirrorUIConnectionChanged(bool connected) override;

  static void ProcessButtonChange(uint16_t changeMask, uint16_t buttonMask);

private:
  SDL_Window *window_;
  SDL_Renderer *renderer_;
  SDL_Texture *texture_;
  GUIRect focus_rect_;
};

HostGUIWindowImp::HostGUIWindowImp(GUICreateWindowParams &p) : window_(nullptr), renderer_(nullptr), texture_(nullptr) {
  chargfx_init();

  window_ = SDL_CreateWindow(p.title ? p.title : "copingTracker", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             320 * 2, 240 * 2, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

  if (window_) {
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer_) {
      texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, 320, 240);
    }
  }
}

HostGUIWindowImp::~HostGUIWindowImp() {
  if (texture_)
    SDL_DestroyTexture(texture_);
  if (renderer_)
    SDL_DestroyRenderer(renderer_);
  if (window_)
    SDL_DestroyWindow(window_);
}

void HostGUIWindowImp::Clear() {
  chargfx_clear();
}

void HostGUIWindowImp::SetColor(Color color) {
  chargfx_set_foreground(color);
}

void HostGUIWindowImp::SetBackgroundColor(Color color) {
  chargfx_set_background(color);
}

void HostGUIWindowImp::ClearTextRect(GUIRect &rect) {
  int x_end = rect.Right();
  int y_end = rect.Bottom();
  for (int y = rect.Top(); y < y_end && y < 24; ++y) {
    for (int x = rect.Left(); x < x_end && x < 32; ++x) {
      // Clear screen at this position
    }
  }
}

void HostGUIWindowImp::DrawString(int x, int y, const char *string) {
  if (!string)
    return;
  chargfx_set_cursor(x, y);
  for (const char *p = string; *p; ++p) {
    chargfx_putc(*p, false);
    if (chargfx_get_cursor_x() < 31) {
      chargfx_set_cursor(chargfx_get_cursor_x() + 1, y);
    }
  }
}

void HostGUIWindowImp::DrawChar(int x, int y, const char c, bool transparent) {
  chargfx_set_cursor(x, y);
  chargfx_putc(c, transparent);
}

GUIRect HostGUIWindowImp::GetRect() {
  return GUIRect(0, 0, 320, 240);
}

const GUIRect &HostGUIWindowImp::GetFocusRect() const {
  return focus_rect_;
}

void HostGUIWindowImp::Invalidate() {
  chargfx_draw_changed();
}

void HostGUIWindowImp::Lock() {
}

void HostGUIWindowImp::Unlock() {
}

void HostGUIWindowImp::Flush() {
  if (!renderer_ || !texture_)
    return;

  // Rasterize the changed text cells from the text screen into the pixel
  // buffer. RasterizeChar() only runs through these draw helpers, so without
  // this call text would never make it to the SDL texture.
  chargfx_draw_changed();

  // 2nd render pass for the focus rect (pulse effect), matching the PICO
  // implementation in picoTrackerGUIWindowImp::Flush().
  if (_window) {
    const GUIRect &rect = _window->GetFocusRect();
    chargfx_draw_focus_rect(rect.Left(), rect.Top(), rect.Width());
  }

  uint32_t *pixels = chargfx_get_pixel_buffer();
  SDL_UpdateTexture(texture_, nullptr, pixels, 320 * 4);

  SDL_RenderClear(renderer_);
  SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
  SDL_RenderPresent(renderer_);
}

void HostGUIWindowImp::PushEvent(GUIEvent &event) {
}

void HostGUIWindowImp::DrawRect(const GUIRect &r) {
  chargfx_fill_rect(r.Left(), r.Top(), r.Right() - r.Left(), r.Bottom() - r.Top());
}

void HostGUIWindowImp::SendFont(uint8_t uifontIndex) {
  chargfx_set_font_index(uifontIndex);
}

void HostGUIWindowImp::SendPalette() {
}

void HostGUIWindowImp::SetPalette(const GUIColor *palette, int colorCount) {
  if (!palette)
    return;
  for (int i = 0; i < colorCount && i < 16; ++i) {
    uint16_t rgb565 = ((palette[i].r_ >> 3) << 11) | ((palette[i].g_ >> 2) << 5) | (palette[i].b_ >> 3);
    chargfx_set_palette_color(i, rgb565);
  }
}

void HostGUIWindowImp::mirrorUIConnectionChanged(bool connected) {
}

void HostGUIWindowImp::ProcessButtonChange(uint16_t changeMask, uint16_t buttonMask) {
  int e = 1;
  System *system = System::GetInstance();
  unsigned long now = system ? system->GetClock() : 0;
  AppWindow *window = AppWindow::GetInstance();
  if (!window)
    return;

  for (int i = 0; i < 10; i++) {
    if (changeMask & e) {
      GUIEventType type = (buttonMask & e) ? ET_PADBUTTONDOWN : ET_PADBUTTONUP;
      GUIEvent event(eventMappingHost[i], type);
      window->DispatchEvent(event);
    }
    e = e << 1;
  }
}

static HostGUIWindowImp *s_window_imp = nullptr;

class HostEventManager : public EventManager {
public:
  virtual int MainLoop() override;
  virtual void SetVirtualButtonMask(uint16_t buttonMask, bool pressed) override;

private:
  uint16_t previous_mask_ = 0;
  unsigned long last_key_time_ = 0;
  bool is_repeating_ = false;
  static constexpr unsigned long KEY_DELAY = 500;
  static constexpr unsigned long KEY_REPEAT = 25;
};

int HostEventManager::MainLoop() {
  unsigned long tick = 0;
  AppWindow *app_window = AppWindow::GetInstance();
  if (!app_window)
    return 1;

  static bool initialized = false;

  while (true) {
    tick++;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        return 0;
      }
    }

    uint16_t new_mask = scanKeys() | 0;

    unsigned long now = tick * 33;

    uint16_t send_mask = (new_mask ^ previous_mask_) | (new_mask & (BM_LEFT | BM_RIGHT | BM_UP | BM_DOWN));

    bool got_event = false;

    if (new_mask == previous_mask_) {
      if (is_repeating_ && (now - last_key_time_) > KEY_REPEAT) {
        got_event = (send_mask != 0);
      }
      if (!is_repeating_ && (now - last_key_time_) > KEY_DELAY) {
        got_event = (send_mask != 0);
        if (got_event)
          is_repeating_ = true;
      }
    } else {
      got_event = (send_mask != 0);
      if (got_event)
        is_repeating_ = false;
    }

    if (got_event && initialized) {
      last_key_time_ = now;
      s_window_imp->ProcessButtonChange(send_mask, new_mask);
      previous_mask_ = new_mask;
    }

    if (tick > 5) {
      initialized = true;
    }

    if (initialized) {
      try {
        app_window->ClockTick();
        app_window->Flush();
      } catch (const std::exception &e) {
        std::cerr << "Exception in MainLoop: " << e.what() << std::endl;
        return 1;
      } catch (...) {
        std::cerr << "Unknown exception in MainLoop" << std::endl;
        return 1;
      }
    }

    SDL_Delay(33);
  }
}

void HostEventManager::SetVirtualButtonMask(uint16_t buttonMask, bool pressed) {
}

static HostEventManager s_event_manager;

HostGUIFactory::HostGUIFactory() {
}

HostGUIFactory::~HostGUIFactory() {
}

I_GUIWindowImp &HostGUIFactory::CreateWindowImp(GUICreateWindowParams &params) {
  alignas(HostGUIWindowImp) static char mem[sizeof(HostGUIWindowImp)];
  s_window_imp = new (mem) HostGUIWindowImp(params);
  return *s_window_imp;
}

EventManager *HostGUIFactory::GetEventManager() {
  return &s_event_manager;
}
