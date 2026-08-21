/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the PatchBay Boot Manager
 */

#include "bl_config.h"
#include "bl_gfx.h"
#include "bl_log.h"
#include "bl_menu.h"
#include "bl_ui.h"
#include "bsp/board.h"
#include "Adapters/copingTracker/platform/platform.h"

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  // Initialize microcontroller and peripheral hardware.
  board_init();
  platform_init();

  // Bring up the ILI9341 display controller.
  gfx_init();

  // Paint the static title bar, labels and key legend once.
  menu_render_static();

  bootlog("BOOTDBG: startup complete");

  // Enter the interactive UI loop (never returns).
  bl_run_ui_loop();

  return 0;
}
