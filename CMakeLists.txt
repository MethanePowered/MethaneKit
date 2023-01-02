cmake_minimum_required(VERSION 3.18.0)

set(METHANE_LANGUAGES CXX C)

if(APPLE)
    list(APPEND METHANE_LANGUAGES OBJCXX)

    if (NOT CMAKE_OSX_ARCHITECTURES)
        # Warn about requirement to set OSX architectures for fat-binary starting with XCode & Clang v12.0
        # Build architectures have to be set with cmake generator command line option -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
        EXECUTE_PROCESS(COMMAND clang --version OUTPUT_VARIABLE CLANG_FULL_VERSION_STRING)
        string(REGEX REPLACE ".*clang version ([0-9]+\\.[0-9]+).*" "\\1" CLANG_VERSION_STRING ${CLANG_FULL_VERSION_STRING})
        if (CLANG_VERSION_STRING VERSION_GREATER_EQUAL 12.0)
            message(AUTHOR_WARNING "Apple Clang v12.0 requires build architectures to be set explicitly with cmake generator option -DCMAKE_OSX_ARCHITECTURES=\"arm64;x86_64\"")
        endif()
    endif()
endif()

if(NOT DEFINED PROJECT_NAME)
    set(IS_TOP_LEVEL_PROJECT ON)
else()
    set(IS_TOP_LEVEL_PROJECT OFF)
endif()

# Methane version, build & product info
set(METHANE_VERSION_MAJOR 0 CACHE STRING "Methane major version")
set(METHANE_VERSION_MINOR 7 CACHE STRING "Methane minor version")
set(METHANE_VERSION_PATCH 1 CACHE STRING "Methane patch version")
set(METHANE_VERSION_BUILD 0 CACHE STRING "Methane build version")

project(METHANE_KIT
        VERSION ${METHANE_VERSION_MAJOR}.${METHANE_VERSION_MINOR}.${METHANE_VERSION_PATCH}.${METHANE_VERSION_BUILD}
        DESCRIPTION "Modern 3D graphics made simple with C++17 cross-platform framework and rendering abstraction API on top of DirectX 12, Metal & Vulkan"
        HOMEPAGE_URL "https://github.com/MethanePowered/MethaneKit"
        LANGUAGES ${METHANE_LANGUAGES})

if (POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW) # option honors normal variables
endif()

if (POLICY CMP0110)
    cmake_policy(SET CMP0110 NEW) # allow whitespace and special characters in add_test() names
endif()

set(DEFAULT_APPS_BUILD_ENABLED ${IS_TOP_LEVEL_PROJECT})
set(DEFAULT_TESTS_BUILD_ENABLED ON)
set(DEFAULT_PRECOMPILED_HEADERS_ENABLED ON)

