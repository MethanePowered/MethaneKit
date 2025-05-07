set(TARGET MethanePlatformInputActionControllers)

include(MethaneModules)

get_module_dirs("Methane/Platform/Input")

set(PATH_TO_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${INCLUDE_DIR})

list(APPEND HEADERS
    ${PATH_TO_INCLUDE_DIR}/KeyboardActionControllerBase.hpp
    ${PATH_TO_INCLUDE_DIR}/MouseActionControllerBase.hpp
)

add_library(${TARGET} INTERFACE)

target_sources(${TARGET}
    INTERFACE
        ${HEADERS}
)

target_include_directories(${TARGET}
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Include
)

target_link_libraries(${TARGET}
    INTERFACE
        MethanePlatformInputControllers
        magic_enum
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Platform/Input
        PUBLIC_HEADER "${HEADERS}"
)

install(TARGETS ${TARGET}
    PUBLIC_HEADER
        DESTINATION ${INSTALL_INCLUDE_DIR}
        COMPONENT Development
)