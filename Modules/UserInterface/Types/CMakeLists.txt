set(TARGET MethaneUserInterfaceTypes)

include(MethaneModules)

get_module_dirs("Methane/UserInterface")

set(HEADERS
    ${INCLUDE_DIR}/Context.h
    ${INCLUDE_DIR}/Item.h
    ${INCLUDE_DIR}/Container.h
    ${INCLUDE_DIR}/Types.hpp
)

set(SOURCES
    ${SOURCES_DIR}/Context.cpp
    ${SOURCES_DIR}/Item.cpp
    ${SOURCES_DIR}/Container.cpp
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
        MethaneGraphicsRhiImpl
        MethaneGraphicsTypes
        MethaneDataEvents
    PRIVATE
        MethanePlatformApp
        MethaneBuildOptions
        MethaneInstrumentation
)

if(METHANE_PRECOMPILED_HEADERS_ENABLED)
    target_precompile_headers(${TARGET} REUSE_FROM MethaneGraphicsRhiImpl)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES  ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/UserInterface
        PUBLIC_HEADER "${HEADERS}"
)

install(TARGETS ${TARGET}
    PUBLIC_HEADER
        DESTINATION ${INSTALL_INCLUDE_DIR}
        COMPONENT Development
    ARCHIVE
        DESTINATION lib
        COMPONENT Development
)

if(METHANE_TESTS_BUILD_ENABLED)

    set(TEST_TARGET MethaneUserInterfaceNullTypes)

    add_library(${TEST_TARGET} STATIC
        ${HEADERS}
        ${SOURCES}
    )

    target_include_directories(${TEST_TARGET}
        PRIVATE
            Sources
        PUBLIC
            Include
    )

    target_link_libraries(${TEST_TARGET}
        PUBLIC
            MethaneGraphicsRhiNullImpl
            MethaneGraphicsTypes
            MethaneDataEvents
        PRIVATE
            MethanePlatformApp

            MethaneBuildOptions
            MethaneInstrumentation
    )

    if(METHANE_PRECOMPILED_HEADERS_ENABLED)
        target_precompile_headers(${TEST_TARGET} REUSE_FROM MethaneGraphicsRhiNullImpl)
    endif()

    set_target_properties(${TEST_TARGET}
        PROPERTIES
        FOLDER Tests
    )

endif() # METHANE_TESTS_BUILD_ENABLED
