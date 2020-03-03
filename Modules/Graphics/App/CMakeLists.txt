set(TARGET MethaneGraphicsApp)

get_module_dirs("Methane/Graphics")

set(HEADERS
    ${INCLUDE_DIR}/App.h
    ${INCLUDE_DIR}/App.hpp
    ${INCLUDE_DIR}/AppController.h
    ${INCLUDE_DIR}/AppCameraController.h
    ${INCLUDE_DIR}/AppContextController.h
)

set(SOURCES
    ${SOURCES_DIR}/AppController.cpp
    ${SOURCES_DIR}/AppCameraController.cpp
    ${SOURCES_DIR}/AppContextController.cpp
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
    MethaneDataPrimitives
    MethanePlatformApp
    MethanePlatformUtils
    MethaneGraphicsCore
    MethaneGraphicsHelpers
    MethaneInstrumentation
)

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Modules/Graphics
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
    FOLDER Modules/Graphics
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
