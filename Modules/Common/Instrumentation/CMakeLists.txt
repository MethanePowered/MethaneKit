set(TARGET MethaneInstrumentation)

include(MethaneModules)

get_module_dirs("Methane")

set(HEADERS
    ${INCLUDE_DIR}/Instrumentation.h
    ${INCLUDE_DIR}/IttApiHelper.h
    ${INCLUDE_DIR}/ScopeTimer.h
    ${INCLUDE_DIR}/ILogger.h
)

set(SOURCES
    ${SOURCES_DIR}/Instrumentation.cpp
    ${SOURCES_DIR}/ScopeTimer.cpp
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
        MethaneCommonPrimitives
        ittnotify
        nowide_static
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

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