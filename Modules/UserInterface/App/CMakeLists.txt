set(TARGET MethaneUserInterfaceApp)

include(MethaneResources)

get_module_dirs("Methane/UserInterface")

set(HEADERS
    ${INCLUDE_DIR}/App.h
    ${INCLUDE_DIR}/App.hpp
    ${INCLUDE_DIR}/AppBase.h
    ${INCLUDE_DIR}/AppController.h
)

set(SOURCES
    ${SOURCES_DIR}/AppBase.cpp
    ${SOURCES_DIR}/AppController.cpp
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

target_include_directories(${TARGET}
    PRIVATE
        Sources
    PUBLIC
        Include
)

target_link_libraries(${TARGET}
PUBLIC
    MethaneGraphicsApp
    MethaneUserInterfaceWidgets
    MethaneInstrumentation
PRIVATE
    MethaneBuildOptions
)

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Modules/UserInterface
    PUBLIC_HEADER "${HEADERS}"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

install(TARGETS ${TARGET}
    PUBLIC_HEADER
        DESTINATION ${INCLUDE_DIR}
        COMPONENT Development
    ARCHIVE
        DESTINATION Lib
        COMPONENT Development
)
