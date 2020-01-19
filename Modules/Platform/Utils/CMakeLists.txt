set(TARGET MethanePlatformUtils)

include(MethaneModules)

get_module_dirs("Methane/Platform")

if(WIN32)
    set(PLATFORM_HEADERS
        ${INCLUDE_PLATFORM_DIR}/Utils.h
    )
elseif(APPLE)
    set(PLATFORM_HEADERS
        ${INCLUDE_PLATFORM_DIR}/Types.hh
        ${INCLUDE_PLATFORM_DIR}/Utils.hh
    )
endif()

set(HEADERS
    ${INCLUDE_DIR}/Utils.h
    ${INCLUDE_DIR}/Logger.h
    ${PLATFORM_HEADERS}
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
        MethaneInstrumentation
        nowide_static
)

if(APPLE)

    target_sources(${TARGET}
        PRIVATE
            ${INCLUDE_PLATFORM_DIR}/Types.hh
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
