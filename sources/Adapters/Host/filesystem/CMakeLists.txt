add_library(platform_filesystem
  HostFileSystem.cpp
)

target_link_libraries(platform_filesystem PUBLIC system_filesystem)
target_include_directories(platform_filesystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
