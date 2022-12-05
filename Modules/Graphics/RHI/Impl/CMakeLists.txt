set(TARGET MethaneGraphicsRhi)

include(MethaneModules)

get_module_dirs("Methane/Graphics/RHI")

list(APPEND HEADERS
    ${INCLUDE_DIR}/Device.h
)

list(APPEND SOURCES
    ${SOURCES_DIR}/ImplWrapper.hpp
    ${SOURCES_DIR}/Device.cpp
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

target_link_libraries(${TARGET}
    PUBLIC
        MethaneGraphicsRhiInterface
    PRIVATE
        $<$<EQUAL:${METHANE_GFX_API},${METHANE_GFX_DIRECTX}>:MethaneGraphicsRhiDirectX>
        $<$<EQUAL:${METHANE_GFX_API},${METHANE_GFX_VULKAN}>:MethaneGraphicsRhiVulkan>
        $<$<EQUAL:${METHANE_GFX_API},${METHANE_GFX_METAL}>:MethaneGraphicsRhiMetal>
)

target_precompile_headers(${TARGET} REUSE_FROM MethanePrecompiledHeaders)

target_include_directories(${TARGET}
    PUBLIC
        Include
)

target_compile_definitions(${TARGET}
    PRIVATE
        $<$<EQUAL:${METHANE_GFX_API},${METHANE_GFX_METAL}>:METHANE_GFX_METAL>
        $<$<EQUAL:${METHANE_GFX_API},${METHANE_GFX_DIRECTX}>:METHANE_GFX_DIRECTX>
        $<$<EQUAL:${METHANE_GFX_API},${METHANE_GFX_VULKAN}>:METHANE_GFX_VULKAN>
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Graphics/RHI
        PUBLIC_HEADER "${HEADERS}"
        UNITY_BUILD ${METHANE_UNITY_BUILD_ENABLED}
        UNITY_BUILD_BATCH_SIZE 4
)

if (APPLE)
    set_source_files_properties(${SOURCES}
        PROPERTIES
            COMPILE_FLAGS "-x objective-c++ -fobjc-weak"
            SKIP_PRECOMPILE_HEADERS ON
            SKIP_UNITY_BUILD_INCLUSION ON
    )
endif()

install(TARGETS ${TARGET}
    PUBLIC_HEADER
        DESTINATION ${INCLUDE_DIR}
        COMPONENT Development
    ARCHIVE
        DESTINATION Lib
        COMPONENT Development
)
