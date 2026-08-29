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

#ifndef _VIEW_EVENT_H_
#define _VIEW_EVENT_H_

#include "Foundation/Observable.h"
#include "Foundation/Types/ViewType.h"

typedef enum ViewTransition {
  vtRevealFromLeft,
  vtRevealFromRight,
  vtRevealFromTop,
  vtRevealFromBottom,
  vtRevealFromCenter,
  vtDissolve,
  vtCollapse,
  vtNone,
} ViewTransition;

enum ViewEventType {
  vetSwitchView,
  vetPlayerPositionUpdate,
  vetListSelect,
  vetLoadProject,
  vetNewProject,
  vetQuitProject,
  vetUpdate
};

struct ViewEventData {
  ViewType type;
  ViewTransition transition;
};

class ViewEvent : public I_ObservableData {
public:
  ViewEvent(ViewEventType type, void *data = 0);
  ViewEventType GetType();
  void *GetData();

private:
  ViewEventType type_;
  void *data_;
};

#endif
