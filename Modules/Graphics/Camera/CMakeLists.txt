set(TARGET MethaneGraphicsCamera)

include(MethaneModules)

get_module_dirs("Methane/Graphics")

set(HEADERS
    ${INCLUDE_DIR}/Camera.h
    ${INCLUDE_DIR}/ArcBallCamera.h
    ${INCLUDE_DIR}/ActionCamera.h
)

set(SOURCES
    ${SOURCES_DIR}/Camera.cpp
    ${SOURCES_DIR}/ArcBallCamera.cpp
    ${SOURCES_DIR}/ActionCamera.cpp
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
        MethaneDataAnimation
        MethaneGraphicsTypes
    PRIVATE
        MethaneBuildOptions
        MethaneMathPrecompiledHeaders
        MethaneInstrumentation
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Graphics
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
