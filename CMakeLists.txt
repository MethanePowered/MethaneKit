cmake_minimum_required(VERSION 3.13.0)

if(APPLE)

    # Warn about requirement to set OSX architectures for fat-binary starting with XCode & Clang v12.0
    # Build architectures have to be set with cmake generator command line option -DCMAKE_OSX_ARCHITECTURES=arm64;x86_64
    EXECUTE_PROCESS(COMMAND clang --version OUTPUT_VARIABLE CLANG_FULL_VERSION_STRING)
    string(REGEX REPLACE ".*clang version ([0-9]+\\.[0-9]+).*" "\\1" CLANG_VERSION_STRING ${CLANG_FULL_VERSION_STRING})
    if (CLANG_VERSION_STRING VERSION_GREATER_EQUAL 12.0 AND NOT CMAKE_OSX_ARCHITECTURES)
        message(AUTHOR_WARNING "Apple Clang v12.0 requires build architectures to be set explicitly with cmake generator option -DCMAKE_OSX_ARCHITECTURES=arm64;x86_64")
    endif()

endif()

project(MethaneKit)

if (POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW) # option honors normal variables
endif()

enable_testing()

# Build configuration
option(METHANE_RUN_TESTS_DURING_BUILD   "Run tests during build" ON)
option(METHANE_SHADERS_CODEVIEW_ENABLED "Enable shaders code symbols viewing in debug tools)" OFF)
option(METHANE_USE_OPEN_IMAGE_IO        "Enable using OpenImageIO library for images loading" OFF)

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
set(METHANE_VERSION_MAJOR 0 CACHE STRING "Methane major version")
set(METHANE_VERSION_MINOR 4 CACHE STRING "Methane minor version")
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

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_library(MethaneBuildOptions INTERFACE)

if(WIN32)

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

    target_include_directories(MethaneBuildOptions INTERFACE ${WINDOWSSDK_INCLUDE_DIRS})
    target_link_directories(MethaneBuildOptions INTERFACE ${WINDOWSSDK_LIBRARY_DIRS})
    target_compile_definitions(MethaneBuildOptions INTERFACE
        -DUNICODE -D_UNICODE -DNOMINMAX -DWIN32_LEAN_AND_MEAN -DUSE_PIX
    )

elseif(APPLE)

    set(CMAKE_BUILD_WITH_INSTALL_RPATH true)
    set(CMAKE_INSTALL_RPATH "@executable_path")

endif()

