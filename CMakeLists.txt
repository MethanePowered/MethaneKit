cmake_minimum_required(VERSION 3.21.0)

# Methane version, build & product info
set(METHANE_VERSION_MAJOR 0 CACHE STRING "Methane major version")
set(METHANE_VERSION_MINOR 7 CACHE STRING "Methane minor version")
set(METHANE_VERSION_PATCH 3 CACHE STRING "Methane patch version")
set(METHANE_VERSION_BUILD 0 CACHE STRING "Methane build version")

# Define CMake languages list
list(APPEND METHANE_LANGUAGES CXX C)
if(APPLE)
    list(APPEND METHANE_LANGUAGES OBJCXX)
endif()

project(METHANE_KIT
        VERSION ${METHANE_VERSION_MAJOR}.${METHANE_VERSION_MINOR}.${METHANE_VERSION_PATCH}.${METHANE_VERSION_BUILD}
        DESCRIPTION "Modern 3D graphics made simple with C++17 cross-platform framework and rendering abstraction API on top of DirectX 12, Metal & Vulkan"
        HOMEPAGE_URL "https://github.com/MethanePowered/MethaneKit"
        LANGUAGES ${METHANE_LANGUAGES})

if (POLICY CMP0077)
    # CMake option honors normal variables
    cmake_policy(SET CMP0077 NEW)
endif()

if (POLICY CMP0110)
    # Allow whitespace and special characters in add_test() names
    cmake_policy(SET CMP0110 NEW)
endif()

set(DEFAULT_APPS_BUILD_ENABLED ${PROJECT_IS_TOP_LEVEL})
set(DEFAULT_TESTS_BUILD_ENABLED ${PROJECT_IS_TOP_LEVEL})
set(DEFAULT_PRECOMPILED_HEADERS_ENABLED ON)
set(DEFAULT_RHI_INLINING_ENABLED OFF)

if(APPLE)
    # Disable precompiled headers in Apple builds because multi-language (C++/Obj-C) compilation is not supported
    set(DEFAULT_PRECOMPILED_HEADERS_ENABLED OFF)

    if (NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        # Disable tests build on Apple mobile systems, because they do not support running unbundled executables
        set(DEFAULT_TESTS_BUILD_ENABLED OFF)
    endif()
endif()

if (CMAKE_BUILD_TYPE EQUAL "Release" OR
    CMAKE_BUILD_TYPE EQUAL "RelWithDebInfo")
    # Methane RHI PIMPL calls to final API implementation is enabled in Release builds only,
    # because it significantly increases build times, but increases runtime performance by 5-10%
    set(DEFAULT_RHI_INLINING_ENABLED ON)
endif()

if (NOT CPM_SOURCE_CACHE)
    # Use build-independent location for Methane Externals CPM.cmake packages cache to speedup multi-configuration builds
    set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/Build/Output/ExternalsCache" CACHE STRING "Set default CPM source cache path for External repositories")
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

if(APPLE)
    option(METHANE_METAL_SHADER_CONVERTER_ENABLED "Enable Apple Metal Shader Converter instead of SPIRV-Cross" OFF)
endif()

# Profiling and instrumentation configuration
option(METHANE_COMMAND_DEBUG_GROUPS_ENABLED "Enable command list debug groups with frame markup" OFF)
option(METHANE_LOGGING_ENABLED              "Enable debug logging" OFF)
option(METHANE_SCOPE_TIMERS_ENABLED         "Enable low-overhead profiling with scope-timers" OFF)
option(METHANE_ITT_INSTRUMENTATION_ENABLED  "Enable ITT instrumentation for trace capture with Intel GPA or VTune" OFF)
option(METHANE_ITT_METADATA_ENABLED         "Enable ITT metadata for tasks and events like function source locations" OFF)
option(METHANE_GPU_INSTRUMENTATION_ENABLED  "Enable GPU instrumentation to collect command list execution timings" OFF)
option(METHANE_TRACY_PROFILING_ENABLED      "Enable realtime profiling with Tracy" OFF)
option(METHANE_TRACY_PROFILING_ON_DEMAND    "Enable Tracy data collection on demand, after client connection" OFF)
option(METHANE_MEMORY_SANITIZER_ENABLED     "Enable memory address sanitizer in compiler and linker" OFF)

# Platform dependent options
if(APPLE)
    set(DEFAULT_APPLE_CODE_SIGNING_ENABLED OFF)
    if (DEFINED APPLE_DEVELOPMENT_TEAM)
        set(DEFAULT_APPLE_CODE_SIGNING_ENABLED ON)
    endif()

    option(METHANE_APPLE_CODE_SIGNING_ENABLED "Enable code signing on Apple platforms" ${DEFAULT_APPLE_CODE_SIGNING_ENABLED})
endif()

# Methane version, build & product info
set(METHANE_VERSION_SHORT "${METHANE_VERSION_MAJOR}.${METHANE_VERSION_MINOR}.${METHANE_VERSION_PATCH}")
set(METHANE_VERSION_FULL  "${METHANE_VERSION_SHORT}.${METHANE_VERSION_BUILD}")
set(METHANE_PRODUCT_NAME  "Methane Kit (${HOMEPAGE_URL})")
set(METHANE_COPYRIGHT     "Copyright 2019-2023 Evgeny Gorodetskiy")
set(RESOURCES_DIR         "${CMAKE_CURRENT_SOURCE_DIR}/Resources")

# CMake configuration settings
set(CMAKE_CXX_STANDARD 17)
set(CTEST_OUTPUT_ON_FAILURE ON)
set(PARSE_CATCH_TESTS_VERBOSE OFF)
set(CMAKE_DISABLE_PRECOMPILE_HEADERS OFF)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(MSVC)
    set(VS_STARTUP_PROJECT "MethaneShadowCube")
endif()

# Use Methane Modules to select RHI backend native graphics API to be for rendering
include(MethaneModules)
get_native_graphics_apis()
get_default_graphics_api(METHANE_GFX_API)
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
message(STATUS "METHANE unity build.............................. ${METHANE_UNITY_BUILD_ENABLED}")
message(STATUS "METHANE code coverage............................ ${METHANE_CODE_COVERAGE_ENABLED}")
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
message(STATUS "METHANE memory sanitizer......................... ${METHANE_MEMORY_SANITIZER_ENABLED}")

if (APPLE)
    message(STATUS "METHANE Apple code signing....................... ${METHANE_APPLE_CODE_SIGNING_ENABLED} (dev.team: '${APPLE_DEVELOPMENT_TEAM}')")
endif()

# Global options are included before Externals to be applied globally
include(MethaneGlobalOptions)

# Add Methane External libraries using CPM.cmake
add_subdirectory(Externals)

# Build options are included after Externals due to dependency from External CMake modules
include(MethaneBuildOptions)

# Add Methane Modules
add_subdirectory(Modules)

if (METHANE_APPS_BUILD_ENABLED)
    # Add Methane Tutorial Applications
    add_subdirectory(Apps)
endif()

if (METHANE_TESTS_BUILD_ENABLED)
    # Add Methane Tests and enable CTest
    enable_testing()
    add_subdirectory(Tests)
endif()

if (NOT PROJECT_IS_TOP_LEVEL)
    # If Methane Kit is used as a library, provide CMAKE_MODULE_PATH to the parent project
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} PARENT_SCOPE)
endif()
