set(TARGET MethaneDataEvents)

include(MethaneModules)

get_module_dirs("Methane/Data")

set(HEADERS
    ${INCLUDE_DIR}/IEmitter.h
    ${INCLUDE_DIR}/Emitter.hpp
    ${INCLUDE_DIR}/Transmitter.hpp
    ${INCLUDE_DIR}/Receiver.hpp
)

set(SOURCES
    ${SOURCES_DIR}/Events.cpp
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
        MethanePrimitives
        MethaneInstrumentation
        MethaneBuildOptions
)

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Data
        PUBLIC_HEADER "${HEADERS}"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

install(TARGETS ${TARGET}
    PUBLIC_HEADER
        DESTINATION ${INCLUDE_DIR}
        COMPONENT Development
)
