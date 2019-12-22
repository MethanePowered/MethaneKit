cmake_minimum_required(VERSION 3.11.0)

project(MethaneKit)

enable_testing()

option(METHANE_USE_OPEN_IMAGE_IO "Enable using OpenImageIO library for images loading" OFF)
option(METHANE_ITT_INSTRUMENTATION_ENABLED "Enable ITT instrumentation" OFF)
option(METHANE_SHADERS_CODEVIEW_ENABLED "Enable shaders code viewing in debug tools while keeping compiler optimizations in Release builds)" OFF)
option(METHANE_RUN_TESTS_DURING_BUILD "Run tests during build" ON)

set(METHANE_VERSION "0.1" CACHE STRING "Methane release major-minor version number")
set(METHANE_BUILD_NUMBER "000000" CACHE STRING "Methane release build number")

set(METHANE_PRODUCT_NAME "Methane Kit (https://github.com/egorodet/MethaneKit)")
set(METHANE_COPYRIGHT "Copyright 2019 Evgeny Gorodetskiy")

include_directories($ENV{INCLUDE})
link_directories($ENV{LIB} ${CMAKE_LIBRARY_INPUT_DIRECTORY})

list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/CMake"
    "${CMAKE_CURRENT_SOURCE_DIR}/Externals/Catch2/single_include/cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/Externals/CMRC"
)

include(ParseAndAddCatchTests)

set(CMAKE_CXX_STANDARD 17)
set(CTEST_OUTPUT_ON_FAILURE ON)
set(PARSE_CATCH_TESTS_VERBOSE OFF)

if(MSVC)

    set(VS_STARTUP_PROJECT MethaneShadowCube)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    set(WINDOWS_SDK_BIN_PATH $ENV{WindowsSdkVerBinPath}x64)

    add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX -DWIN32_LEAN_AND_MEAN -DUSE_PIX)

elseif(APPLE)

    set(CMAKE_BUILD_WITH_INSTALL_RPATH true)
    set(CMAKE_INSTALL_RPATH "@executable_path")

endif()

if(METHANE_ITT_INSTRUMENTATION_ENABLED)
    add_definitions(-DITT_INSTRUMENTATION_ENABLED)
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