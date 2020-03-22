set(TARGET MethaneDataTypes)

include(MethaneModules)

get_module_dirs("Methane/Data")

set(HEADERS
    ${INCLUDE_DIR}/Types.h
    ${INCLUDE_DIR}/Point.hpp
    ${INCLUDE_DIR}/Rect.hpp
    ${INCLUDE_DIR}/Chunk.hpp
)

set(SOURCES
    ${SOURCES_DIR}/Types.cpp
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

target_include_directories(${TARGET}
    PRIVATE
        Sources
    PUBLIC
        Include
)

target_link_libraries(${TARGET}
    PUBLIC
        CML
    PRIVATE
        MethaneBuildOptions
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES  ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Modules/Data
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
