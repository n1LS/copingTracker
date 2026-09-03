add_executable(copingTracker
  HostMain.cpp
)

target_link_libraries(copingTracker
  platform_system
  platform_gui
  platform_timer
  platform_mutex
  platform_audio
  platform_midi
  platform_filesystem
  platform_display
  platform
  application
  foundation_services
  foundation
  SDL2::SDL2
)

target_include_directories(copingTracker PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
