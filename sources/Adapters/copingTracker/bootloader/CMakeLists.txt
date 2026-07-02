add_executable(copingTrackerBootloader
  bootloader_main.cpp
  bootloader_gfx.cpp
  bootloader_log.cpp
  flash_writer.cpp
  bootloader_menu.cpp
  path_utils.cpp
  slot_boot.cpp
  trace_wrap.cpp
  uf2_parser.cpp
  ${PROJECT_SOURCE_DIR}/Adapters/copingTracker/display/ili9341.cpp
  ${PROJECT_SOURCE_DIR}/Adapters/copingTracker/platform/platform.cpp
  ${PROJECT_SOURCE_DIR}/Adapters/copingTracker/mutex/picoTrackerMutex.cpp
  ${PROJECT_SOURCE_DIR}/Adapters/copingTracker/system/input.cpp
)

target_link_libraries(copingTrackerBootloader PUBLIC
    # Keep bootloader deps minimal and self-contained.
    platform_sdcard

    # Foundation
    foundation_services
    foundation
    
    # External and SDK libraries
    pico_sdk_bundle
)

target_include_directories(copingTrackerBootloader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR})

target_compile_options(copingTrackerBootloader PRIVATE
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

target_link_options(copingTrackerBootloader PRIVATE
  -Wl,--wrap=_ZN5Trace5DebugEPKcz
  -Wl,--wrap=_ZN5Trace3LogEPKcS1_z
  -Wl,--print-memory-usage
)

target_compile_definitions(copingTrackerBootloader PUBLIC
  PICO_FLASH_SIZE_BYTES=16*1024*1024
  PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64
  PICO_PRINTF_SUPPORT_FLOAT=0
  PICO_PRINTF_SUPPORT_EXPONENTIAL=0
)

pico_enable_stdio_usb(copingTrackerBootloader 0)
pico_enable_stdio_uart(copingTrackerBootloader 0)

# Create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(copingTrackerBootloader)
