set(TARGET MethaneInstrumentation)

include(MethaneModules)

get_module_dirs("Methane")

set(HEADERS
    ${INCLUDE_DIR}/Instrumentation.h
    ${INCLUDE_DIR}/IttApiHelper.h
    ${INCLUDE_DIR}/ScopeTimer.h
    ${INCLUDE_DIR}/ILogger.h
    ${INCLUDE_DIR}/TracyGpu.hpp
)

set(SOURCES
    ${SOURCES_PLATFORM_DIR}/Instrumentation.${CPP_EXT}
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
        $<$<BOOL:${METHANE_LOGGING_ENABLED}>:MethanePlatformUtils> # Logging functions
        MethanePrimitives
        TracyInstrumentation
        ittnotify
        nowide
    PRIVATE
        MethaneBuildOptions
)

if(APPLE)
    target_link_libraries(${TARGET}
        PRIVATE
            "-framework Foundation"
    )
endif()

target_compile_definitions(${TARGET}
    PUBLIC
        $<$<BOOL:${METHANE_SCOPE_TIMERS_ENABLED}>:METHANE_SCOPE_TIMERS_ENABLED>
        $<$<BOOL:${METHANE_LOGGING_ENABLED}>:METHANE_LOGGING_ENABLED>
        # Tracy configuration
        $<$<BOOL:${METHANE_TRACY_PROFILING_ON_DEMAND}>:TRACY_ON_DEMAND>
        $<$<BOOL:${METHANE_TRACY_PROFILING_ENABLED}>:TRACY_ENABLE>
        $<$<AND:$<BOOL:${METHANE_TRACY_PROFILING_ENABLED}>,$<BOOL:${METHANE_GPU_INSTRUMENTATION_ENABLED}>>:TRACY_GPU_ENABLE>
        # Call stacks collection does not work on MacOS
        # $<$<BOOL:${METHANE_TRACY_PROFILING_ENABLED}>:TRACY_MEMORY_CALL_STACK_DEPTH=16> # call stacks for memory new/delete
        # $<$<BOOL:${METHANE_TRACY_PROFILING_ENABLED}>:TRACY_ZONE_CALL_STACK_DEPTH=16> # call stacks for each zone
)

# TracyClient interface target is defined separately in Externals,
# but it also has to be configured with TRACY_* macro definitions
# and is required for linking in target application for Tracy profiling
target_compile_definitions(TracyClient
    INTERFACE
        $<$<BOOL:${METHANE_TRACY_PROFILING_ENABLED}>:TRACY_ENABLE>
        $<$<BOOL:${METHANE_TRACY_PROFILING_ON_DEMAND}>:TRACY_ON_DEMAND>
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