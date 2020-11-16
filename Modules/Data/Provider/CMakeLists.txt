set(TARGET MethaneDataProvider)

include(MethaneModules)

get_module_dirs("Methane/Data")

set(HEADERS
    ${INCLUDE_DIR}/Provider.h
    ${INCLUDE_DIR}/FileProvider.hpp
    ${INCLUDE_DIR}/ResourceProvider.hpp
    ${INCLUDE_DIR}/AppResourceProviders.h
    ${INCLUDE_DIR}/AppShadersProvider.h
    ${INCLUDE_DIR}/AppTexturesProvider.h
    ${INCLUDE_DIR}/AppFontsProvider.h
)

set(SOURCES
    ${SOURCES_DIR}/Provider.cpp
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
        MethaneDataTypes
        MethaneInstrumentation
        MethanePlatformUtils
    PRIVATE
        MethaneBuildOptions
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES  ${HEADERS} ${SOURCES})

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
