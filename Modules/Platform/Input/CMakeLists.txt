set(TARGET MethanePlatformInput)

include(MethaneModules)

get_module_dirs("Methane/Platform")

if (APPLE_MACOS)
    set(PLATFORM_LIBRARIES
        "-framework AppKit"
    )
endif()

set(HEADERS
    ${INCLUDE_PLATFORM_DIR}/Keyboard.h
    ${INCLUDE_DIR}/Keyboard.h
    ${INCLUDE_DIR}/Mouse.h
    ${INCLUDE_DIR}/KeyboardActionControllerBase.hpp
    ${INCLUDE_DIR}/MouseActionControllerBase.hpp
    ${INCLUDE_DIR}/Input/HelpProvider.h
    ${INCLUDE_DIR}/Input/Controller.h
    ${INCLUDE_DIR}/Input/ControllersPool.h
    ${INCLUDE_DIR}/Input/State.h
)

set(SOURCES
    $<$<BOOL:${APPLE_MACOS}>:${SOURCES_PLATFORM_DIR}/Keyboard.${CPP_EXT}>
    ${SOURCES_DIR}/Keyboard.cpp
    ${SOURCES_DIR}/Mouse.cpp
    ${SOURCES_DIR}/Input/ControllersPool.cpp
    ${SOURCES_DIR}/Input/State.cpp
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

target_link_libraries(${TARGET}
    PUBLIC
        MethaneDataTypes
        MethaneInstrumentation
    PRIVATE
        MethaneBuildOptions
        MethanePrecompiledHeaders
        MethanePrimitives
        MethanePlatformUtils
        magic_enum
        ${PLATFORM_LIBRARIES}
)

target_precompile_headers(${TARGET} REUSE_FROM MethanePrecompiledHeaders)

target_include_directories(${TARGET}
    PRIVATE
        Sources
        $<$<BOOL:${LINUX}>:${X11_xcb_INCLUDE_PATH} ${X11_X11_INCLUDE_PATH}>
    PUBLIC
        Include
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
        ${SOURCES_PLATFORM_DIR}/Keyboard.${CPP_EXT}
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
