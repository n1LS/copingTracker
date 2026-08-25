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

#ifndef _GUIWINDOW_H_
#define _GUIWINDOW_H_

#include "Foundation/Types/Colors.h"
#include "UIFramework/BasicDatas/GUICreateWindowParams.h"
#include "UIFramework/BasicDatas/GUIEvent.h"
#include "UIFramework/Interfaces/I_GUIGraphics.h"

class I_GUIWindowImp;

//
// Abstract top class for all windows. Windows are control containers and
// they use a Window implementation class to provide the Graphical and
// notification functionalities
//

class GUIWindow : public I_GUIGraphics {

protected:
  // We protect the constructors because zzeeee
  // Class is abstract.

  GUIWindow(I_GUIWindowImp &);

public:
  // Destructor

  virtual ~GUIWindow();

public: // I_GUIGraphics implementation
  virtual void SetColor(Color color);
  virtual void SetBackgroundColor(Color color);
  virtual void ClearTextRect(GUIRect &rect);
  virtual void DrawChar(int x, int y, const char c, bool transparent = false);
  virtual void DrawString(int x, int y, const char *string);
  virtual void DrawRect(const GUIRect &rect);
  virtual void SetCurrentRectColor(Color color);
  virtual GUIRect GetRect();
  virtual void Invalidate();
  virtual void Flush();
  virtual void Lock();
  virtual void Unlock();
  virtual void Update(bool redraw);
  virtual void ClockTick();
  virtual void onUpdate(bool redraw) = 0;
  virtual void AnimationUpdate() = 0;
  //	virtual void Save() ;
  //	virtual void Restore() ;
  void PushEvent(GUIEvent &event);
  void Clear();
  I_GUIWindowImp *GetImpWindow() {
    return _imp;
  };

public:
  // When a local event has reached the control and could
  // not be dispatched elsewhere in the childrens. Events are
  // always in local coordinates

  virtual bool onEvent(GUIEvent &Event) = 0;

  virtual bool DispatchEvent(GUIEvent &Event);

  // Returns the offset of the control with respect to the
  // I_GUIGraphics it uses to render itself.

  virtual GUIPoint GetOffset() {
    return GUIPoint(0, 0);
  };

  virtual I_GUIGraphics *GetDC();

protected:
  // Returns the graphics used to render the window

  virtual I_GUIGraphics *GetGraphics();

private:
  I_GUIWindowImp *_imp; // the implementation system-dependant window
};

#endif
