cmake_minimum_required(VERSION 3.13.0)

project(MethaneKit)

enable_testing()

option(METHANE_USE_OPEN_IMAGE_IO "Enable using OpenImageIO library for images loading" OFF)
option(METHANE_ITT_INSTRUMENTATION_ENABLED "Enable ITT instrumentation" OFF)
option(METHANE_SCOPE_TIMERS_ENABLED "Enable low-overhead profiling with scope-timers" OFF)
option(METHANE_SHADERS_CODEVIEW_ENABLED "Enable shaders code symbols viewing in debug tools)" OFF)
option(METHANE_COMMAND_EXECUTION_LOGGING "Enable command execution debug logging" OFF)
option(METHANE_RUN_TESTS_DURING_BUILD "Run tests during build" ON)

set(METHANE_VERSION_MAJOR 0 CACHE STRING "Methane major version")
set(METHANE_VERSION_MINOR 3 CACHE STRING "Methane minor version")
set(METHANE_VERSION_BUILD 0 CACHE STRING "Methane build version")

set(METHANE_VERSION_SHORT "${METHANE_VERSION_MAJOR}.${METHANE_VERSION_MINOR}")
set(METHANE_VERSION_FULL  "${METHANE_VERSION_SHORT}.${METHANE_VERSION_BUILD}")
set(METHANE_PRODUCT_NAME  "Methane Kit (https://github.com/egorodet/MethaneKit)")
set(METHANE_COPYRIGHT     "Copyright 2019-2020 Evgeny Gorodetskiy")

list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/CMake"
    "${CMAKE_CURRENT_SOURCE_DIR}/Externals/CMakeModules"
    "${CMAKE_CURRENT_SOURCE_DIR}/Externals/Catch2/single_include/cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/Externals/CMRC"
)

include(MethaneUtils)
include(ParseAndAddCatchTests)

set(CMAKE_CXX_STANDARD 17)
set(CTEST_OUTPUT_ON_FAILURE ON)
set(PARSE_CATCH_TESTS_VERBOSE OFF)

if(MSVC)

    include(FindWindowsSDK)
    if(WINDOWSSDK_FOUND)
        message(STATUS "${WINDOWSSDK_LATEST_NAME} was selected for build")
    else()
        message(FATAL_ERROR "Windows SDK was not found!")
    endif()

    get_windowssdk_include_dirs(${WINDOWSSDK_LATEST_DIR} WINDOWSSDK_INCLUDE_DIRS)
    if(NOT WINDOWSSDK_INCLUDE_DIRS)
        message(FATAL_ERROR "Failed to get Windows SDK include directories from ${WINDOWSSDK_LATEST_DIR}")
    endif()

    get_windowssdk_library_dirs(${WINDOWSSDK_LATEST_DIR} WINDOWSSDK_LIBRARY_DIRS)
    if(NOT WINDOWSSDK_LIBRARY_DIRS)
        message(FATAL_ERROR "Failed to get Windows SDK library directories from ${WINDOWSSDK_LATEST_DIR}")
    endif()

    set(VS_STARTUP_PROJECT MethaneAsteroids)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>") # use /MT static runtime linking
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")                           # enable multi-threaded build

    include_directories(${WINDOWSSDK_INCLUDE_DIRS})
    link_directories(${WINDOWSSDK_LIBRARY_DIRS})
    add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX -DWIN32_LEAN_AND_MEAN -DUSE_PIX)

elseif(APPLE)

    set(CMAKE_BUILD_WITH_INSTALL_RPATH true)
    set(CMAKE_INSTALL_RPATH "@executable_path")

endif()

message(STATUS "Methane Kit version ${METHANE_VERSION_FULL}")
split_by_first_delimiter("${METHANE_VERSION_BUILD}" "." METHANE_VERSION_BUILD_MAJOR METHANE_VERSION_BUILD_MINOR)
add_definitions(
    -DMETHANE_VERSION_MAJOR=${METHANE_VERSION_MAJOR}
    -DMETHANE_VERSION_MINOR=${METHANE_VERSION_MINOR}
    -DMETHANE_VERSION_BUILD=${METHANE_VERSION_BUILD_MAJOR}
)

if(METHANE_USE_OPEN_IMAGE_IO)
    message(STATUS "Methane Kit OpenImageIO media-library is enabled.")
endif()

if(METHANE_ITT_INSTRUMENTATION_ENABLED)
    message(STATUS "Methane ITT instrumentation is enabled")
    add_definitions(-DITT_INSTRUMENTATION_ENABLED)
endif()

if(METHANE_SCOPE_TIMERS_ENABLED)
    message(STATUS "Methane scope timers are enabled")
    add_definitions(-DSCOPE_TIMERS_ENABLED)
endif()

if(METHANE_SHADERS_CODEVIEW_ENABLED)
    message(STATUS "Methane shaders code symbols are enabled")
endif()

if(METHANE_RUN_TESTS_DURING_BUILD)
    message(STATUS "Methane tests running during build is enabled")
endif()

if(METHANE_COMMAND_EXECUTION_LOGGING)
    message(STATUS "Methane command execution debug logging is enabled")
    add_definitions(-DCOMMAND_EXECUTION_LOGGING)
endif()

if (CMAKE_INSTALL_PREFIX AND EXISTS "${CMAKE_INSTALL_PREFIX}/bin/ctest")
    set(CTEST_EXE "${CMAKE_INSTALL_PREFIX}/bin/ctest")
else()
    set(CTEST_EXE "ctest")
endif()

set(RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Resources")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(Externals)
add_subdirectory(Modules)
add_subdirectory(Apps)