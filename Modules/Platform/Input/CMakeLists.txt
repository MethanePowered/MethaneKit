set(TARGET MethanePlatformInput)

include(Methane)

get_module_dirs("Methane/Platform")

if (WIN32)

    set(PLATFORM_HEADERS
        #${INCLUDE_PLATFORM_DIR}/
    )

    set(PLATFORM_SOURCES
        #${SOURCES_PLATFORM_DIR}/
    )

elseif(APPLE)

    set(PLATFORM_HEADERS
        #${INCLUDE_PLATFORM_DIR}/
    )

    set(PLATFORM_SOURCES
        #${SOURCES_PLATFORM_DIR}/
    )

    set(LIBRARIES
        "-framework AppKit"
    )

endif()

set(LIBRARIES ${LIBRARIES}
    MethaneDataTypes
)

set(HEADERS ${PLATFORM_HEADERS}
    ${INCLUDE_DIR}/Keyboard.h
    ${INCLUDE_DIR}/Mouse.h
)

set(SOURCES ${PLATFORM_SOURCES}
    ${SOURCES_DIR}/Keyboard.cpp
    ${SOURCES_DIR}/Mouse.cpp
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

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

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