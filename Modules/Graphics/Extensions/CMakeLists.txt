set(TARGET MethaneGraphicsExtensions)

include(MethaneShaders)

get_module_dirs("Methane/Graphics")

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
)

set(HLSL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/SkyBox.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/ScreenQuad.hlsl
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

add_methane_shaders(${TARGET} "${HLSL_SOURCES}" "6_0")

target_link_libraries(${TARGET}
    PUBLIC
        MethaneGraphicsCore
        MethaneDataPrimitives
        MethaneInstrumentation
        TaskFlow
    PRIVATE
        MethaneBuildOptions
        MethanePrecompiledExtraHeaders
        MethanePlatformUtils
        MethaneGraphicsPrimitives
        MethaneGraphicsCamera
        MethaneDataProvider
)

target_precompile_headers(${TARGET} REUSE_FROM MethanePrecompiledExtraHeaders)

if (METHANE_USE_OPEN_IMAGE_IO)
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
)

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
