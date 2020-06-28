set(TARGET MethaneUserInterfaceWidgets)

include(MethaneResources)

get_module_dirs("Methane/UserInterface")

set(HEADERS
    ${INCLUDE_DIR}/Widgets.h
    ${INCLUDE_DIR}/Badge.h
    ${INCLUDE_DIR}/HeadsUpDisplay.h
)

set(SOURCES
    ${SOURCES_DIR}/Badge.cpp
    ${SOURCES_DIR}/HeadsUpDisplay.cpp
)

set(IMAGES_DIR ${RESOURCES_DIR}/Images)
set(TEXTURES
    ${IMAGES_DIR}/Logo/MethaneLogoNameWatermark.png
)

set(FONTS
    ${RESOURCES_DIR}/Fonts/RobotoMono/RobotoMono-Regular.ttf
    ${RESOURCES_DIR}/Fonts/RobotoMono/RobotoMono-Bold.ttf
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

add_methane_embedded_textures(${TARGET} "${IMAGES_DIR}" "${TEXTURES}")
add_methane_embedded_fonts(${TARGET} "${RESOURCES_DIR}" "${FONTS}")

target_link_libraries(${TARGET}
    PUBLIC
        MethaneUserInterfaceTypography
        MethaneGraphicsExtensions
    PRIVATE
        MethaneBuildOptions
        MethaneGraphicsPrimitives
        MethaneInstrumentation
)

target_include_directories(${TARGET}
    PRIVATE
        Sources
    PUBLIC
        Include
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Modules/UserInterface
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

