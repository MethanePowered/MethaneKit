include(MethaneApplications)

set(TARGET MethaneTypography)
set(SOURCES
    TypographyApp.h
    TypographyApp.cpp
    TypographyAppController.h
    TypographyAppController.cpp
)
set(FONTS
    ${RESOURCES_DIR}/Fonts/Roboto/Roboto-Regular.ttf
    ${RESOURCES_DIR}/Fonts/Playball/Playball-Regular.ttf
    ${RESOURCES_DIR}/Fonts/SawarabiMincho/SawarabiMincho-Regular.ttf
#   ${RESOURCES_DIR}/Fonts/Amiri/Amiri-Regular.ttf
)

add_methane_application(${TARGET}
    "${SOURCES}"
    "${RESOURCES_DIR}"
    "Apps"
    "Methane Typography"
    "Dynamic text rendering and fonts management tutorial with Methane Kit."
    "${METHANE_COPYRIGHT}"
    "${METHANE_VERSION_SHORT}"
    "${METHANE_VERSION_BUILD}"
)

add_methane_embedded_fonts(${TARGET} "${RESOURCES_DIR}" "${FONTS}")

target_link_libraries(${TARGET}
    PRIVATE
        MethaneAppsCommon
)

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Apps/Tutorials
)
