/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#include "Adapters/copingTracker/platform/platform.h"
#include "Adapters/copingTracker/system/picoTrackerSystem.h"
#include "Adapters/copingTracker/usb/msd_mode.h"
#include "Application/Application.h"
#include "BaseClasses/View.h"
#include "bsp/board.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "tusb.h"
#include <System/Console/Trace.h>

#include "../system/input.h"

int main(int argc, char *argv[]) {

  // Initialise microcontroller specific hardware
  board_init();

  // Check if MSD mode was requested before previous reboot
  if (msd_mode_requested()) {
    g_msd_mode = true;

    // Need platform_init for GPIO (display, SD card, input buttons)
    platform_init();

    // Enter MSD mode - handles SD init, USB init, and main loop.
    // tusb_init() is called inside msd_mode_run() after all slow
    // initialization (display, SD card) so that tud_task() can be
    // serviced immediately, allowing the USB host to enumerate the device.
    msd_mode_run();
  }

  // Normal mode: Initialise TinyUSB
  tusb_init();

  // Do remaining pT init, this needs to be done *after* above hardware and
  // tinyusb subsystem init
  platform_init();

  // Check for EDIT key hold on boot to force load untitled project
  {
    uint16_t keys = scanKeys();
    if (keys & BM_EDIT) { // Check for INPUT_EDIT (bit 6)
      forceLoadUntitledProject = true;
    }
  }

  // Make sure we get ETL logs
  Trace::RegisterEtlErrorHandler();

  picoTrackerSystem::Boot(argc, argv);

  GUICreateWindowParams params;
  params.title = "copingTracker";

  Application::GetInstance()->Init(params);

  picoTrackerSystem::MainLoop();
  // WE NEVER GET HERE

  picoTrackerSystem::Shutdown();
}
