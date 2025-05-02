set(TARGET MethaneGraphicsMesh)

include(MethaneModules)

get_module_dirs("Methane/Graphics")

set(HEADERS
    ${INCLUDE_DIR}/Mesh.h
    ${INCLUDE_DIR}/BaseMesh.hpp
    ${INCLUDE_DIR}/QuadMesh.hpp
    ${INCLUDE_DIR}/CubeMesh.hpp
    ${INCLUDE_DIR}/UberMesh.hpp
    ${INCLUDE_DIR}/SphereMesh.hpp
    ${INCLUDE_DIR}/IcosahedronMesh.hpp
)

set(SOURCES
    ${SOURCES_DIR}/Mesh.cpp
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

if(METHANE_PRECOMPILED_HEADERS_ENABLED)
    target_precompile_headers(${TARGET} REUSE_FROM MethaneMathPrecompiledHeaders)
endif()

target_link_libraries(${TARGET}
    PUBLIC
        MethaneGraphicsTypes
        MethaneInstrumentation
        magic_enum
    PRIVATE
        MethaneBuildOptions
        MethaneMathPrecompiledHeaders
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Graphics
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
