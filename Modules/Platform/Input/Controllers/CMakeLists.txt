set(TARGET MethanePlatformInputControllers)

include(MethaneModules)

get_module_dirs("Methane/Platform/Input")

list(APPEND HEADERS
    ${INCLUDE_DIR}/IHelpProvider.h
    ${INCLUDE_DIR}/IActionController.h
    ${INCLUDE_DIR}/IController.h
    ${INCLUDE_DIR}/Controller.h
    ${INCLUDE_DIR}/ControllersPool.h
    ${INCLUDE_DIR}/State.h
)

list(APPEND SOURCES
    ${SOURCES_DIR}/ControllersPool.cpp
    ${SOURCES_DIR}/State.cpp
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

target_link_libraries(${TARGET}
    PUBLIC
        MethanePlatformInputMouse
        MethanePlatformInputKeyboard
    PRIVATE
        MethaneBuildOptions
        MethanePrecompiledHeaders
        MethaneInstrumentation
)

target_precompile_headers(${TARGET} REUSE_FROM MethanePrecompiledHeaders)

target_include_directories(${TARGET}
    PRIVATE
        Sources
    PUBLIC
        Include
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Platform/Input
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
