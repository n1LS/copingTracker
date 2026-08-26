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

enum ViewEventType {
  VET_SWITCH_VIEW,
  VET_PLAYER_POSITION_UPDATE,
  VET_LIST_SELECT,
  VET_LOAD_PROJECT,
  VET_NEW_PROJECT,
  VET_UPDATE
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
