cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile_commands.json")
set(HOST_BUILD ON)

add_compile_options(
  -Wall
  -Wno-format
  -Wno-unused-variable
  -Wno-array-bounds
  -Wno-unused-parameter
  -Wno-missing-field-initializers
  -Wno-deprecated-declarations
  -Wno-inconsistent-missing-override
  -Wno-non-pod-varargs
  -Wno-incompatible-pointer-types
  -Wno-write-strings
  -Wno-narrowing
  -Wno-pragma-pack
  -Wno-overloaded-virtual
)


project(copingTracker C CXX)

# The static libraries depend on each other circularly. GNU ld scans each
# archive only once, so on Linux the whole library list is wrapped in a group
# that is rescanned until all symbols resolve. (ld64 on macOS needs nothing.)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(CMAKE_CXX_LINK_EXECUTABLE
    "<CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> -Wl,--start-group <LINK_LIBRARIES> -Wl,--end-group")
endif()

find_package(SDL2 REQUIRED)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../config")

add_subdirectory(../../Externals "${CMAKE_BINARY_DIR}/Externals")
add_subdirectory(../../Foundation "${CMAKE_BINARY_DIR}/Foundation")
add_subdirectory(../../System "${CMAKE_BINARY_DIR}/System")
add_subdirectory(../../Services "${CMAKE_BINARY_DIR}/Services")
add_subdirectory(../../UIFramework "${CMAKE_BINARY_DIR}/UIFramework")
add_subdirectory(../../Application "${CMAKE_BINARY_DIR}/Application")

add_subdirectory(main)
add_subdirectory(system)
add_subdirectory(timer)
add_subdirectory(mutex)
add_subdirectory(display)
add_subdirectory(gui)
add_subdirectory(audio)
add_subdirectory(filesystem)
add_subdirectory(midi)
add_subdirectory(platform)
