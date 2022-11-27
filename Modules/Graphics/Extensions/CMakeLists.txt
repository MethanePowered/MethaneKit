set(TARGET MethaneGraphicsExtensions)

include(MethaneShaders)

get_module_dirs("Methane/Graphics")

set(SHADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Shaders)

set(HEADERS
    ${INCLUDE_DIR}/Extensions.h
    ${INCLUDE_DIR}/ImageLoader.h
    ${INCLUDE_DIR}/MeshBuffers.hpp
    ${INCLUDE_DIR}/SkyBox.h
    ${INCLUDE_DIR}/ScreenQuad.h
)

set(SOURCES
    ${SOURCES_DIR}/ImageLoader.cpp
    ${SOURCES_DIR}/SkyBox.cpp
    ${SOURCES_DIR}/ScreenQuad.cpp
    ${SHADERS_DIR}/ScreenQuadConstants.h
    ${SHADERS_DIR}/SkyBoxUniforms.h
)

set(HLSL_SOURCES
    ${SHADERS_DIR}/SkyBox.hlsl
    ${SHADERS_DIR}/ScreenQuad.hlsl
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

target_link_libraries(${TARGET}
    PUBLIC
        MethaneGraphicsRhi
        MethaneGraphicsMesh
        MethaneDataPrimitives
        MethaneDataTypes
        MethaneInstrumentation
        TaskFlow
    PRIVATE
        MethaneBuildOptions
        MethanePrecompiledExtraHeaders
        MethanePlatformUtils
        MethaneGraphicsPrimitives
        MethaneGraphicsCamera
        MethaneDataProvider
        magic_enum
)

target_precompile_headers(${TARGET} REUSE_FROM MethanePrecompiledExtraHeaders)

if (METHANE_OPEN_IMAGE_IO_ENABLED)
    target_link_libraries(${TARGET} PRIVATE OpenImageIO)
    target_compile_definitions(${TARGET}
        PRIVATE
            USE_OPEN_IMAGE_IO
    )
    set_target_properties(${TARGET}
        PROPERTIES
            PREREQUISITE_MODULES "OpenImageIO"
    )
else()
    target_link_libraries(${TARGET} PRIVATE STB)
endif()

target_include_directories(${TARGET}
    PRIVATE
        Sources
    PUBLIC
        Include
        Shaders
)

add_methane_shaders_source(
    TARGET ${TARGET}
    SOURCE Shaders/ScreenQuad.hlsl
    VERSION 6_0
    TYPES
    "vert=QuadVS"
    "frag=QuadPS"
    "frag=QuadPS:TTEXEL=float,RMASK=r,WMASK=a"
    "frag=QuadPS:TEXTURE_DISABLED"
)

add_methane_shaders_source(
    TARGET ${TARGET}
    SOURCE Shaders/SkyBox.hlsl
    VERSION 6_0
    TYPES
    frag=SkyboxPS
    vert=SkyboxVS
)

add_methane_shaders_library(${TARGET})

# Disable GCC/Clang warnings produced by external code from 'stb_image.h'
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set_source_files_properties(${SOURCES_DIR}/ImageLoader.cpp
        PROPERTIES
            COMPILE_FLAGS "-Wno-sign-compare -Wno-unused-but-set-variable"
    )
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
    CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    set_source_files_properties(${SOURCES_DIR}/ImageLoader.cpp
        PROPERTIES
        COMPILE_FLAGS "-Wno-sign-compare"
    )
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Graphics
        PUBLIC_HEADER "${HEADERS}"
)

install(TARGETS ${TARGET}
    PUBLIC_HEADER
        DESTINATION ${INCLUDE_DIR}
        COMPONENT Development
    ARCHIVE
        DESTINATION Lib
        COMPONENT Development
)