if (MSVC)

    set(VS_STARTUP_PROJECT MethaneAsteroids)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>") # use /MT static runtime linking
    string(REPLACE "/W3 " "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

        # Enable multi-threaded build with MSVC
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
        target_compile_options(MethaneBuildOptions INTERFACE
            # Set maximum warnings level and treat warnings as errors
            /W4 /WX
            # Disable useless warnings
            /wd4250 # - C4250: inheritance via dominance (used only with abstract interfaces)
            /wd4324 # - C4324: structure was padded due to alignment specifier
        )

    else() # Clang compiler on Windows

        target_compile_options(MethaneBuildOptions INTERFACE
            # Set maximum warnings level & treat warnings as errors
            -Werror
            # Disable useless warnings produced by Windows headers
            -Wno-unknown-pragmas
            -Wno-unused-local-typedef
            -Wno-ignored-pragma-intrinsic
            -Wno-expansion-to-defined
            -Wno-nonportable-include-path
            -Wno-pragma-pack
            -Wno-unused-value
            -Wno-microsoft-template
            -Wno-microsoft-template-shadow
            -Wno-microsoft-sealed
            -Wno-microsoft-exception-spec
            -Wno-ignored-attributes
            -Wno-macro-redefined
            -Wno-extern-c-compat
            -Wno-invalid-noreturn
        )

    endif()

else() # Clang or GCC on Linux/MacOS

    target_compile_options(MethaneBuildOptions INTERFACE
        # Set maximum warnings level & treat warnings as errors
        -Wall -Wextra -Werror
        # Disable useless Clang and GCC warnings
        -Wno-missing-field-initializers
    )

    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU") # GCC

        target_compile_options(MethaneBuildOptions INTERFACE
            # Disable useless GCC warnings
            -Wno-ignored-qualifiers
            -Wno-deprecated-copy
        )

    endif()

endif()

target_compile_definitions(MethaneBuildOptions INTERFACE
    "$<$<CONFIG:Debug>:_DEBUG>"
    "$<$<CONFIG:Release>:NDEBUG>"
    "$<$<CONFIG:MinSizeRel>:NDEBUG>"
    "$<$<CONFIG:RelWithDebInfo>:NDEBUG>"
)

message(STATUS "Methane Kit version ${METHANE_VERSION_FULL}")
split_by_first_delimiter("${METHANE_VERSION_BUILD}" "." METHANE_VERSION_BUILD_MAJOR METHANE_VERSION_BUILD_MINOR)
target_compile_definitions(MethaneBuildOptions INTERFACE
    -DMETHANE_VERSION_MAJOR=${METHANE_VERSION_MAJOR}
    -DMETHANE_VERSION_MINOR=${METHANE_VERSION_MINOR}
    -DMETHANE_VERSION_BUILD=${METHANE_VERSION_BUILD_MAJOR}
)

if(METHANE_RUN_TESTS_DURING_BUILD)
    message(STATUS "Methane tests running during build is enabled")
endif()

if(METHANE_SHADERS_CODEVIEW_ENABLED)
    message(STATUS "Methane shaders code symbols are enabled")
endif()

if(METHANE_USE_OPEN_IMAGE_IO)
    message(STATUS "Methane Kit OpenImageIO media-library is enabled.")
endif()

if(METHANE_COMMAND_DEBUG_GROUPS_ENABLED)
    message(STATUS "Methane command list debug groups are enabled")
endif()

if(METHANE_SCOPE_TIMERS_ENABLED)
    message(STATUS "Methane scope timers are enabled")
endif()

if(METHANE_ITT_INSTRUMENTATION_ENABLED)
    message(STATUS "Methane ITT instrumentation is enabled")
    target_compile_definitions(MethaneBuildOptions INTERFACE -DITT_INSTRUMENTATION_ENABLED)

    if(METHANE_ITT_METADATA_ENABLED)
        message(STATUS "Methane ITT metadata is enabled")
        target_compile_definitions(MethaneBuildOptions INTERFACE -DITT_ARGUMENTS_METADATA_ENABLED)
    endif()
endif()

if(METHANE_GPU_INSTRUMENTATION_ENABLED)
    message(STATUS "Methane GPU instrumentation is enabled")
endif()

if (METHANE_TRACY_PROFILING_ENABLED)
    if(METHANE_TRACY_PROFILING_ON_DEMAND)
        message(STATUS "Methane Tracy profiling enabled with data collection on demand (when client connected)")
    else()
        message(STATUS "Methane Tracy profiling enabled with data collection from application start")
    endif()
endif()

if(METHANE_LOGGING_ENABLED)
    message(STATUS "Methane debug logging is enabled")
endif()

if (CMAKE_INSTALL_PREFIX AND EXISTS "${CMAKE_INSTALL_PREFIX}/bin/ctest")
    set(CTEST_EXE "${CMAKE_INSTALL_PREFIX}/bin/ctest")
else()
    set(CTEST_EXE "ctest")
endif()

set(RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Resources")

# Disable finding packages by External libraries
set(CMAKE_DISABLE_FIND_PACKAGE_Doxygen TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_HarfBuzz TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_PkgConfig TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_ZLIB TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_PNG TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_BZip2 TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_HHC TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_LATEX TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_Perl TRUE)

add_subdirectory(Externals)
add_subdirectory(Modules)
add_subdirectory(Apps)
