set(TARGET MethanePlatformAppView)

include(MethaneModules)

get_module_dirs("Methane/Platform")

if (WIN32)

    list(APPEND PLATFORM_HEADERS
        ${INCLUDE_PLATFORM_OS_DIR}/AppEnvironment.h
    )

elseif(APPLE)

    list(APPEND PLATFORM_HEADERS
        ${INCLUDE_PLATFORM_OS_DIR}/AppViewMT.hh
        ${INCLUDE_PLATFORM_OS_DIR}/AppEnvironment.hh
    )

    list(APPEND PLATFORM_SOURCES
        ${SOURCES_PLATFORM_OS_DIR}/AppViewMT.mm
    )

    list(APPEND PLATFORM_LIBRARIES
        "-framework $<IF:$<BOOL:${APPLE_MACOS}>,AppKit,UIKit>"
        "-framework Metal"
        "-framework QuartzCore"
    )

    # CLANG_ENABLE_OBJC_WEAK = YES
    set_source_files_properties(
        ${PLATFORM_SOURCES}
        PROPERTIES
            COMPILE_FLAGS -fobjc-weak
    )

elseif() # Linux

    list(APPEND PLATFORM_HEADERS
        ${INCLUDE_PLATFORM_OS_DIR}/AppEnvironment.h
    )

endif()

list(APPEND HEADERS
    ${PLATFORM_HEADERS}
    ${INCLUDE_DIR}/AppView.h
    ${INCLUDE_DIR}/AppEnvironment.h
)

list(APPEND SOURCES
    ${PLATFORM_SOURCES}
    ${SOURCES_DIR}/AppView.cpp
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

target_link_libraries(${TARGET}
    PRIVATE
        MethaneBuildOptions
        MethaneInstrumentation
        MethaneDataTypes
        ${PLATFORM_LIBRARIES}
)

target_include_directories(${TARGET}
    PRIVATE
        Sources
    PUBLIC
        Include
)

target_compile_definitions(${TARGET}
    PRIVATE
        $<$<BOOL:${METHANE_GPU_INSTRUMENTATION_ENABLED}>:METHANE_GPU_INSTRUMENTATION_ENABLED>
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Platform
        PUBLIC_HEADER "${HEADERS}"
)

if (APPLE)
    # Disable precompiled headers on MacOS for Objective-C files:
    set_source_files_properties(
        ${PLATFORM_SOURCES}
        PROPERTIES
            SKIP_PRECOMPILE_HEADERS ON
    )
endif()

install(TARGETS ${TARGET}
    PUBLIC_HEADER
        DESTINATION ${INCLUDE_DIR}
        COMPONENT Development
    ARCHIVE
        DESTINATION Lib
        COMPONENT Development
)

