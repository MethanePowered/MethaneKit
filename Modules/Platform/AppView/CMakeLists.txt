set(TARGET MethanePlatformAppView)

include(MethaneModules)

get_module_dirs("Methane/Platform")

if (WIN32)

    set(PLATFORM_HEADERS
        ${INCLUDE_PLATFORM_DIR}/AppEnvironment.h
    )

elseif(APPLE)

    set(PLATFORM_HEADERS
        ${INCLUDE_PLATFORM_DIR}/AppViewMT.hh
        ${INCLUDE_PLATFORM_DIR}/AppEnvironment.hh
    )

    set(PLATFORM_SOURCES
        ${SOURCES_PLATFORM_DIR}/AppViewMT.mm
    )

    set(LIBRARIES
        "-framework AppKit \
         -framework Metal \
         -framework QuartzCore"
    )

    # CLANG_ENABLE_OBJC_WEAK = YES
    set_source_files_properties(
        ${PLATFORM_SOURCES}
        PROPERTIES
        COMPILE_FLAGS -fobjc-weak
    )

elseif() # Linux

    set(PLATFORM_HEADERS
        ${INCLUDE_PLATFORM_DIR}/AppEnvironment.h
    )

endif()

set(HEADERS ${PLATFORM_HEADERS}
    ${INCLUDE_DIR}/AppView.h
    ${INCLUDE_DIR}/AppEnvironment.h
)

set(SOURCES ${PLATFORM_SOURCES}
    ${SOURCES_DIR}/AppView.cpp
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

target_link_libraries(${TARGET}
    ${LIBRARIES}
    MethaneInstrumentation
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
