set(TARGET MethaneTestsCatchHelpers)

add_library(${TARGET} INTERFACE)

target_sources(${TARGET} INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/Methane/HlslCatchHelpers.hpp
)

target_include_directories(${TARGET} INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
)

target_link_libraries(${TARGET} INTERFACE
    MethaneBuildOptions
    MethaneDataTypes
    Catch2
    HLSLpp
)