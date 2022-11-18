set(TARGET MethaneGraphicsRhiDirectX)

include(MethaneModules)

get_module_dirs("Methane/Graphics/DirectX")

list(APPEND HEADERS
    ${INCLUDE_DIR}/Types.h
    ${INCLUDE_DIR}/Device.h
    ${INCLUDE_DIR}/Fence.h
    ${INCLUDE_DIR}/IContext.h
    ${INCLUDE_DIR}/Context.hpp
    ${INCLUDE_DIR}/Shader.h
    ${INCLUDE_DIR}/Program.h
    ${INCLUDE_DIR}/ProgramArgumentBinding.h
    ${INCLUDE_DIR}/ProgramBindings.h
    ${INCLUDE_DIR}/RenderContext.h
    ${INCLUDE_DIR}/RenderState.h
    ${INCLUDE_DIR}/IResource.h
    ${INCLUDE_DIR}/ResourceView.h
    ${INCLUDE_DIR}/Resource.hpp
    ${INCLUDE_DIR}/ResourceBarriers.h
    ${INCLUDE_DIR}/DescriptorManager.h
    ${INCLUDE_DIR}/DescriptorHeap.h
    ${INCLUDE_DIR}/Buffer.h
    ${INCLUDE_DIR}/Texture.h
    ${INCLUDE_DIR}/Sampler.h
    ${INCLUDE_DIR}/QueryPool.h
    ${INCLUDE_DIR}/RenderPass.h
    ${INCLUDE_DIR}/CommandQueue.h
    ${INCLUDE_DIR}/CommandListSet.h
    ${INCLUDE_DIR}/CommandList.hpp
    ${INCLUDE_DIR}/TransferCommandList.h
    ${INCLUDE_DIR}/RenderCommandList.h
    ${INCLUDE_DIR}/ParallelRenderCommandList.h
)

list(APPEND SOURCES
    ${SOURCES_DIR}/Types.cpp
    ${SOURCES_DIR}/Device.cpp
    ${SOURCES_DIR}/Fence.cpp
    ${SOURCES_DIR}/Shader.cpp
    ${SOURCES_DIR}/Program.cpp
    ${SOURCES_DIR}/ProgramArgumentBinding.cpp
    ${SOURCES_DIR}/ProgramBindings.cpp
    ${SOURCES_DIR}/RenderContext.cpp
    ${SOURCES_DIR}/RenderState.cpp
    ${SOURCES_DIR}/IResource.cpp
    ${SOURCES_DIR}/ResourceView.cpp
    ${SOURCES_DIR}/ResourceBarriers.cpp
    ${SOURCES_DIR}/DescriptorManager.cpp
    ${SOURCES_DIR}/DescriptorHeap.cpp
    ${SOURCES_DIR}/Buffer.cpp
    ${SOURCES_DIR}/Texture.cpp
    ${SOURCES_DIR}/Sampler.cpp
    ${SOURCES_DIR}/QueryPool.cpp
    ${SOURCES_DIR}/RenderPass.cpp
    ${SOURCES_DIR}/CommandQueue.cpp
    ${SOURCES_DIR}/CommandListSet.cpp
    ${SOURCES_DIR}/TransferCommandList.cpp
    ${SOURCES_DIR}/RenderCommandList.cpp
    ${SOURCES_DIR}/ParallelRenderCommandList.cpp
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

target_link_libraries(${TARGET}
    PUBLIC
        MethaneGraphicsRhiBase
    PRIVATE
        MethaneBuildOptions
        MethaneDataPrimitives
        MethaneGraphicsPrimitives
        MethanePlatformUtils
        MethaneInstrumentation
        MethanePrecompiledExtraHeaders
        TaskFlow
        nowide
        magic_enum
        # DirectX Libs
        DirectX-Headers
        DirectXTex
        DirectXCompiler
        d3d12
        dxgi
        dxguid
)

# Add cyclic dependency of the RhiBase library from RhiDirectX due to symbolic dependency
# from RhiInterface fabric methods ISomething::Create(...) implementations defined in RhiDirectX
target_link_libraries(MethaneGraphicsRhiBase PRIVATE ${TARGET})

target_precompile_headers(${TARGET} REUSE_FROM MethanePrecompiledExtraHeaders)

target_include_directories(${TARGET}
    PRIVATE
        Sources
    PUBLIC
        Include
)

target_compile_definitions(${TARGET}
    PUBLIC
        $<$<BOOL:${METHANE_COMMAND_DEBUG_GROUPS_ENABLED}>:METHANE_COMMAND_DEBUG_GROUPS_ENABLED>
    PRIVATE
        $<$<BOOL:${METHANE_GPU_INSTRUMENTATION_ENABLED}>:METHANE_GPU_INSTRUMENTATION_ENABLED=1> # 1 - Methane, 2 - Tracy
        METHANE_GFX_DIRECTX
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Graphics/RHI
        UNITY_BUILD ${METHANE_UNITY_BUILD_ENABLED}
        UNITY_BUILD_BATCH_SIZE 4
)

if(METHANE_GFX_API EQUAL METHANE_GFX_DIRECTX)
    set_target_properties(${TARGET} PROPERTIES PREREQUISITE_MODULES "DirectXCompiler")
endif()