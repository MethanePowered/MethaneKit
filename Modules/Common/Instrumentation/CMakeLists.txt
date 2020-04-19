set(TARGET MethaneInstrumentation)

include(MethaneModules)

get_module_dirs("Methane")

set(HEADERS
    ${INCLUDE_DIR}/Instrumentation.h
    ${INCLUDE_DIR}/InstrumentMemoryAllocations.h
    ${INCLUDE_DIR}/IttApiHelper.h
    ${INCLUDE_DIR}/ScopeTimer.h
    ${INCLUDE_DIR}/ILogger.h
)

set(SOURCES
    ${SOURCES_DIR}/Instrumentation.cpp
    ${SOURCES_DIR}/InstrumentMemoryAllocations.cpp
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
        MethanePrimitives
        TracyInstrumentation
        ittnotify
        nowide_static
    PRIVATE
        MethaneBuildOptions
)

if (METHANE_TRACY_PROFILING_ENABLED)
    # Force include header with memory allocation operators override, implementing Tracy memory instrumentation
    set(FORCE_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${INCLUDE_DIR}/InstrumentMemoryAllocations.h)
    if (MSVC)
        target_compile_options(${TARGET} PUBLIC /FI "${FORCE_INCLUDE_PATH}")
    else()
        target_compile_options(${TARGET} PUBLIC -include "${FORCE_INCLUDE_PATH}")
    endif()
endif()

if (METHANE_SCOPE_TIMERS_ENABLED)
    target_compile_definitions(${TARGET} INTERFACE -DMETHANE_SCOPE_TIMERS_ENABLED)
endif()

if (METHANE_LOGGING_ENABLED)
    target_compile_definitions(${TARGET} INTERFACE -DMETHANE_LOGGING_ENABLED)
    target_link_libraries(${TARGET}
        PUBLIC
            MethanePlatformUtils
    )
endif()

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