set(TARGET MethaneGraphicsRhi)

include(MethaneModules)

get_module_dirs("Methane/Graphics/RHI")

list(APPEND HEADERS
    ${INCLUDE_DIR}/Implementations.h
    ${INCLUDE_DIR}/Pimpl.h
    ${INCLUDE_DIR}/Device.h
    ${INCLUDE_DIR}/CommandListSet.h
    ${INCLUDE_DIR}/CommandQueue.h
    ${INCLUDE_DIR}/CommandListDebugGroup.h
    ${INCLUDE_DIR}/CommandKit.h
    ${INCLUDE_DIR}/Fence.h
    ${INCLUDE_DIR}/Shader.h
    ${INCLUDE_DIR}/Program.h
    ${INCLUDE_DIR}/RenderPass.h
    ${INCLUDE_DIR}/RenderContext.h
    ${INCLUDE_DIR}/RenderState.h
    ${INCLUDE_DIR}/ViewState.h
    ${INCLUDE_DIR}/Buffer.h
    ${INCLUDE_DIR}/Texture.h
    ${INCLUDE_DIR}/Sampler.h
    ${INCLUDE_DIR}/ResourceBarriers.h
    ${INCLUDE_DIR}/RenderCommandList.h
    ${INCLUDE_DIR}/ParallelRenderCommandList.h
    ${INCLUDE_DIR}/TransferCommandList.h
)

list(APPEND SOURCES
    ${SOURCES_DIR}/ImplWrapper.hpp
    ${SOURCES_DIR}/Device.cpp
    ${SOURCES_DIR}/CommandListSet.cpp
    ${SOURCES_DIR}/CommandQueue.cpp
    ${SOURCES_DIR}/CommandListDebugGroup.cpp
    ${SOURCES_DIR}/CommandKit.cpp
    ${SOURCES_DIR}/Fence.cpp
    ${SOURCES_DIR}/Shader.cpp
    ${SOURCES_DIR}/Program.cpp
    ${SOURCES_DIR}/RenderPass.cpp
    ${SOURCES_DIR}/RenderContext.cpp
    ${SOURCES_DIR}/RenderState.cpp
    ${SOURCES_DIR}/ViewState.cpp
    ${SOURCES_DIR}/Buffer.cpp
    ${SOURCES_DIR}/Texture.cpp
    ${SOURCES_DIR}/Sampler.cpp
    ${SOURCES_DIR}/ResourceBarriers.cpp
    ${SOURCES_DIR}/RenderCommandList.cpp
    ${SOURCES_DIR}/ParallelRenderCommandList.cpp
    ${SOURCES_DIR}/TransferCommandList.cpp
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
