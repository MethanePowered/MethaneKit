set(TARGET MethaneGraphicsHelpers)

include(MethaneModules)

get_module_dirs("Methane/Graphics")

set(HEADERS
    ${INCLUDE_DIR}/Helpers.h
    ${INCLUDE_DIR}/Camera.h
    ${INCLUDE_DIR}/ArcBallCamera.h
    ${INCLUDE_DIR}/ActionCamera.h
    ${INCLUDE_DIR}/FpsCounter.h
    ${INCLUDE_DIR}/Noise.hpp
    ${INCLUDE_DIR}/MathTypes.h
    ${INCLUDE_DIR}/Mesh.h
    ${INCLUDE_DIR}/Mesh/BaseMesh.hpp
    ${INCLUDE_DIR}/Mesh/QuadMesh.hpp
    ${INCLUDE_DIR}/Mesh/CubeMesh.hpp
    ${INCLUDE_DIR}/Mesh/UberMesh.hpp
    ${INCLUDE_DIR}/Mesh/SphereMesh.hpp
    ${INCLUDE_DIR}/Mesh/IcosahedronMesh.hpp
    ${INCLUDE_PLATFORM_DIR}/Helpers.h
    ${INCLUDE_PLATFORM_DIR}/MathTypes.h
)

set(SOURCES
    ${SOURCES_DIR}/Camera.cpp
    ${SOURCES_DIR}/ArcBallCamera.cpp
    ${SOURCES_DIR}/ActionCamera.cpp
    ${SOURCES_DIR}/FpsCounter.cpp
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

target_link_libraries(${TARGET}
    MethaneDataPrimitives
    MethaneDataAnimation
    MethanePlatformUtils
    MethaneInstrumentation
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

add_subdirectory(Test)
