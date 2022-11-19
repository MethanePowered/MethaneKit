set(TARGET MethaneAppsCommon)

get_module_dirs("Methane/Tutorials")

set(HEADERS
    ${INCLUDE_DIR}/AppSettings.h
    ${INCLUDE_DIR}/TextureLabeler.h
)

set(SOURCES
    ${SOURCES_DIR}/AppSettings.cpp
    ${SOURCES_DIR}/TextureLabeler.cpp
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

target_include_directories(${TARGET}
    PUBLIC
        Include
)

target_link_libraries(${TARGET}
    PUBLIC
        MethaneGraphicsApp
        MethaneUserInterfaceApp
        MethaneBuildOptions
    PRIVATE
        MethaneUserInterfaceTypography
        magic_enum
)

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Apps
)