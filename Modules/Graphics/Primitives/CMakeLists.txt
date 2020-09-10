set(TARGET MethaneGraphicsPrimitives)

include(MethaneModules)

get_module_dirs("Methane/Graphics")

set(HEADERS
    ${INCLUDE_DIR}/Primitives.h
    ${INCLUDE_DIR}/FpsCounter.h
    ${INCLUDE_DIR}/PerlinNoise.h
    ${INCLUDE_DIR}/Mesh.h
    ${INCLUDE_DIR}/Mesh/BaseMesh.hpp
    ${INCLUDE_DIR}/Mesh/QuadMesh.hpp
    ${INCLUDE_DIR}/Mesh/CubeMesh.hpp
    ${INCLUDE_DIR}/Mesh/UberMesh.hpp
    ${INCLUDE_DIR}/Mesh/SphereMesh.hpp
    ${INCLUDE_DIR}/Mesh/IcosahedronMesh.hpp
    ${INCLUDE_PLATFORM_DIR}/Primitives.h
)

set(SOURCES
    ${SOURCES_DIR}/FpsCounter.cpp
    ${SOURCES_DIR}/Mesh.cpp
    ${SOURCES_DIR}/PerlinNoise.cpp
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
        MethaneDataAnimation
        MethaneGraphicsTypes
        MethaneInstrumentation
    PRIVATE
        MethaneBuildOptions
        MethanePlatformUtils
        PerlinNoise
        CML
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
