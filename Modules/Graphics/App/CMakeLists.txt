set(TARGET MethaneGraphicsApp)

get_module_dirs("Methane/Graphics")

add_library(${TARGET} INTERFACE)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/${INCLUDE_DIR}/App.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/${INCLUDE_DIR}/AppDataProvider.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/${INCLUDE_DIR}/AppCameraController.hpp
)

target_sources(${TARGET} INTERFACE
    ${HEADERS}
)

target_include_directories(${TARGET} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
    $<INSTALL_INTERFACE:Include>
)

target_link_libraries(${TARGET} INTERFACE
    MethaneDataTypes
    MethanePlatformApp
    MethanePlatformUtils
    MethaneGraphicsCore
    MethaneGraphicsHelpers
)

install(FILES ${HEADERS} DESTINATION ${INCLUDE_DIR})