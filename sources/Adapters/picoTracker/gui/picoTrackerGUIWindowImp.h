/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#ifndef PICOTRACKERWINDOWIMP_H_
#define PICOTRACKERWINDOWIMP_H_

#include "Adapters/picoTracker/display/chargfx.h"
#include "Foundation/Observable.h"
#include "UIFramework/Interfaces/I_GUIWindowImp.h"
#include "View.h"
#include "picoTrackerEventQueue.h"
#include <string>

// classic picotracker mapping
static GUIEventPadButtonType eventMappingPico[11] = {
    EPBT_LEFT,   // SW1
    EPBT_DOWN,   // SW2
    EPBT_RIGHT,  // SW3
    EPBT_UP,     // SW4
    EPBT_L,      // SW5
    EPBT_B,      // SW6
    EPBT_A,      // SW7
    EPBT_R,      // SW8
    EPBT_START,  // SW9
    EPBT_SELECT, // No SW
    EPBT_POWER   // Power button
};

class picoTrackerGUIWindowImp : public I_GUIWindowImp, public I_Observer {

public:
  picoTrackerGUIWindowImp(GUICreateWindowParams &p);
  virtual ~picoTrackerGUIWindowImp();

public: // I_GUIWindowImp implementation
  virtual void SetColor(Color color) override;
  virtual void SetBackgroundColor(Color color) override;
  virtual void DrawRect(GUIRect &rect) override;
  virtual void DrawChar(const char c, const GUIPoint &pos, bool transparent = false) override;
  virtual void DrawString(const char *string, const GUIPoint &pos) override;
  virtual GUIRect GetRect();
  virtual void Invalidate();
  virtual void Flush();
  virtual void Lock();
  virtual void Unlock();
  virtual void Clear();
  virtual void ClearTextRect(GUIRect &rect);
  virtual void PushEvent(GUIEvent &event);

  virtual void SendFont(uint8_t uifontIndex);
  virtual void SendPalette();
  virtual void SetPalette(const GUIColor *palette, int colorCount) override;

  static void ProcessEvent(picoTrackerEvent &event);
  static void ProcessButtonChange(uint16_t changeMask, uint16_t buttonMask);

  static picoTrackerGUIWindowImp *instance_;

  // mirrorUI connection and input handling
  virtual void mirrorUIConnectionChanged(bool connected);

protected:
  virtual void Update(Observable &o, I_ObservableData *d);

private:
  bool mirrorUIEnabled_ = false;
};
#endif
