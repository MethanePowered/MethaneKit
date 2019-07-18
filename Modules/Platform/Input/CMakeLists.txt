set(TARGET MethanePlatformInput)

include(Methane)

get_module_dirs("Methane/Platform")

if (WIN32)

    set(PLATFORM_HEADERS
        ${INCLUDE_PLATFORM_DIR}/KeyboardWin.h
    )

    set(PLATFORM_SOURCES
        ${SOURCES_PLATFORM_DIR}/KeyboardWin.cpp
    )

elseif(APPLE)

    set(PLATFORM_HEADERS
        ${INCLUDE_PLATFORM_DIR}/KeyboardMac.h
    )

    set(PLATFORM_SOURCES
        ${SOURCES_PLATFORM_DIR}/KeyboardMac.mm
    )

    set(LIBRARIES
        "-framework AppKit"
    )

endif()

set(LIBRARIES ${LIBRARIES}
    MethaneDataTypes
    MethanePlatformUtils
)

set(HEADERS ${PLATFORM_HEADERS}
    ${INCLUDE_DIR}/Keyboard.h
    ${INCLUDE_DIR}/Mouse.h
    ${INCLUDE_DIR}/Input/Controller.h
    ${INCLUDE_DIR}/Input/ControllersPool.h
    ${INCLUDE_DIR}/Input/State.h
)

set(SOURCES ${PLATFORM_SOURCES}
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
    ${LIBRARIES}
)

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

install(TARGETS ${TARGET}
    PUBLIC_HEADER
        DESTINATION ${INCLUDE_DIR}
        COMPONENT Development
    ARCHIVE
        DESTINATION Lib
        COMPONENT Development
)

add_subdirectory(Test)
