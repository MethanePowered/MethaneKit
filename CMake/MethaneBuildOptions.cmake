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

if(TARGET MethaneBuildOptions)
    # MethaneBuildOptions target is already defined
    return()
endif()

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
        target_compile_definitions(MethaneBuildOptions INTERFACE APPLE_IOS)
    elseif (CMAKE_SYSTEM_NAME STREQUAL "tvOS")
        target_compile_definitions(MethaneBuildOptions INTERFACE APPLE_TVOS)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        target_compile_definitions(MethaneBuildOptions INTERFACE APPLE_MACOS)
    else()
        message(FATAL_ERROR "Methane Kit does not support Apple system: ${CMAKE_SYSTEM_NAME}")
    endif()

else(UNIX)

endif()

if (MSVC)

    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

        target_compile_options(MethaneBuildOptions INTERFACE
            # /Ox - Enable Most Speed Optimizations, including following flags:
            #   /Ob2 - Inline Function Expansion, Any Suitable;
            #   /Oi  - Generate Intrinsic Functions;
            #   /Ot  - Favor Fast Code;
            #   /Oy  - Frame-Pointer Omission.
            # /GL - Whole program optimization
            # /GF - Eliminate duplicate strings
            # /GS - Disable buffer security checks
            # /fp:except- - Disable floating point exceptions
            $<$<CONFIG:Release>:/Ox /GL /GF /GS- /fp:except->
            # Exception handling mode
            /EHsc
            # Set maximum warnings level and treat warnings as errors
            /W4 /WX
            # Disable useless warnings
            /wd4250 # - C4250: inheritance via dominance (used only with abstract interfaces)
            /wd4324 # - C4324: structure was padded due to alignment specifier
            /wd4201 # - C4201: nonstandard extension used : nameless struct/union (used for bitfields with mask)
            /wd4714 # - C4714: function marked as __forceinline not inlined
        )

        target_link_options(MethaneBuildOptions INTERFACE
            # /LTCG - Link-time code generation to perform whole-program optimization
            $<$<CONFIG:Release>:/LTCG>
        )

        target_compile_definitions(MethaneBuildOptions INTERFACE
            # FIXME: Silence MSVC warnings about use of stdext::checked_array_iterator until migration to C++20:
            #        warning STL4043: stdext::checked_array_iterator, stdext::unchecked_array_iterator,
            #        and related factory functions are non-Standard extensions and will be removed in the future.
            #        std::span (since C++20) and gsl::span can be used instead.
            _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING
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
        # -flto - use the link-time optimizer
        $<$<CONFIG:Release>:-flto>
        # Set maximum warnings level & treat warnings as errors
        -Wall -Wextra -Werror
        # Disable useless Clang and GCC warnings
        -Wno-missing-field-initializers
    )

    target_link_options(MethaneBuildOptions INTERFACE
        # -flto - use the link-time optimizer
        $<$<CONFIG:Release>:-flto>
    )

    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU") # GCC

        target_compile_options(MethaneBuildOptions INTERFACE
            # Disable useless GCC warnings
            -Wno-ignored-qualifiers
        )

    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

        target_compile_options(MethaneBuildOptions INTERFACE
            # -fwhole-program-vtables - whole-program vtable optimizations, such as single-implementation devirtualization
            $<$<CONFIG:Release>:-fwhole-program-vtables>
        )

        target_link_options(MethaneBuildOptions INTERFACE
            # -fwhole-program-vtables - whole-program vtable optimizations, such as single-implementation devirtualization
            $<$<CONFIG:Release>:-fwhole-program-vtables>
        )

    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")

        EXECUTE_PROCESS(COMMAND clang --version OUTPUT_VARIABLE CLANG_FULL_VERSION_STRING)
        string(REGEX REPLACE ".*clang version ([0-9]+\\.[0-9]+).*" "\\1" CLANG_VERSION_STRING ${CLANG_FULL_VERSION_STRING})

        # Warn about requirement to set OSX architectures for fat-binary starting with XCode & Clang v12.0
        # Build architectures have to be set with cmake generator command line option -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
        if (NOT CMAKE_OSX_ARCHITECTURES AND CLANG_VERSION_STRING VERSION_GREATER_EQUAL 12.0)
            message(WARNING "Apple Clang v12.0 requires build architectures to be set explicitly with cmake generator option -DCMAKE_OSX_ARCHITECTURES=\"arm64;x86_64\"")
        endif()

        # Disable duplicate libraries warning for AppleClang >= 15.0
        if (CLANG_VERSION_STRING VERSION_GREATER_EQUAL 15.0)
            target_link_options(MethaneBuildOptions INTERFACE
                LINKER:-no_warn_duplicate_libraries
            )
        endif()

        if(METHANE_RHI_PIMPL_INLINE_ENABLED AND METHANE_GFX_API EQUAL METHANE_GFX_METAL)
            target_compile_options(MethaneBuildOptions INTERFACE
                # Compile all C++ sources as Objective-C++ in case of RHI PIMPL inlining
                -x objective-c++
                # Enable automatic reference counting in Objective-C
                -fobjc-arc
            )
        endif()

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
