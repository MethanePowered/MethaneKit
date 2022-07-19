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

set(PLATFORM_SOURCES
    ${SOURCES_PLATFORM_DIR}/Instrumentation.${CPP_EXT}
)

set(SOURCES
    ${PLATFORM_SOURCES}
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
        MethanePrimitives
        $<$<BOOL:${METHANE_LOGGING_ENABLED}>:MethanePlatformUtils> # Logging functions
        $<$<BOOL:${METHANE_ITT_INSTRUMENTATION_ENABLED}>:ittnotify>
        TracyInstrumentation
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
)

# Tracy call stacks collection: enabled on Windows only, does not work on MacOS (reasons unknown)
#  - it is mostly useful for memory profiling to know where each new/delete comes from
#  - zone call stacks can be collected in sampling mode when client application is started under Administrator
if (WIN32 AND METHANE_TRACY_PROFILING_ENABLED)
    target_compile_definitions(${TARGET}
        PUBLIC
        TRACY_MEMORY_CALL_STACK_DEPTH=16 # call stacks for memory new/delete
        # TRACY_ZONE_CALL_STACK_DEPTH=16 # call stacks for each zone
    )
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

if (APPLE)
    # Disable precompiled headers on MacOS for Objective-C files:
    set_source_files_properties(
        ${PLATFORM_SOURCES}
        PROPERTIES
            SKIP_PRECOMPILE_HEADERS ON
    )
endif()

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
