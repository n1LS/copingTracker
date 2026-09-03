add_library(platform_gui
  HostGUIFactory.cpp
)

target_link_libraries(platform_gui PUBLIC
  platform_display
  uiframework_simplebaseclasses
  uiframework_basicdatas
  SDL2::SDL2
)

target_include_directories(platform_gui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
