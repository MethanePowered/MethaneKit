set(TARGET MethaneDataAnimation)

include(Methane)

get_module_dirs("Methane/Data")

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/${INCLUDE_DIR}/ActionTimer.hpp
)

add_library(${TARGET} INTERFACE)

target_sources(${TARGET} INTERFACE
    ${HEADERS}
)

target_include_directories(${TARGET} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
    $<INSTALL_INTERFACE:Include>
)

target_link_libraries(${TARGET} INTERFACE
    MethaneDataTypes
)

install(FILES ${HEADERS} DESTINATION ${INCLUDE_DIR})