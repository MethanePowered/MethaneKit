set(TARGET MethanePlatformApp)

include(MethaneModules)

get_module_dirs("Methane/Platform")

if (WIN32)

    list(APPEND PLATFORM_HEADERS
        ${INCLUDE_PLATFORM_OS_DIR}/AppWin.h
    )

    list(APPEND PLATFORM_SOURCES
        ${SOURCES_PLATFORM_OS_DIR}/AppWin.cpp
        ${SOURCES_PLATFORM_OS_DIR}/ConsoleStreams.h
        ${SOURCES_PLATFORM_OS_DIR}/ConsoleStreams.cpp
    )

    list(APPEND PLATFORM_LIBRARIES
        Shcore
    )

elseif(APPLE)

    if (APPLE_MACOS)
        list(APPEND PLATFORM_HEADERS
            ${INCLUDE_PLATFORM_OS_DIR}/AppMac.hh
            ${SOURCES_PLATFORM_OS_DIR}/WindowDelegate.hh
        )
        list(APPEND PLATFORM_SOURCES
            ${SOURCES_PLATFORM_OS_DIR}/AppMac.mm
            ${SOURCES_PLATFORM_OS_DIR}/WindowDelegate.mm
        )
        list(APPEND PLATFORM_LIBRARIES "-framework AppKit")
    else() # iOS
        list(APPEND PLATFORM_HEADERS
            ${INCLUDE_PLATFORM_OS_DIR}/AppIOS.hh
        )
        list(APPEND PLATFORM_SOURCES
            ${SOURCES_PLATFORM_OS_DIR}/AppIOS.mm
        )
        list(APPEND PLATFORM_LIBRARIES "-framework UIKit")
    endif() # APPLE_MACOS

    list(APPEND PLATFORM_HEADERS
        ${SOURCES_PLATFORM_OS_DIR}/AppDelegate.hh
        ${SOURCES_PLATFORM_OS_DIR}/AppViewController.hh
    )
    list(APPEND PLATFORM_SOURCES
        ${SOURCES_PLATFORM_OS_DIR}/AppDelegate.mm
        ${SOURCES_PLATFORM_OS_DIR}/AppViewController.mm
    )

else(LINUX)

    list(APPEND PLATFORM_HEADERS
        ${INCLUDE_PLATFORM_OS_DIR}/AppLin.h
    )

    list(APPEND PLATFORM_SOURCES
        ${SOURCES_PLATFORM_OS_DIR}/AppLin.cpp
        ${SOURCES_PLATFORM_OS_DIR}/MessageBox.h
        ${SOURCES_PLATFORM_OS_DIR}/MessageBox.cpp
        ${SOURCES_PLATFORM_OS_DIR}/XcbUtils.h
        ${SOURCES_PLATFORM_OS_DIR}/XcbUtils.cpp
    )

    # TODO: replace "xcb-randr" with "X11::xcb_randr" when supported by CMake 3.24
    set(PLATFORM_LIBRARIES X11::X11 X11::X11_xcb X11::xcb xcb-randr xcb-sync STB)

endif()

list(APPEND HEADERS ${PLATFORM_HEADERS}
    ${INCLUDE_DIR}/IApp.h
    ${INCLUDE_DIR}/App.h
    ${INCLUDE_DIR}/AppBase.h
    ${INCLUDE_DIR}/AppController.h
)

list(APPEND SOURCES ${PLATFORM_SOURCES}
    ${SOURCES_DIR}/IApp.cpp
    ${SOURCES_DIR}/AppBase.cpp
    ${SOURCES_DIR}/AppController.cpp
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

target_link_libraries(${TARGET}
    PUBLIC
        CLI11
        MethanePrimitives
        MethaneInstrumentation
        MethaneDataTypes
        MethanePlatformAppView
        MethanePlatformInputActionControllers
    PRIVATE
        MethaneBuildOptions
        MethaneCommonPrecompiledHeaders
        MethanePlatformUtils
        MethaneDataProvider
        TaskFlow
        fmt
        nowide
        ${PLATFORM_LIBRARIES}
)

target_compile_definitions(${TARGET}
    PRIVATE
        METHANE_RENDER_APP
)

if(METHANE_PRECOMPILED_HEADERS_ENABLED)
    target_precompile_headers(${TARGET} REUSE_FROM MethaneCommonPrecompiledHeaders)
endif()

target_include_directories(${TARGET}
    PRIVATE
        Sources
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
        ${PLATFORM_SOURCES}
        PROPERTIES
            COMPILE_FLAGS -fobjc-arc
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

