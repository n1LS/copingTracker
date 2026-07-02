/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#ifndef GUI_FACTORY_H_
#define GUI_FACTORY_H_

#include "UIFramework/Interfaces/I_GUIWindowFactory.h"

class GUIFactory : public I_GUIWindowFactory {

public:
  GUIFactory();
  virtual I_GUIWindowImp &CreateWindowImp(GUICreateWindowParams &);
  virtual EventManager *GetEventManager();
};

#endif
