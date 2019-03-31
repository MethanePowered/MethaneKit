set(TARGET MethanePlatformUtils)

include(Methane)

get_module_dirs("Methane/Platform")

set(HEADERS
    ${INCLUDE_DIR}/Utils.h
    ${INCLUDE_PLATFORM_DIR}/Utils.h
)

set(SOURCES
    ${SOURCES_DIR}/Utils.cpp
    ${SOURCES_PLATFORM_DIR}/Utils.${CPP_EXT}
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
        nowide_static
)

if(APPLE)

    target_sources(${TARGET}
        PRIVATE
            ${INCLUDE_PLATFORM_DIR}/Types.h
    )

    set(HEADERS ${HEADERS}
        ${INCLUDE_PLATFORM_DIR}/Types.h
    )

    target_link_libraries(${TARGET}
        PUBLIC
        "-framework Cocoa"
    )

endif()

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
