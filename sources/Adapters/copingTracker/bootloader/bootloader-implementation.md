# Bootloader Implementation

This document reflects the current implemented state of the picoTracker bootloader.

## Goal

The bootloader is permanently stored at flash start and handles:

1. Importing UF2 files from SD card root into a firmware library directory.
2. Flashing a selected firmware image into a fixed app slot.
3. Booting the app slot manually or via countdown auto-boot.
4. Persisting metadata about the last flashed firmware.

The active design is single-slot app boot (no multi-slot policy).


## Module Structure

```
bl_main.cpp          → Entry point (board_init, platform_init, gfx_init, menu_render_static)
                         │
                         ▼
bl_ui.cpp            → Main loop: button debounce/dispatch, SD hotplug poll, auto-boot countdown
                         │
              ┌────────┼────────────┬──────────────────┐
              ▼        ▼            ▼                  ▼
       bl_sd_ops  bl_flash_logic  bl_menu          bl_log
        .h/.cpp    .h/.cpp        .h/.cpp           .h/.cpp
              │        │
              ▼        ▼
       bl_path_utils ──┼── bl_flash_writer  bl_uf2_parser
        .h/.cpp        │    .h/.cpp          .h/.cpp
                       │         │
                       ▼         ▼
                  bl_slot_boot  bl_config (shared constants)
                   .h/.cpp       bl_config.h
```

**Dependency direction:** Outer modules (bl_ui, bl_flash_logic, bl_sd_ops) depend on inner modules (bl_path_utils, bl_flash_writer, bl_uf2_parser, bl_slot_boot). All modules depend on bl_config.h for shared constants. There are no circular dependencies.


## Flash Layout

- Boot2 region:     0x10000000 .. 0x100000FF (256 bytes, never erased/written by firmware update)
- App slot base:    0x10000100 (right after boot2)
- App slot size:    0x007F0000
- Bootloader:       0x10FF0000 .. 0x10FFFFFF (top 64 KB, never written by firmware update)

Bootloader constants are consolidated in `bl_config.h`. Range checks are
implemented in:

- sources/Adapters/copingTracker/bootloader/bl_flash_writer.cpp

## Build Targets

Bootloader target name:

- PatchBay

Bootloader target definition:

- sources/Adapters/copingTracker/bootloader/CMakeLists.txt

Primary output artifacts:

- build/Adapters/copingTracker/bootloader/PatchBay.elf
- build/Adapters/copingTracker/bootloader/PatchBay.bin
- build/Adapters/copingTracker/bootloader/PatchBay.uf2

## Current Runtime Flow

Entry point:

- sources/Adapters/copingTracker/bootloader/bl_main.cpp (minimal startup, delegates to bl_run_ui_loop())
- sources/Adapters/copingTracker/bootloader/bl_ui.cpp (UI loop with button dispatch and auto-boot countdown)

Startup sequence:

