#[[****************************************************************************

Copyright 2022 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: MethaneBuildOptions.cmake
Methane compilation and build options interface target.

*****************************************************************************]]

add_library(MethaneBuildOptions INTERFACE)

target_compile_definitions(MethaneBuildOptions INTERFACE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:MinSizeRel>:NDEBUG>
    $<$<CONFIG:RelWithDebInfo>:NDEBUG>
    $<$<BOOL:${METHANE_CHECKS_ENABLED}>:METHANE_CHECKS_ENABLED>
    $<$<BOOL:${METHANE_ITT_INSTRUMENTATION_ENABLED}>:ITT_INSTRUMENTATION_ENABLED>
    $<$<AND:$<BOOL:${METHANE_ITT_INSTRUMENTATION_ENABLED}>,$<BOOL:${METHANE_ITT_METADATA_ENABLED}>>:ITT_ARGUMENTS_METADATA_ENABLED>
    METHANE_VERSION_MAJOR=${METHANE_VERSION_MAJOR}
    METHANE_VERSION_MINOR=${METHANE_VERSION_MINOR}
    METHANE_VERSION_PATCH=${METHANE_VERSION_PATCH}
    METHANE_VERSION_BUILD=${METHANE_VERSION_BUILD}
)

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
        UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN USE_PIX
        _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING # silence warning C4996 about std::wstring_convert deprecation
    )

elseif(APPLE)

    # Apple platform specific definitions
    if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(APPLE_IOS 1)
        target_compile_definitions(MethaneBuildOptions INTERFACE APPLE_IOS)
    elseif (CMAKE_SYSTEM_NAME STREQUAL "tvOS")
        set(APPLE_TVOS 1)
        target_compile_definitions(MethaneBuildOptions INTERFACE APPLE_TVOS)
    else() # Darwin
        set(APPLE_MACOS 1)
        target_compile_definitions(MethaneBuildOptions INTERFACE APPLE_MACOS)

        if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
            set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
        endif()
    endif()

    # Common code-signing options
    if (METHANE_APPLE_CODE_SIGNING_ENABLED)
        set(APPLE_CODE_SIGNING_FLAG "YES")
        set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic")
        if(NOT DEFINED CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY)
            set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development")
        endif()
    else()
        set(APPLE_CODE_SIGNING_FLAG "NO")
    endif()

    # Enable Obj-C automatic reference counting and Apple app options
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH true)
    set(CMAKE_INSTALL_RPATH "@executable_path")
    set(CMAKE_XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path")

else(UNIX)

    set(LINUX 1)

    find_package(X11 REQUIRED)
    if (NOT X11_xcb_FOUND OR
        NOT X11_X11_xcb_FOUND)
        # NOT X11_xcb_randr_FOUND) - TODO: uncomment "xcb_randr" check when supported by CMake 3.24
        message(FATAL_ERROR "Failed to find X11-XCB libraries, try `sudo apt-get install xcb libx11-dev libxcb-randr0-dev`")
    endif()

endif()

if (MSVC)

    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

        # Remove default warning level 3 if exists to replaced with higher level 4 (see below)
        string(REPLACE "/W3 " "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

        # Enable multi-threaded build with MSVC
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

        target_compile_options(MethaneBuildOptions INTERFACE
            # Exception handling mode
            /EHsc
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
        )

    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")

        target_compile_options(MethaneBuildOptions INTERFACE
            # Enable automatic reference counting in Objective-C
            "-fobjc-arc"
        )

    endif()

endif()

if (METHANE_CODE_COVERAGE_ENABLED)

    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
        CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        message(STATUS "Methane code coverage is enabled with Clang compiler")
        target_compile_options(MethaneBuildOptions INTERFACE -fprofile-instr-generate -fcoverage-mapping)
        target_link_options(MethaneBuildOptions INTERFACE -fprofile-instr-generate)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(STATUS "Methane code coverage is enabled with GCC compiler")
        target_compile_options(MethaneBuildOptions INTERFACE -coverage)
        target_link_options(MethaneBuildOptions INTERFACE -coverage)
    else()
        message(AUTHOR_WARNING "Methane code coverage is unavailable with ${CMAKE_CXX_COMPILER_ID} compiler")
    endif()

endif()