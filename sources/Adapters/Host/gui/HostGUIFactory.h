/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef HOST_GUI_FACTORY_H_
#define HOST_GUI_FACTORY_H_

#include "UIFramework/Interfaces/I_GUIWindowFactory.h"

class HostGUIFactory : public I_GUIWindowFactory {
public:
  HostGUIFactory();
  virtual ~HostGUIFactory();

  virtual I_GUIWindowImp &CreateWindowImp(GUICreateWindowParams &params) override;
  virtual EventManager *GetEventManager() override;
};

#endif
