include(MethaneApplications)

set(TARGET MethaneTypography)

add_methane_application(
    TARGET ${TARGET}
    NAME "Methane Typography"
    DESCRIPTION "Tutorial demonstrating dynamic text rendering and font atlases management with Methane Kit."
    INSTALL_DIR "Apps"
    SOURCES
        TypographyApp.h
        TypographyApp.cpp
        TypographyAppController.h
        TypographyAppController.cpp
)

set(FONTS
    ${RESOURCES_DIR}/Fonts/Roboto/Roboto-Regular.ttf
    ${RESOURCES_DIR}/Fonts/Playball/Playball-Regular.ttf
    ${RESOURCES_DIR}/Fonts/SawarabiMincho/SawarabiMincho-Regular.ttf
)
add_methane_embedded_fonts(${TARGET} "${RESOURCES_DIR}" "${FONTS}")

target_link_libraries(${TARGET}
    PRIVATE
        MethaneAppsCommon
)

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Apps
)