1. board_init()
2. platform_init()
3. gfx_init()
4. menu_render_static()
5. SD mount
6. Read persisted firmware metadata from /firmwares/firmware_info.txt
7. Scan UF2 files in SD root (hidden files ignored)
8. Import new UF2 files to /firmwares/*.bin
9. Scan /firmwares for selectable .bin entries

Runtime interaction:

- UP/DOWN: selection
- START: flash selected firmware and persist metadata
- ENTER: boot app slot
- EDIT: enter device update bootloader mode
- Auto-boot: 3-second countdown if metadata exists; any key aborts

## USB Behavior

USB device polling is currently disabled in bootloader runtime:

- kEnableUsbDeviceTask is false

This is implemented as compile-time guarded tusb_init/tud_task calls in:

- sources/Adapters/copingTracker/bootloader/bl_ui.cpp

## UF2 Import and Flash Path

UF2 parsing and optional flashing are implemented in:

- sources/Adapters/copingTracker/bootloader/bl_uf2_parser.cpp

Behavior:

- Validates UF2 block magic and payload bounds.
- Preflights all target addresses against app slot range.
- Writes derived firmware image to /firmwares/*.bin.
- Optionally flashes to app slot page-by-page.

Flash operations are implemented in:

- sources/Adapters/copingTracker/bootloader/bl_flash_writer.cpp

Current flash writer behavior:

- Strict range and alignment checks.
- Erase step is skipped (old data left in place; page program overwrites relevant bytes).
- Page program and readback verify.
- Erase (when used) clamps to XIP_BASE to protect the boot2 region.
- Range checks reject writes into the bootloader region (0x10FF0000+).
- Returns error codes only (no printf-based diagnostics).

## App Slot Handoff

Implemented in:

- sources/Adapters/copingTracker/bootloader/bl_slot_boot.cpp

Current mode string in code:

- direct-jump-v4-no-cpsid

Vector table probe order:

1. slot + 0x100
2. slot + 0x110
3. slot + 0x000

Handoff checks and actions:

- Validate stack pointer range and reset handler range/thumb bit.
- Require 256-byte aligned vector table.
- Disable SysTick and clear NVIC pending/enables.
- Set VTOR to selected vector table.
- Set MSP and branch to reset handler.

## UI and Rendering

Text UI and list/menu rendering:

- sources/Adapters/copingTracker/bootloader/bl_menu.cpp

Character graphics implementation:

- sources/Adapters/copingTracker/bootloader/bl_gfx.cpp

Current rendering details:

- gfx_putc advances cursor x.
- Dirty-region rendering retained.
- Sub-region rendering was refactored into helpers for clarity.
- BUFFER_CHARS currently set to 15.

## Size Status

Latest confirmed local bootloader size:

- Size: 56652 bytes
- Overflow vs 65536-byte limit: 0

This confirms the bootloader is currently under the 64 KB cap.

## Major Recent Size Changes Already Landed

1. Runtime USB task disabled in bootloader main loop.
2. Bootloader target no longer compiles local USB descriptor unit.
3. Formatted printf calls removed from bl_flash_writer.
4. Trace::Debug/Log wrappers kept to avoid heavy log formatting pull-in.

## Known Follow-Up Cleanup (Non-Blocking)

- Cosmetic formatting in bl_flash_writer.cpp and bl_flash_writer.h can be normalized.
- SdFat ExFat path still links in; this is now optional optimization work, not required for size compliance.

## Key Files

- sources/Adapters/copingTracker/bootloader/bl_main.cpp — Startup entry point
- sources/Adapters/copingTracker/bootloader/bl_config.h — Shared constants (flash layout, paths, timing)
- sources/Adapters/copingTracker/bootloader/bl_ui.cpp — Main UI loop with button dispatch and auto-boot
- sources/Adapters/copingTracker/bootloader/bl_flash_logic.cpp — Flash-and-boot / boot-installed orchestration
- sources/Adapters/copingTracker/bootloader/bl_sd_ops.cpp — SD mount, scan, metadata read/write
- sources/Adapters/copingTracker/bootloader/bl_menu.cpp — Menu rendering (static + dynamic list)
- sources/Adapters/copingTracker/bootloader/bl_gfx.cpp — Character-graphics frame buffer display driver
- sources/Adapters/copingTracker/bootloader/bl_flash_writer.cpp — Low-level flash erase/write/verify
- sources/Adapters/copingTracker/bootloader/bl_uf2_parser.cpp — UF2 parse, derive .bin, flash .bin to slot
- sources/Adapters/copingTracker/bootloader/bl_slot_boot.cpp — Vector-table handoff to application
- sources/Adapters/copingTracker/bootloader/bl_path_utils.cpp — String/path manipulation helpers
- sources/Adapters/copingTracker/bootloader/bl_log.cpp — Boot-time logging to SD card
- sources/Adapters/copingTracker/bootloader/CMakeLists.txt
