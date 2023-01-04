set(TARGET MethaneGraphicsRhiBase)

include(MethaneModules)

get_module_dirs("Methane/Graphics/Base")

set(HEADERS
    ${INCLUDE_DIR}/Object.h
    ${INCLUDE_DIR}/Device.h
    ${INCLUDE_DIR}/Context.h
    ${INCLUDE_DIR}/RenderContext.h
    ${INCLUDE_DIR}/Fence.h
    ${INCLUDE_DIR}/Shader.h
    ${INCLUDE_DIR}/Program.h
    ${INCLUDE_DIR}/ProgramArgumentBinding.h
    ${INCLUDE_DIR}/ProgramBindings.h
    ${INCLUDE_DIR}/RenderPass.h
    ${INCLUDE_DIR}/RenderState.h
    ${INCLUDE_DIR}/ViewState.h
    ${INCLUDE_DIR}/ResourceBarriers.h
    ${INCLUDE_DIR}/Resource.h
    ${INCLUDE_DIR}/Buffer.h
    ${INCLUDE_DIR}/Texture.h
    ${INCLUDE_DIR}/Sampler.h
    ${INCLUDE_DIR}/CommandKit.h
    ${INCLUDE_DIR}/CommandQueue.h
    ${INCLUDE_DIR}/CommandQueueTracking.h
    ${INCLUDE_DIR}/CommandList.h
    ${INCLUDE_DIR}/CommandListSet.h
    ${INCLUDE_DIR}/CommandListDebugGroup.h
    ${INCLUDE_DIR}/RenderCommandList.h
    ${INCLUDE_DIR}/ParallelRenderCommandList.h
    ${INCLUDE_DIR}/DescriptorManager.h
    ${INCLUDE_DIR}/QueryPool.h
    ${INCLUDE_DIR}/FpsCounter.h
)

set(SOURCES ${GRAPHICS_API_SOURCES}
    ${SOURCES_DIR}/Object.cpp
    ${SOURCES_DIR}/Device.cpp
    ${SOURCES_DIR}/Context.cpp
    ${SOURCES_DIR}/RenderContext.cpp
    ${SOURCES_DIR}/Fence.cpp
    ${SOURCES_DIR}/Shader.cpp
    ${SOURCES_DIR}/Program.cpp
    ${SOURCES_DIR}/ProgramArgumentBinding.cpp
    ${SOURCES_DIR}/ProgramBindings.cpp
    ${SOURCES_DIR}/RenderState.cpp
    ${SOURCES_DIR}/ViewState.cpp
    ${SOURCES_DIR}/ResourceBarriers.cpp
    ${SOURCES_DIR}/Resource.cpp
    ${SOURCES_DIR}/Buffer.cpp
    ${SOURCES_DIR}/Texture.cpp
    ${SOURCES_DIR}/Sampler.cpp
    ${SOURCES_DIR}/RenderPass.cpp
    ${SOURCES_DIR}/CommandKit.cpp
    ${SOURCES_DIR}/CommandQueue.cpp
    ${SOURCES_DIR}/CommandQueueTracking.cpp
    ${SOURCES_DIR}/CommandList.cpp
    ${SOURCES_DIR}/CommandListSet.cpp
    ${SOURCES_DIR}/CommandListDebugGroup.cpp
    ${SOURCES_DIR}/RenderCommandList.cpp
    ${SOURCES_DIR}/ParallelRenderCommandList.cpp
    ${SOURCES_DIR}/DescriptorManager.cpp
    ${SOURCES_DIR}/QueryPool.cpp
    ${SOURCES_DIR}/FpsCounter.cpp
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

if(METHANE_PRECOMPILED_HEADERS_ENABLED)
    target_precompile_headers(${TARGET} REUSE_FROM MethaneMathPrecompiledHeaders)
endif()

target_link_libraries(${TARGET}
    PUBLIC
        MethaneGraphicsRhiInterface
        magic_enum
    PRIVATE
        MethaneBuildOptions
        MethaneCommonPrecompiledHeaders
        MethaneDataPrimitives
        MethanePlatformUtils
        MethaneInstrumentation
        MethaneMathPrecompiledHeaders
        TaskFlow
        nowide
)

target_compile_definitions(${TARGET}
    PUBLIC
        $<$<BOOL:${METHANE_GPU_INSTRUMENTATION_ENABLED}>:METHANE_GPU_INSTRUMENTATION_ENABLED=1> # 1 - Methane, 2 - Tracy
)

target_include_directories(${TARGET}
    PUBLIC
        Include
    PRIVATE
        Sources
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Graphics/RHI
        UNITY_BUILD ${METHANE_UNITY_BUILD_ENABLED}
        UNITY_BUILD_BATCH_SIZE 6
)
