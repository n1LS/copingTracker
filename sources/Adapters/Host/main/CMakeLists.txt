add_executable(copingTracker
  HostMain.cpp
)

set(COPINGTRACKER_LIBS
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
  memory_pool
)

# The static libraries depend on each other circularly. GNU ld scans each one
# only once, so on Linux they have to be rescanned until all symbols resolve.
# (ld64 on macOS resolves across archives and needs nothing.)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(copingTracker
    "$<LINK_GROUP:RESCAN,${COPINGTRACKER_LIBS}>"
    SDL2::SDL2
  )
else()
  target_link_libraries(copingTracker ${COPINGTRACKER_LIBS} SDL2::SDL2)
endif()

target_include_directories(copingTracker PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
