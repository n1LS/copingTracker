# Generic arm-none-eabi toolchain for the RP2040 (cortex-m0plus).
# Expects arm-none-eabi-gcc/g++ on PATH (used by CI). For a specific install,
# point PICO_TOOLCHAIN_FILE at your own file instead.
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR cortex-m0plus)

set(CMAKE_EXECUTABLE_SUFFIX_ASM ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_C ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX ".elf")

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)

set(CMAKE_C_FLAGS_INIT "-mcpu=cortex-m0plus -mthumb")
set(CMAKE_CXX_FLAGS_INIT "-mcpu=cortex-m0plus -mthumb")
set(CMAKE_ASM_FLAGS_INIT "-mcpu=cortex-m0plus -mthumb")

set(CMAKE_CROSSCOMPILING TRUE)

# Prevent CMake from trying to link/run test programs
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
