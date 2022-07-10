cmake_minimum_required(VERSION 3.14.0)

set(CMAKE_DISABLE_FIND_PACKAGE_Doxygen TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_HarfBuzz TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_PkgConfig TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_ZLIB TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_PNG TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_BZip2 TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_HHC TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_LATEX TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_Perl TRUE)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

# CMake libraries
include(CPM) # keep 1-st
include(CMakeModules)
include(CMRC)
include(FindPkgConfig)

# Cross-platform C++ libraries
include(BoostNowide)
include(FMT)
include(Catch2)
include(CLI11)
include(FreeType2)
include(HLSLpp)
include(MagicEnum)
include(TaskFlow)
include(Tracy)
include(PerlinNoise)

if(METHANE_ITT_INSTRUMENTATION_ENABLED)
    include(IttApi)
endif()

if (NOT METHANE_OPEN_IMAGE_IO_ENABLED)
    include(STB)
endif()

# DirectX API C++ libraries
if(METHANE_GFX_API EQUAL METHANE_GFX_DIRECTX)
    include(DirectXHeaders)
    include(DirectXTex)
endif()

# Vulkan API C++ libraries
if (METHANE_GFX_API EQUAL METHANE_GFX_VULKAN)
    include(VulkanHeaders)
    include(SPIRVCross)
endif()

# Pre-built binary tools
include(DirectXShaderCompilerBinary)
include(SPIRVCrossBinary)

if (METHANE_OPEN_IMAGE_IO_ENABLED)
    include(OpenImageIOBinary)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} PARENT_SCOPE)