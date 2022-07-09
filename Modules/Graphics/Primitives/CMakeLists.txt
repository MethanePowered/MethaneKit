set(TARGET MethaneGraphicsPrimitives)

include(MethaneModules)

get_module_dirs("Methane/Graphics")

set(HEADERS
    ${INCLUDE_DIR}/FpsCounter.h
    ${INCLUDE_DIR}/PerlinNoise.h
)

set(SOURCES
    ${SOURCES_DIR}/FpsCounter.cpp
    ${SOURCES_DIR}/PerlinNoise.cpp
)

if (WIN32)
    set(HEADERS ${HEADERS}
        ${INCLUDE_PLATFORM_DIR}/ErrorHandling.h
    )
    set(SOURCES ${SOURCES}
        ${SOURCES_PLATFORM_DIR}/ErrorHandling.cpp
    )
endif()

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
        MethanePrecompiledExtraHeaders
        MethanePlatformUtils
        PerlinNoise
)

target_precompile_headers(${TARGET} REUSE_FROM MethanePrecompiledExtraHeaders)

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
