add_library(platform_system
  HostSystem.cpp
  input.cpp
  HostSamplePool.cpp
  HostProjectLoader.cpp
)

target_link_libraries(platform_system PUBLIC
  system_system
  system_timer
  system_filesystem
  system_process
  services_audio
  services_midi
  application_instruments
  uiframework_simplebaseclasses
  platform_timer
  platform_audio
  platform_midi
  platform_filesystem
  platform_gui
  SDL2::SDL2
)

target_include_directories(platform_system PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
