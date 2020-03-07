include(MethaneApplications)

set(TARGET MethaneTextRender)
set(SOURCES
    TextRenderApp.h
    TextRenderApp.cpp
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

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Apps/Tutorials
)
