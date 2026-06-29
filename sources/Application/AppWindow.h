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

#ifndef _APP_WINDOW_H_
#define _APP_WINDOW_H_

#include "Application/Views/BaseClasses/View.h"
#include "Foundation/Types/Colors.h"
typedef union color_t {
  struct {
    Color fg : 4;
    Color bg : 4;
  };
  unsigned char byte;
} color_t;

#include "Application/Views/BaseClasses/View.h"
#include "Application/Views/ViewData.h"
#include "Foundation/Observable.h"
#include "Foundation/Types/Colors.h"
#include "System/Process/SysMutex.h"
#include "System/io/Status.h"
#include "UIFramework/SimpleBaseClasses/GUIWindow.h"
#include <UIFramework/Framework/GUIColor.h>
#include <UIFramework/SimpleBaseClasses/EventManager.h>

#define PROP_INVERT 0x80
#define CHAR_WIDTH 10
#define CHAR_HEIGHT 10
#define SCREEN_WIDTH 32
#define SCREEN_HEIGHT 24
#define SCREEN_MAP_HEIGHT 4
#define SCREEN_MAP_WIDTH 4
#define BATTERY_GAUGE_WIDTH 4
#define SCREEN_CHARS SCREEN_WIDTH *SCREEN_HEIGHT
#define MAX_FIELD_WIDTH 32
#define SCREEN_REDRAW_RATE PICO_CLOCK_HZ

class View;
struct AppWindowViews;

class AppWindow : public GUIWindow, I_Observer, Status {
protected:
  AppWindow(I_GUIWindowImp &imp, const char *projectName);
  virtual ~AppWindow();

public:
  static AppWindow *Create(GUICreateWindowParams &, const char *projectName);

  enum LoadProjectResult { LOAD_FAILED = -1, LOAD_OK = 0 };

  LoadProjectResult LoadProject(const char *name);
  void CloseProject();

  using GUIWindow::Clear;
  virtual void Clear();
  virtual void ClearTextRect(GUIRect &rect);
  virtual void DrawChar(const char c, const GUIPoint &pos, bool transparent = false);
  virtual void DrawString(const char *string, const GUIPoint &pos);
  virtual void SwapColors();
  virtual void SetColor(Color color);
  virtual void SetBackgroundColor(Color color);
  void InvalidateTextCache();

  void SetDirty();
  void UpdateColorsFromConfig();
  void SetSdCardPresent(bool present);

  char projectName_[MAX_PROJECT_NAME_LENGTH + 1];

  // Accessor for callback to update instruments
  Project &GetProject() {
    return project_;
  }

  static GUIColor colorPalette_[16];

public:
  void Flush();

protected: // GUIWindow implementation
  virtual bool onEvent(GUIEvent &event);
  virtual void onUpdate(bool redraw);
  virtual void LayoutChildren();
  virtual void Redraw() {};
  virtual void AnimationUpdate();

  // I_Observer implementation

  virtual void Update(Observable &o, I_ObservableData *d);

  // Status implementation

  virtual void Print(char *);
  virtual void PrintMultiLine(char *);

  void defineColor(FourCC colorCode, GUIColor &color, int paletteIndex);

private:
  bool AutoSave();

  Project project_;
  ViewData viewData_;
  AppWindowViews *views_;
  View *_currentView;

  bool _closeProject;
  bool _shouldQuit;
  uint16_t _mask;
  unsigned long _lastA;
  unsigned long _lastB;
  char _statusLine[80];

  bool lowBatteryState_;
  bool lowBatteryMessageShown_;
  bool sdCardMissing_;
  bool sdCardMessageShown_;

  static unsigned char _charScreen[SCREEN_CHARS];
  static color_t _screenColor[SCREEN_CHARS];
  static unsigned char _preScreen[SCREEN_CHARS];
  static color_t _preScreenColor[SCREEN_CHARS];

  color_t color_ = {.fg = Theme::View::fg, .bg = Theme::View::bg};

  static int charWidth_;
  static int charHeight_;

  bool loadProject_ = false;
  bool awaitingProjectLoadAck_ = false;
  bool createProjectOnLoad_ = false;
  bool playerInitialized_ = false;

  uint32_t lastAutoSave = 0;

  // Counter for animation frames, updated once per frame at PICO_CLOCK_HZ
  static uint32_t animationFrameCounter_;

public:
  // Static accessor for the animation frame counter
  static uint32_t GetAnimationFrameCounter() {
    return animationFrameCounter_;
  }
};

// C interface for use from the hardware adapter layer
void appwindow_set_sdcard_present(bool present);

#endif
