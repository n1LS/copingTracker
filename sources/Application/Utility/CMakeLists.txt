add_library(project_loader
  ProjectLoader.cpp
)

target_link_libraries(project_loader PUBLIC
  system_console
)

target_include_directories(project_loader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${PROJECT_SOURCE_DIR})
