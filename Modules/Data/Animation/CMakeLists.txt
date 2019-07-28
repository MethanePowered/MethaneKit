set(TARGET MethaneDataAnimation)

include(Methane)

get_module_dirs("Methane/Data")

set(HEADERS
    ${INCLUDE_DIR}/Timer.h
    ${INCLUDE_DIR}/Animation.h
    ${INCLUDE_DIR}/AnimationsPool.h
    ${INCLUDE_DIR}/ValueAnimation.hpp
)

set(SOURCES
    ${SOURCES_DIR}/AnimationsPool.cpp
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

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Modules/Data
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Modules/Data
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