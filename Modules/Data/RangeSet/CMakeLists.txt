set(TARGET MethaneDataRangeSet)

include(MethaneModules)

get_module_dirs("Methane/Data")

set(SOURCES
    ${INCLUDE_DIR}/Range.hpp
    ${INCLUDE_DIR}/RangeSet.hpp
    ${SOURCES_DIR}/RangeSet.cpp
)

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
        MethaneInstrumentation
)

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Modules/Data
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

add_subdirectory(Test)