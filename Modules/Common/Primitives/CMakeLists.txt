set(TARGET MethanePrimitives)

include(MethaneModules)

get_module_dirs("Methane")

set(HEADERS
    ${INCLUDE_DIR}/Version.h
    ${INCLUDE_DIR}/Memory.hpp
    ${INCLUDE_DIR}/Exceptions.hpp
    ${INCLUDE_DIR}/Checks.hpp
    ${INCLUDE_DIR}/Timer.hpp
    ${INCLUDE_DIR}/Pimpl.h
    ${INCLUDE_DIR}/Pimpl.hpp
)

set(SOURCES
    ${SOURCES_DIR}/Primitives.cpp
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
        MethaneBuildOptions
        fmt
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Common
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
