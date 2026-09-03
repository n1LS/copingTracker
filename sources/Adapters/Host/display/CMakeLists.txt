add_library(platform_display
  chargfx_host.cpp
)

target_link_libraries(platform_display PUBLIC SDL2::SDL2)
target_include_directories(platform_display PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
