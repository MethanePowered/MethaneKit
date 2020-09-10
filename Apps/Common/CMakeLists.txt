set(TARGET MethaneAppsCommon)

get_module_dirs("Methane/Samples")

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/${INCLUDE_DIR}/AppSettings.hpp
)

add_library(${TARGET} INTERFACE)

target_sources(${TARGET}
    INTERFACE
        ${HEADERS}
)

target_include_directories(${TARGET}
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Include
)

target_link_libraries(${TARGET}
    INTERFACE
        MethaneGraphicsApp
        MethaneBuildOptions
)