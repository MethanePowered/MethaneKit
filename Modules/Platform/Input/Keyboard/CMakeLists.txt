set(TARGET MethanePlatformInputKeyboard)

include(MethaneModules)

get_module_dirs("Methane/Platform/Input")

list(APPEND HEADERS ${INCLUDE_DIR}/Keyboard.h)
list(APPEND SOURCES ${SOURCES_DIR}/Keyboard.cpp)

if (APPLE_MACOS)
    list(APPEND HEADERS ${INCLUDE_PLATFORM_OS_DIR}/Keyboard.h)
    list(APPEND PLATFORM_LIBRARIES "-framework AppKit")
endif()

if (NOT (APPLE_IOS OR APPLE_TVOS))
    list(APPEND HEADERS ${INCLUDE_PLATFORM_OS_DIR}/Keyboard.h)
    list(APPEND SOURCES ${SOURCES_PLATFORM_OS_DIR}/Keyboard.${CPP_EXT})
endif()

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
        MethaneCommonPrecompiledHeaders
        MethanePrimitives
        MethanePlatformUtils
        magic_enum
        ${PLATFORM_LIBRARIES}
)

if(METHANE_PRECOMPILED_HEADERS_ENABLED)
    target_precompile_headers(${TARGET} REUSE_FROM MethaneCommonPrecompiledHeaders)
endif()

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
        FOLDER Modules/Platform/Input
        PUBLIC_HEADER "${HEADERS}"
)

if (APPLE)
    # Disable precompiled headers on MacOS for Objective-C files:
    set_source_files_properties(
        ${SOURCES_PLATFORM_OS_DIR}/Keyboard.mm
        PROPERTIES
            COMPILE_FLAGS -fobjc-arc
            SKIP_PRECOMPILE_HEADERS ON
    )
endif()

install(TARGETS ${TARGET}
    PUBLIC_HEADER
        DESTINATION ${INSTALL_INCLUDE_DIR}
        COMPONENT Development
    ARCHIVE
        DESTINATION lib
        COMPONENT Development
)
