add_library(platform_timer
  HostTimer.cpp
)

target_link_libraries(platform_timer PUBLIC system_timer SDL2::SDL2)
target_include_directories(platform_timer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
