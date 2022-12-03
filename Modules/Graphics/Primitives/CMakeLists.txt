set(TARGET MethaneGraphicsPrimitives)

include(MethaneModules)

get_module_dirs("Methane/Graphics")

list(APPEND HEADERS
    ${INCLUDE_DIR}/FpsCounter.h
)

list(APPEND SOURCES
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
        MethaneDataAnimation
        MethaneInstrumentation
    PRIVATE
        MethaneBuildOptions
        MethanePrecompiledExtraHeaders
        MethanePlatformUtils
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
