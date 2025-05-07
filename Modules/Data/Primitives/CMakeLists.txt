set(TARGET MethaneDataPrimitives)

include(MethaneModules)

get_module_dirs("Methane/Data")

set(HEADERS
    ${INCLUDE_DIR}/AlignedAllocator.hpp
    ${INCLUDE_DIR}/RectBinPack.hpp
    ${INCLUDE_DIR}/IFpsCounter.h
    ${INCLUDE_DIR}/FpsCounter.h
)

set(SOURCES
    ${SOURCES_DIR}/Primitives.cpp
    ${SOURCES_DIR}/IFpsCounter.cpp
    ${SOURCES_DIR}/FpsCounter.cpp
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
        DESTINATION ${INSTALL_INCLUDE_DIR}
        COMPONENT Development
    ARCHIVE
        DESTINATION lib
        COMPONENT Development
)
