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

FILE: MethaneGlobalOptions.cmake
Methane global solution/workspace options, which must be set from root CMakeLists.txt

*****************************************************************************]]

if (METHANE_MEMORY_SANITIZER_ENABLED)
    # Add compiler and linker options globally for all modules
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

if(WIN32)

    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

        # Remove default warning level 3 if exists to replaced with higher level 4 in MethaneBuildOptions target
        string(REPLACE "/W3 " "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

        # Enable multi-threaded build with MSVC
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

        # Use /MT Static runtime linking
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    endif()

elseif(APPLE)

    if (NOT CMAKE_OSX_ARCHITECTURES)
        # Warn about requirement to set OSX architectures for fat-binary starting with XCode & Clang v12.0
        # Build architectures have to be set with cmake generator command line option -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
        EXECUTE_PROCESS(COMMAND clang --version OUTPUT_VARIABLE CLANG_FULL_VERSION_STRING)
        string(REGEX REPLACE ".*clang version ([0-9]+\\.[0-9]+).*" "\\1" CLANG_VERSION_STRING ${CLANG_FULL_VERSION_STRING})
        if (CLANG_VERSION_STRING VERSION_GREATER_EQUAL 12.0)
            message(WARNING "Apple Clang v12.0 requires build architectures to be set explicitly with cmake generator option -DCMAKE_OSX_ARCHITECTURES=\"arm64;x86_64\"")
        endif()
    endif()

    # Set OS deployment target minimum version
    if (DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "${DEPLOYMENT_TARGET}")
    else()
        set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0")
    endif()

    message(STATUS "METHANE OSX Minimum Deployment Target............ ${CMAKE_OSX_DEPLOYMENT_TARGET}")

    # Apple platform specific definitions
    if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(APPLE_IOS 1)
    elseif (CMAKE_SYSTEM_NAME STREQUAL "tvOS")
        set(APPLE_TVOS 1)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(APPLE_MACOS 1)
    else()
        message(FATAL_ERROR "Methane Kit does not support Apple system: ${CMAKE_SYSTEM_NAME}")
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

    if (METHANE_GFX_API EQUAL METHANE_GFX_VULKAN)
        # * On Windows and Linux, only Vulkan-Headers are required to build Methane Kit,
        #   because it is dynamically loading Vulkan library in runtime. Vulkan SDK is not required for build.
        # * To enable Vulkan API on MacOS we link statically with MoltenVK framework (translation layer to Metal).
        #   in order to enable finding Vulkan SDK with MoltenVK framework, environment variable might need to be set:
        #   - VULKAN_SDK to full installation path before running CMake generator ($HOME/VulkanSDK/1.3.250.1/macOS)
        find_package(Vulkan REQUIRED)
        # * To run Vulkan application on MacOS, additional environment variables might need to be set for application:
        #   - VK_ICD_FILENAMES=$HOME/VulkanSDK/1.3.250.1/macOS/share/vulkan/icd.d/MoltenVK_icd.json
        #   - VK_LAYER_PATH=$HOME/VulkanSDK/1.3.250.1/macOS/share/vulkan/explicit_layer.d
    endif()

else(UNIX)

    set(LINUX 1)

    find_package(X11 REQUIRED)
    if (NOT X11_xcb_FOUND OR
        NOT X11_X11_xcb_FOUND)
        # NOT X11_xcb_randr_FOUND) - TODO: uncomment "xcb_randr" check when supported by CMake 3.24
        message(FATAL_ERROR "Failed to find X11-XCB libraries, try `sudo apt-get install xcb libx11-dev libxcb-randr0-dev`")
    endif()

endif()