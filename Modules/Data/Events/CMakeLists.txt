set(TARGET MethaneDataEvents)

include(MethaneModules)

get_module_dirs("Methane/Data")

set(SOURCES
    ${INCLUDE_DIR}/IEmitter.h
    ${INCLUDE_DIR}/Emitter.hpp
    ${INCLUDE_DIR}/Receiver.hpp
    ${SOURCES_DIR}/Events.cpp
    Include/Methane/Data/IEmitter.h)

add_library(${TARGET} STATIC
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
        FOLDER ${NAMESPACE_DIR}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

add_subdirectory(Test)