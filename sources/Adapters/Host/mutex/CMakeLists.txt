add_library(platform_mutex
  HostMutex.cpp
)

target_link_libraries(platform_mutex PUBLIC system_process)
target_include_directories(platform_mutex PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
