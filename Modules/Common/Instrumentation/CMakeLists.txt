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
    $<$<BOOL:${METHANE_TRACY_PROFILING_ENABLED}>:${SOURCES_DIR}/InstrumentMemoryAllocations.cpp>
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
        $<$<BOOL:${METHANE_LOGGING_ENABLED}>:MethanePlatformUtils>
        MethanePrimitives
        TracyInstrumentation
        ittnotify
        nowide_static
    PRIVATE
        MethaneBuildOptions
)

target_compile_definitions(${TARGET}
    PUBLIC
        $<$<BOOL:${METHANE_SCOPE_TIMERS_ENABLED}>:METHANE_SCOPE_TIMERS_ENABLED>
        $<$<BOOL:${METHANE_LOGGING_ENABLED}>:METHANE_LOGGING_ENABLED>
        # Uncomment to enable call-stacks collection (increases profiling overhead)
        # $<$<BOOL:${METHANE_TRACY_PROFILING_ENABLED}>:TRACY_ZONE_CALL_STACK_DEPTH=16>
        $<$<BOOL:${METHANE_TRACY_PROFILING_ENABLED}>:TRACY_MEMORY_CALL_STACK_DEPTH=16>
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Modules/Common
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