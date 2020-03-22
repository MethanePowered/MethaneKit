set(TARGET MethaneGraphicsExtensions)

include(MethaneShaders)
include(MethaneResources)

get_module_dirs("Methane/Graphics")

set(HEADERS
    ${INCLUDE_DIR}/Extensions.h
    ${INCLUDE_DIR}/ImageLoader.h
    ${INCLUDE_DIR}/MeshBuffers.hpp
    ${INCLUDE_DIR}/SkyBox.h
    ${INCLUDE_DIR}/ScreenQuad.h
    ${INCLUDE_DIR}/Badge.h
    ${INCLUDE_DIR}/Font.h
    ${INCLUDE_DIR}/Text.h
)

set(SOURCES
    ${SOURCES_DIR}/ImageLoader.cpp
    ${SOURCES_DIR}/SkyBox.cpp
    ${SOURCES_DIR}/ScreenQuad.cpp
    ${SOURCES_DIR}/Badge.cpp
    ${SOURCES_DIR}/Font.cpp
    ${SOURCES_DIR}/Text.cpp
)

set(HLSL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/SkyBox.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/ScreenQuad.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Text.hlsl
)

set(IMAGES_DIR ${RESOURCES_DIR}/Images)
set(TEXTURES
    ${IMAGES_DIR}/Logo/MethaneLogoNameWatermark.png
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

add_methane_shaders(${TARGET} "${HLSL_SOURCES}" "6_0")
add_methane_embedded_textures(${TARGET} "${IMAGES_DIR}" "${TEXTURES}")

target_link_libraries(${TARGET}
    PUBLIC
        MethaneDataPrimitives
    PRIVATE
        MethaneBuildOptions
        MethaneCommonPrimitives
        MethanePlatformUtils
        MethaneGraphicsCore
        MethaneDataProvider
        MethaneInstrumentation
        freetype
)

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

