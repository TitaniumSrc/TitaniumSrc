cmake_minimum_required(VERSION 3.10)

project("TitaniumSrc")

set(TISRC_ROMFS "" CACHE STRING "Name of the PAF archive in 'src/main/assets'")
set(TISRC_DEFAULTGAME "" CACHE STRING "Name of the game to run instead of 'default'")
set(TISRC_DEBUG "1" CACHE STRING "The debug level for debug builds")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(TISRC__DBGARG "'DEBUG=${TISRC_DEBUG}'")
else()
    set(TISRC__DBGARG "")
endif()

add_custom_target(
    tisrc ALL
    DEPENDS "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libtisrc.so" sentry
    COMMAND :
)

add_custom_target(
    sentry
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/sentry1"
)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/SDL")
find_library(SDL2 SDL2)

add_custom_command(
    OUTPUT "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libtisrc.so" "${CMAKE_CURRENT_BINARY_DIR}/sentry1"
    DEPENDS SDL2
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tisrc"
    COMMAND make CROSS=android ${TISRC__DBGARG} ROMFS_EXT='${TISRC_ROMFS}' DEFAULTGAME='${TISRC_DEFAULTGAME}' OUTDIR='${CMAKE_LIBRARY_OUTPUT_DIRECTORY}' CFLAGS='-I${CMAKE_CURRENT_SOURCE_DIR}/SDL/include' LDFLAGS='-L${CMAKE_LIBRARY_OUTPUT_DIRECTORY}' CC='${CMAKE_C_COMPILER} --target=${CMAKE_C_COMPILER_TARGET}' CXX='${CMAKE_CXX_COMPILER} --target=${CMAKE_CXX_COMPILER_TARGET}' PLATFORM='Android_${CMAKE_SYSTEM_PROCESSOR}' -j$$(nproc)
)
