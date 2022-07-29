set(TARGET MethaneAppsCommon)

get_module_dirs("Methane/Samples")

set(HEADERS
    ${INCLUDE_DIR}/AppSettings.hpp
    ${INCLUDE_DIR}/TextureLabeler.h
)

set(SOURCES
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
        MethaneBuildOptions
        magic_enum
    PRIVATE
        MethaneUserInterfaceTypography
)

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Apps
)