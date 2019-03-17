cmake_minimum_required(VERSION 3.11.0)

project(Methane)

enable_testing()

option(METHANE_USE_OPEN_IMAGE_IO "Enable using OpenImageIO library for images loading" OFF)
option(METHANE_USE_ITT_INSTRUMENTATION "Enable ITT instrumentation" OFF)

include_directories($ENV{INCLUDE})
link_directories($ENV{LIB} ${CMAKE_LIBRARY_INPUT_DIRECTORY})

list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/CMake"
    "${CMAKE_CURRENT_SOURCE_DIR}/Externals/Catch2/single_include/cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/Externals/CMRC"
)

set(CMAKE_CXX_STANDARD 14)
set(CTEST_OUTPUT_ON_FAILURE ON)

if(MSVC)

    set(VS_STARTUP_PROJECT MethaneShadowCube)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    set(WINDOWS_SDK_BIN_PATH $ENV{WindowsSdkVerBinPath}x64)

elseif(APPLE)

    set(CMAKE_BUILD_WITH_INSTALL_RPATH true)
    set(CMAKE_INSTALL_RPATH "@executable_path")

endif()

add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX -DWIN32_LEAN_AND_MEAN -DUSE_PIX)

if(METHANE_USE_ITT_INSTRUMENTATION)
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
