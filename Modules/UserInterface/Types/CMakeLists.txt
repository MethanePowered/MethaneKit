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

target_precompile_headers(${TARGET} REUSE_FROM MethanePrecompiledExtraHeaders)

target_link_libraries(${TARGET}
    PUBLIC
        MethaneGraphicsTypes
        MethaneDataEvents
    PRIVATE
        MethaneGraphicsCore
        MethaneBuildOptions
        MethanePrecompiledExtraHeaders
        MethaneInstrumentation
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES  ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/UserInterface
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