if(APPLE)
    # Disable precompiled headers in Apple builds because multi-language (C++/Obj-C) compilation is not supported
    set(DEFAULT_PRECOMPILED_HEADERS_ENABLED OFF)

    if (NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        # Disable tests build on Apple mobile systems, because they do not support running unbundled executables
        set(DEFAULT_TESTS_BUILD_ENABLED OFF)
    endif()

    if (DEFINED APPLE_DEVELOPMENT_TEAM)
        set(DEFAULT_APPLE_CODE_SIGNING_ENABLED ON)
    else()
        set(DEFAULT_APPLE_CODE_SIGNING_ENABLED OFF)
    endif()
endif()

if (CMAKE_BUILD_TYPE EQUAL "Release" OR
    CMAKE_BUILD_TYPE EQUAL "RelWithDebInfo")
    set(DEFAULT_RHI_INLINING_ENABLED ON)
else()
    set(DEFAULT_RHI_INLINING_ENABLED OFF)
endif()

# Build configuration
option(METHANE_GFX_VULKAN_ENABLED           "Enable Vulkan graphics API instead of platform native API" OFF)
option(METHANE_APPS_BUILD_ENABLED           "Enable applications build" ${DEFAULT_APPS_BUILD_ENABLED})
option(METHANE_TESTS_BUILD_ENABLED          "Enable tests build" ${DEFAULT_TESTS_BUILD_ENABLED})
option(METHANE_RHI_PIMPL_INLINE_ENABLED     "Enable RHI PIMPL implementation inlining" ${DEFAULT_RHI_INLINING_ENABLED})
option(METHANE_PRECOMPILED_HEADERS_ENABLED  "Enable precompiled headers" ${DEFAULT_PRECOMPILED_HEADERS_ENABLED})
option(METHANE_CHECKS_ENABLED               "Enable runtime checks of input arguments" ON)
option(METHANE_RUN_TESTS_DURING_BUILD       "Enable test auto-run after module build" ON)
option(METHANE_UNITY_BUILD_ENABLED          "Enable unity build speedup for some modules" ON)
option(METHANE_CODE_COVERAGE_ENABLED        "Enable code coverage data collection with GCC and Clang" OFF)
option(METHANE_SHADERS_CODEVIEW_ENABLED     "Enable shaders code symbols viewing in debug tools" OFF)
option(METHANE_OPEN_IMAGE_IO_ENABLED        "Enable using OpenImageIO library for images loading" OFF)
option(METHANE_APPLE_CODE_SIGNING_ENABLED   "Enable code signing on Apple platforms" ${DEFAULT_APPLE_CODE_SIGNING_ENABLED})

# Profiling and instrumentation configuration
option(METHANE_COMMAND_DEBUG_GROUPS_ENABLED "Enable command list debug groups with frame markup" OFF)
option(METHANE_LOGGING_ENABLED              "Enable debug logging" OFF)
option(METHANE_SCOPE_TIMERS_ENABLED         "Enable low-overhead profiling with scope-timers" OFF)
option(METHANE_ITT_INSTRUMENTATION_ENABLED  "Enable ITT instrumentation for trace capture with Intel GPA or VTune" OFF)
option(METHANE_ITT_METADATA_ENABLED         "Enable ITT metadata for tasks and events like function source locations" OFF)
option(METHANE_GPU_INSTRUMENTATION_ENABLED  "Enable GPU instrumentation to collect command list execution timings" OFF)
option(METHANE_TRACY_PROFILING_ENABLED      "Enable realtime profiling with Tracy" OFF)
option(METHANE_TRACY_PROFILING_ON_DEMAND    "Enable Tracy data collection on demand, after client connection" OFF)

# Methane version, build & product info
set(METHANE_VERSION_SHORT "${METHANE_VERSION_MAJOR}.${METHANE_VERSION_MINOR}.${METHANE_VERSION_PATCH}")
set(METHANE_VERSION_FULL  "${METHANE_VERSION_SHORT}.${METHANE_VERSION_BUILD}")
set(METHANE_PRODUCT_NAME  "Methane Kit (${HOMEPAGE_URL})")
set(METHANE_COPYRIGHT     "Copyright 2019-2022 Evgeny Gorodetskiy")
set(RESOURCES_DIR         "${CMAKE_CURRENT_SOURCE_DIR}/Resources")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")
include(MethaneModules)

get_native_graphics_apis()
get_default_graphics_api(METHANE_GFX_API)

set(CMAKE_CXX_STANDARD 17)
set(CTEST_OUTPUT_ON_FAILURE ON)
set(PARSE_CATCH_TESTS_VERBOSE OFF)
set(CMAKE_DISABLE_PRECOMPILE_HEADERS OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Use build-independent location for CPM package source cache to speedup CMake configuration
# NOTE: Do not use single cache location under CLion IDE to workaround CPM cache initialization collision
#       when multiple CMake configurations are initialized simultaneously
if (NOT CPM_SOURCE_CACHE AND NOT DEFINED ENV{CLION_IDE})
    set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/Build/Output/ExternalsCache" CACHE STRING "Set default CPM source cache path for External repositories")
endif()

if(MSVC)
    set(VS_STARTUP_PROJECT "MethaneShadowCube")
endif()

# METHANE_GFX_API is platform dependent and is selected in CMake/MethaneModules.cmake
get_graphics_dir(GRAPHICS_API_NAME)

# Print Methane Kit configuration options
message(STATUS "METHANE KIT version.............................. ${METHANE_VERSION_FULL}")
message(STATUS "METHANE RHI graphics API......................... ${GRAPHICS_API_NAME}")
message(STATUS "METHANE RHI PIMPL implementation inlining........ ${METHANE_RHI_PIMPL_INLINE_ENABLED}")
message(STATUS "METHANE build with precompiled headers........... ${METHANE_PRECOMPILED_HEADERS_ENABLED}")
message(STATUS "METHANE applications build....................... ${METHANE_APPS_BUILD_ENABLED}")
message(STATUS "METHANE tests build.............................. ${METHANE_TESTS_BUILD_ENABLED}")
message(STATUS "METHANE tests running during build............... ${METHANE_RUN_TESTS_DURING_BUILD}")
message(STATUS "METHANE runtime validation checks................ ${METHANE_CHECKS_ENABLED}")
message(STATUS "METHANE debug logging............................ ${METHANE_LOGGING_ENABLED}")
message(STATUS "METHANE command list debug groups................ ${METHANE_COMMAND_DEBUG_GROUPS_ENABLED}")
message(STATUS "METHANE shaders code symbols..................... ${METHANE_SHADERS_CODEVIEW_ENABLED}")
message(STATUS "METHANE image loading with OpenImageIO library... ${METHANE_OPEN_IMAGE_IO_ENABLED}")
message(STATUS "METHANE profiling scope timers................... ${METHANE_SCOPE_TIMERS_ENABLED}")
message(STATUS "METHANE ITT instrumentation...................... ${METHANE_ITT_INSTRUMENTATION_ENABLED}")
message(STATUS "METHANE ITT metadata............................. ${METHANE_ITT_METADATA_ENABLED}")
message(STATUS "METHANE GPU instrumentation...................... ${METHANE_GPU_INSTRUMENTATION_ENABLED}")
message(STATUS "METHANE Tracy profiling.......................... ${METHANE_TRACY_PROFILING_ENABLED}")
message(STATUS "METHANE Tracy profiling on demand................ ${METHANE_TRACY_PROFILING_ON_DEMAND}")

if (APPLE)
    message(STATUS "METHANE Apple code signing....................... ${METHANE_LOGGING_ENABLED} (dev.team: '${APPLE_DEVELOPMENT_TEAM}')")
endif()

if (APPLE AND METHANE_GFX_API EQUAL METHANE_GFX_VULKAN)
    # On Windows and Linux, only Vulkan-Headers are required to build Methane Kit,
    # because it is dynamically loading Vulkan library in runtime. Vulkan SDK is not required for build.
    message(WARNING "METHANE RHI: Vulkan API is not supported on MacOS, because MoltenVK does not support extension 'VK_EXT_extended_dynamic_state'")
    # To enable Vulkan API on MacOS we link statically with MoltenVK framework (translation layer to Metal).
    # in order to enable finding Vulkan SDK with MoltenVK framework, set environment variable
    # - VULKAN_SDK to full installation path before running CMake generator ($USER_HOME$/VulkanSDK/1.2.182.0/macOS)
    find_package(Vulkan REQUIRED)
    # To run Vulkan application on MacOS, additional environment variables need to be set for application:
    # - VK_ICD_FILENAMES=$USER_HOME$/VulkanSDK/1.2.182.0/macOS/share/vulkan/icd.d/MoltenVK_icd.json
    # - VK_LAYER_PATH=$USER_HOME$/VulkanSDK/1.2.182.0/macOS/share/vulkan/explicit_layer.d
endif()

if (METHANE_TESTS_BUILD_ENABLED)
    enable_testing()
endif()

if (CMAKE_INSTALL_PREFIX AND EXISTS "${CMAKE_INSTALL_PREFIX}/bin/ctest")
    set(CTEST_EXE "${CMAKE_INSTALL_PREFIX}/bin/ctest")
else()
    set(CTEST_EXE "ctest")
endif()

# Global options are included before Externals to be applied globally
include(MethaneGlobalOptions)

add_subdirectory(Externals)

# Build options are included after Externals due to dependency from External CMake modules
include(MethaneBuildOptions)

add_subdirectory(Modules)

if (METHANE_APPS_BUILD_ENABLED)
    add_subdirectory(Apps)
endif()

if (METHANE_TESTS_BUILD_ENABLED)
    add_subdirectory(Tests)
endif()

if (NOT IS_TOP_LEVEL_PROJECT)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} PARENT_SCOPE)
endif()
