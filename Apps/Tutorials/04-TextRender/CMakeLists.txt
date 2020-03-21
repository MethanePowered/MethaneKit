include(MethaneApplications)

set(TARGET MethaneTextRender)
set(SOURCES
    TextRenderApp.h
    TextRenderApp.cpp
)
set(FONTS
    ${RESOURCES_DIR}/Fonts/Roboto/Roboto-Regular.ttf
    ${RESOURCES_DIR}/Fonts/Playball/Playball-Regular.ttf
)

add_methane_application(${TARGET}
    "${SOURCES}"
    "${RESOURCES_DIR}"
    "Apps/Tutorials"
    "Methane Text Render"
    "Text rendering tutorial with Methane Kit."
    "${METHANE_COPYRIGHT}"
    "${METHANE_VERSION_SHORT}"
    "${METHANE_VERSION_BUILD}"
)

add_methane_embedded_fonts(${TARGET} ${RESOURCES_DIR} ${FONTS})

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Apps/Tutorials
)
