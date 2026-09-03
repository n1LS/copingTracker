add_library(platform_midi
  HostMidiService.cpp
)

target_link_libraries(platform_midi PUBLIC services_midi)
target_include_directories(platform_midi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
