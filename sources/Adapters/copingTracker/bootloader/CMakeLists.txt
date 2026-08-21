### bootloader ################################################################

add_executable(PatchBay
  bl_main.cpp
  bl_gfx.cpp
  bl_log.cpp
  bl_flash_writer.cpp
  bl_menu.cpp
  bl_path_utils.cpp
  bl_slot_boot.cpp
  bl_ui.cpp
  bl_sd_ops.cpp
  bl_flash_logic.cpp
  trace_wrap.cpp
  bl_uf2_parser.cpp
  ${PROJECT_SOURCE_DIR}/Adapters/copingTracker/display/ili9341.cpp
  ${PROJECT_SOURCE_DIR}/Adapters/copingTracker/platform/platform.cpp
  ${PROJECT_SOURCE_DIR}/Adapters/copingTracker/mutex/picoTrackerMutex.cpp
  ${PROJECT_SOURCE_DIR}/Adapters/copingTracker/system/input.cpp
)


pico_set_linker_script(PatchBay
  ${CMAKE_CURRENT_SOURCE_DIR}/bootloader_memmap.ld
)

target_link_libraries(PatchBay PUBLIC
    # Keep bootloader deps minimal and self-contained.
    platform_sdcard

    # Foundation
    foundation_services
    foundation
    
    # External and SDK libraries
    pico_sdk_bundle
    pico_stdlib
)

pico_define_boot_stage2(boot2_custom
  "${CMAKE_CURRENT_SOURCE_DIR}/boot2_custom.S"
)

target_compile_definitions(boot2_custom PRIVATE
  BOOTLOADER_START=0x10FF0000
)

pico_set_boot_stage2(PatchBay boot2_custom)

target_include_directories(PatchBay PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR})

target_compile_options(PatchBay PRIVATE
  $<$<COMPILE_LANGUAGE:ASM>:-Os>
  $<$<COMPILE_LANGUAGE:C>:-Os>
  $<$<COMPILE_LANGUAGE:CXX>:-Os>
  $<$<COMPILE_LANGUAGE:C>:-fno-unwind-tables>
  $<$<COMPILE_LANGUAGE:C>:-fno-asynchronous-unwind-tables>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-unwind-tables>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-asynchronous-unwind-tables>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
)

target_link_options(PatchBay PRIVATE
  -Wl,--wrap=_ZN5Trace5DebugEPKcz
  -Wl,--wrap=_ZN5Trace3LogEPKcS1_z
  -Wl,--print-memory-usage
)

target_compile_definitions(PatchBay PUBLIC
  PICO_FLASH_SIZE_BYTES=16*1024*1024
  PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64
  PICO_PRINTF_SUPPORT_FLOAT=0
  PICO_PRINTF_SUPPORT_EXPONENTIAL=0
)

pico_enable_stdio_usb(PatchBay 0)
pico_enable_stdio_uart(PatchBay 0)

# Create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(PatchBay)
