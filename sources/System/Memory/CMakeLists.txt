add_library(memory_pool
  MemoryPool.cpp
)

target_include_directories(memory_pool PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${PROJECT_SOURCE_DIR})
