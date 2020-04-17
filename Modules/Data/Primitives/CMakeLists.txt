set(TARGET MethaneDataPrimitives)

include(MethaneModules)

get_module_dirs("Methane/Data")

set(HEADERS
    ${INCLUDE_DIR}/AlignedAllocator.hpp
    ${INCLUDE_DIR}/Math.hpp
    ${INCLUDE_DIR}/Parallel.hpp
    ${INCLUDE_DIR}/RectBinPack.hpp
)

set(SOURCES
    ${SOURCES_DIR}/Primitives.cpp
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
        MethaneInstrumentation
    PRIVATE
        MethaneBuildOptions
        MethaneDataTypes
)

if (UNIX)

    target_link_libraries(${TARGET}
        PRIVATE
            pthread
    )

endif()

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
