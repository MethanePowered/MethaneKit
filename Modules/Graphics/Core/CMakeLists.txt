set(TARGET MethaneGraphicsCore)

include(MethaneModules)

get_module_dirs("Methane/Graphics")

set(HEADERS
    ${INCLUDE_DIR}/IObject.h
    ${INCLUDE_DIR}/IDevice.h
    ${INCLUDE_DIR}/IContext.h
    ${INCLUDE_DIR}/IRenderContext.h
    ${INCLUDE_DIR}/IFence.h
    ${INCLUDE_DIR}/IShader.h
    ${INCLUDE_DIR}/IProgram.h
    ${INCLUDE_DIR}/IProgramBindings.h
    ${INCLUDE_DIR}/IRenderState.h
    ${INCLUDE_DIR}/Resource.h
    ${INCLUDE_DIR}/ResourceBarriers.h
    ${INCLUDE_DIR}/ResourceView.h
    ${INCLUDE_DIR}/Buffer.h
    ${INCLUDE_DIR}/Texture.h
    ${INCLUDE_DIR}/Sampler.h
    ${INCLUDE_DIR}/RenderPass.h
    ${INCLUDE_DIR}/CommandKit.h
    ${INCLUDE_DIR}/CommandQueue.h
    ${INCLUDE_DIR}/CommandList.h
    ${INCLUDE_DIR}/TransferCommandList.h
    ${INCLUDE_DIR}/RenderCommandList.h
    ${INCLUDE_DIR}/ParallelRenderCommandList.h
)

if (METHANE_GFX_API EQUAL METHANE_GFX_DIRECTX)

    set(GRAPHICS_API_SOURCES
        ${SOURCES_GRAPHICS_DIR}/TypesDX.h
        ${SOURCES_GRAPHICS_DIR}/TypesDX.cpp
        ${SOURCES_GRAPHICS_DIR}/DeviceDX.h
        ${SOURCES_GRAPHICS_DIR}/DeviceDX.cpp
        ${SOURCES_GRAPHICS_DIR}/FenceDX.h
        ${SOURCES_GRAPHICS_DIR}/FenceDX.cpp
        ${SOURCES_GRAPHICS_DIR}/ContextDX.h
        ${SOURCES_GRAPHICS_DIR}/ContextDX.hpp
        ${SOURCES_GRAPHICS_DIR}/ShaderDX.h
        ${SOURCES_GRAPHICS_DIR}/ShaderDX.cpp
        ${SOURCES_GRAPHICS_DIR}/ProgramDX.h
        ${SOURCES_GRAPHICS_DIR}/ProgramDX.cpp
        ${SOURCES_GRAPHICS_DIR}/ProgramArgumentBindingDX.h
        ${SOURCES_GRAPHICS_DIR}/ProgramArgumentBindingDX.cpp
        ${SOURCES_GRAPHICS_DIR}/ProgramBindingsDX.h
        ${SOURCES_GRAPHICS_DIR}/ProgramBindingsDX.cpp
        ${SOURCES_GRAPHICS_DIR}/RenderContextDX.h
        ${SOURCES_GRAPHICS_DIR}/RenderContextDX.cpp
        ${SOURCES_GRAPHICS_DIR}/RenderStateDX.h
        ${SOURCES_GRAPHICS_DIR}/RenderStateDX.cpp
        ${SOURCES_GRAPHICS_DIR}/ResourceDX.h
        ${SOURCES_GRAPHICS_DIR}/ResourceDX.cpp
        ${SOURCES_GRAPHICS_DIR}/ResourceDX.hpp
        ${SOURCES_GRAPHICS_DIR}/ResourceBarriersDX.h
        ${SOURCES_GRAPHICS_DIR}/ResourceBarriersDX.cpp
        ${SOURCES_GRAPHICS_DIR}/DescriptorManagerDX.h
        ${SOURCES_GRAPHICS_DIR}/DescriptorManagerDX.cpp
        ${SOURCES_GRAPHICS_DIR}/DescriptorHeapDX.h
        ${SOURCES_GRAPHICS_DIR}/DescriptorHeapDX.cpp
        ${SOURCES_GRAPHICS_DIR}/QueryPoolDX.h
        ${SOURCES_GRAPHICS_DIR}/QueryPoolDX.cpp
        ${SOURCES_GRAPHICS_DIR}/BufferDX.h
        ${SOURCES_GRAPHICS_DIR}/BufferDX.cpp
        ${SOURCES_GRAPHICS_DIR}/TextureDX.h
        ${SOURCES_GRAPHICS_DIR}/TextureDX.cpp
        ${SOURCES_GRAPHICS_DIR}/SamplerDX.h
        ${SOURCES_GRAPHICS_DIR}/SamplerDX.cpp
        ${SOURCES_GRAPHICS_DIR}/RenderPassDX.h
        ${SOURCES_GRAPHICS_DIR}/RenderPassDX.cpp
        ${SOURCES_GRAPHICS_DIR}/CommandQueueDX.h
        ${SOURCES_GRAPHICS_DIR}/CommandQueueDX.cpp
        ${SOURCES_GRAPHICS_DIR}/CommandListDX.h
        ${SOURCES_GRAPHICS_DIR}/CommandListDX.cpp
        ${SOURCES_GRAPHICS_DIR}/CommandListDX.hpp
        ${SOURCES_GRAPHICS_DIR}/TransferCommandListDX.h
        ${SOURCES_GRAPHICS_DIR}/TransferCommandListDX.cpp
        ${SOURCES_GRAPHICS_DIR}/RenderCommandListDX.h
        ${SOURCES_GRAPHICS_DIR}/RenderCommandListDX.cpp
        ${SOURCES_GRAPHICS_DIR}/ParallelRenderCommandListDX.h
        ${SOURCES_GRAPHICS_DIR}/ParallelRenderCommandListDX.cpp
    )

    set(PLATFORM_LIBRARIES
        DirectX-Headers
        DirectXTex
        DirectXCompiler
        d3d12
        dxgi
        dxguid
    )

elseif(METHANE_GFX_API EQUAL METHANE_GFX_METAL)

    set(GRAPHICS_API_SOURCES
        ${SOURCES_GRAPHICS_DIR}/TypesMT.hh
        ${SOURCES_GRAPHICS_DIR}/TypesMT.mm
        ${SOURCES_GRAPHICS_DIR}/DeviceMT.hh
        ${SOURCES_GRAPHICS_DIR}/DeviceMT.mm
        ${SOURCES_GRAPHICS_DIR}/FenceMT.hh
        ${SOURCES_GRAPHICS_DIR}/FenceMT.mm
        ${SOURCES_GRAPHICS_DIR}/ContextMT.h
        ${SOURCES_GRAPHICS_DIR}/ContextMT.hpp
        ${SOURCES_GRAPHICS_DIR}/RenderContextMT.hh
        ${SOURCES_GRAPHICS_DIR}/RenderContextMT.mm
        ${SOURCES_GRAPHICS_DIR}/RenderContextAppViewMT.hh
        ${SOURCES_GRAPHICS_DIR}/ShaderMT.hh
        ${SOURCES_GRAPHICS_DIR}/ShaderMT.mm
        ${SOURCES_GRAPHICS_DIR}/ProgramMT.hh
        ${SOURCES_GRAPHICS_DIR}/ProgramMT.mm
        ${SOURCES_GRAPHICS_DIR}/ProgramLibraryMT.hh
        ${SOURCES_GRAPHICS_DIR}/ProgramLibraryMT.mm
        ${SOURCES_GRAPHICS_DIR}/ProgramArgumentBindingMT.hh
        ${SOURCES_GRAPHICS_DIR}/ProgramArgumentBindingMT.mm
        ${SOURCES_GRAPHICS_DIR}/ProgramBindingsMT.hh
        ${SOURCES_GRAPHICS_DIR}/ProgramBindingsMT.mm
        ${SOURCES_GRAPHICS_DIR}/RenderStateMT.hh
        ${SOURCES_GRAPHICS_DIR}/RenderStateMT.mm
        ${SOURCES_GRAPHICS_DIR}/ResourceMT.hh
        ${SOURCES_GRAPHICS_DIR}/ResourceMT.mm
        ${SOURCES_GRAPHICS_DIR}/DescriptorManagerMT.h
        ${SOURCES_GRAPHICS_DIR}/QueryPoolMT.mm
        ${SOURCES_GRAPHICS_DIR}/BufferMT.hh
        ${SOURCES_GRAPHICS_DIR}/BufferMT.mm
        ${SOURCES_GRAPHICS_DIR}/TextureMT.hh
        ${SOURCES_GRAPHICS_DIR}/TextureMT.mm
        ${SOURCES_GRAPHICS_DIR}/SamplerMT.hh
        ${SOURCES_GRAPHICS_DIR}/SamplerMT.mm
        ${SOURCES_GRAPHICS_DIR}/RenderPassMT.hh
        ${SOURCES_GRAPHICS_DIR}/RenderPassMT.mm
        ${SOURCES_GRAPHICS_DIR}/CommandQueueMT.hh
        ${SOURCES_GRAPHICS_DIR}/CommandQueueMT.mm
        ${SOURCES_GRAPHICS_DIR}/CommandListMT.hh
        ${SOURCES_GRAPHICS_DIR}/CommandListMT.mm
        ${SOURCES_GRAPHICS_DIR}/CommandListMT.hpp
        ${SOURCES_GRAPHICS_DIR}/TransferCommandListMT.hh
        ${SOURCES_GRAPHICS_DIR}/TransferCommandListMT.mm
        ${SOURCES_GRAPHICS_DIR}/RenderCommandListMT.hh
        ${SOURCES_GRAPHICS_DIR}/RenderCommandListMT.mm
        ${SOURCES_GRAPHICS_DIR}/ParallelRenderCommandListMT.hh
        ${SOURCES_GRAPHICS_DIR}/ParallelRenderCommandListMT.mm
    )

    set(PLATFORM_SOURCES_OBJC ${GRAPHICS_API_SOURCES})

    set(PLATFORM_LIBRARIES
        "-framework Metal"
    )

elseif(METHANE_GFX_API EQUAL METHANE_GFX_VULKAN)

    set(GRAPHICS_API_SOURCES
        ${SOURCES_GRAPHICS_DIR}/PlatformVK.h
        ${SOURCES_GRAPHICS_DIR}/PlatformVK.cpp
        ${SOURCES_GRAPHICS_DIR}/${PLATFORM_DIR}/PlatformExtVK.${CPP_EXT}
        ${SOURCES_GRAPHICS_DIR}/UtilsVK.hpp
        ${SOURCES_GRAPHICS_DIR}/TypesVK.h
        ${SOURCES_GRAPHICS_DIR}/TypesVK.cpp
        ${SOURCES_GRAPHICS_DIR}/DeviceVK.h
        ${SOURCES_GRAPHICS_DIR}/DeviceVK.cpp
        ${SOURCES_GRAPHICS_DIR}/FenceVK.h
        ${SOURCES_GRAPHICS_DIR}/FenceVK.cpp
        ${SOURCES_GRAPHICS_DIR}/ContextVK.h
        ${SOURCES_GRAPHICS_DIR}/ContextVK.hpp
        ${SOURCES_GRAPHICS_DIR}/ShaderVK.h
        ${SOURCES_GRAPHICS_DIR}/ShaderVK.cpp
        ${SOURCES_GRAPHICS_DIR}/ProgramVK.h
        ${SOURCES_GRAPHICS_DIR}/ProgramVK.cpp
        ${SOURCES_GRAPHICS_DIR}/ProgramArgumentBindingVK.h
        ${SOURCES_GRAPHICS_DIR}/ProgramArgumentBindingVK.cpp
        ${SOURCES_GRAPHICS_DIR}/ProgramBindingsVK.h
        ${SOURCES_GRAPHICS_DIR}/ProgramBindingsVK.cpp
        ${SOURCES_GRAPHICS_DIR}/RenderContextVK.h
        ${SOURCES_GRAPHICS_DIR}/RenderContextVK.cpp
        ${SOURCES_GRAPHICS_DIR}/RenderStateVK.h
        ${SOURCES_GRAPHICS_DIR}/RenderStateVK.cpp
        ${SOURCES_GRAPHICS_DIR}/ResourceVK.h
        ${SOURCES_GRAPHICS_DIR}/ResourceVK.cpp
        ${SOURCES_GRAPHICS_DIR}/ResourceVK.hpp
        ${SOURCES_GRAPHICS_DIR}/ResourceBarriersVK.h
        ${SOURCES_GRAPHICS_DIR}/ResourceBarriersVK.cpp
        ${SOURCES_GRAPHICS_DIR}/DescriptorManagerVK.h
        ${SOURCES_GRAPHICS_DIR}/DescriptorManagerVK.cpp
        ${SOURCES_GRAPHICS_DIR}/QueryPoolVK.h
        ${SOURCES_GRAPHICS_DIR}/QueryPoolVK.cpp
        ${SOURCES_GRAPHICS_DIR}/BufferVK.h
        ${SOURCES_GRAPHICS_DIR}/BufferVK.cpp
        ${SOURCES_GRAPHICS_DIR}/TextureVK.h
        ${SOURCES_GRAPHICS_DIR}/TextureVK.cpp
        ${SOURCES_GRAPHICS_DIR}/SamplerVK.h
        ${SOURCES_GRAPHICS_DIR}/SamplerVK.cpp
        ${SOURCES_GRAPHICS_DIR}/RenderPassVK.h
        ${SOURCES_GRAPHICS_DIR}/RenderPassVK.cpp
        ${SOURCES_GRAPHICS_DIR}/CommandQueueVK.h
        ${SOURCES_GRAPHICS_DIR}/CommandQueueVK.cpp
        ${SOURCES_GRAPHICS_DIR}/CommandListVK.h
        ${SOURCES_GRAPHICS_DIR}/CommandListVK.cpp
        ${SOURCES_GRAPHICS_DIR}/CommandListVK.hpp
        ${SOURCES_GRAPHICS_DIR}/TransferCommandListVK.h
        ${SOURCES_GRAPHICS_DIR}/TransferCommandListVK.cpp
        ${SOURCES_GRAPHICS_DIR}/RenderCommandListVK.h
        ${SOURCES_GRAPHICS_DIR}/RenderCommandListVK.cpp
        ${SOURCES_GRAPHICS_DIR}/ParallelRenderCommandListVK.h
        ${SOURCES_GRAPHICS_DIR}/ParallelRenderCommandListVK.cpp
    )

    if (APPLE)
        set(VULKAN_TO_METAL_SOURCES
            ${SOURCES_GRAPHICS_DIR}/RenderContextVK.mm
            ${SOURCES_DIR}/Metal/TypesMT.hh
            ${SOURCES_DIR}/Metal/TypesMT.mm
            ${SOURCES_DIR}/Metal/RenderContextAppViewMT.hh
        )
        set(GRAPHICS_API_SOURCES ${GRAPHICS_API_SOURCES}
            ${VULKAN_TO_METAL_SOURCES}
        )
        set(PLATFORM_SOURCES_OBJC ${VULKAN_TO_METAL_SOURCES}
            ${SOURCES_GRAPHICS_DIR}/${PLATFORM_DIR}/PlatformExtVK.mm
        )
    endif()

    set(PLATFORM_LIBRARIES
        $<$<NOT:$<BOOL:${APPLE}>>:Vulkan-Headers> # Lin/Win: Dynamic linking with Vulkan, only Vulkan headers are needed
        $<$<BOOL:${LINUX}>:dl>                    # Linux: Link with dynamic linker for vk::DynamicLoader on Linux
        $<$<BOOL:${APPLE}>:Vulkan::Vulkan>        # MacOS: Link statically with MoltenVK framework on MacOS
        spirv-cross-core
        spirv-cross-hlsl
    )

endif()

set(SOURCES ${GRAPHICS_API_SOURCES}

    # Base implementation
    ${SOURCES_DIR}/ObjectBase.h
    ${SOURCES_DIR}/ObjectBase.cpp
    ${SOURCES_DIR}/DeviceBase.h
    ${SOURCES_DIR}/DeviceBase.cpp
    ${SOURCES_DIR}/ContextBase.h
    ${SOURCES_DIR}/ContextBase.cpp
    ${SOURCES_DIR}/RenderContextBase.h
    ${SOURCES_DIR}/RenderContextBase.cpp
    ${SOURCES_DIR}/FenceBase.h
    ${SOURCES_DIR}/FenceBase.cpp
    ${SOURCES_DIR}/ShaderBase.h
    ${SOURCES_DIR}/ShaderBase.cpp
    ${SOURCES_DIR}/ProgramBase.h
    ${SOURCES_DIR}/ProgramBase.cpp
    ${SOURCES_DIR}/ProgramArgumentBindingBase.h
    ${SOURCES_DIR}/ProgramArgumentBindingBase.cpp
    ${SOURCES_DIR}/ProgramBindingsBase.h
    ${SOURCES_DIR}/ProgramBindingsBase.cpp
    ${SOURCES_DIR}/RenderStateBase.h
    ${SOURCES_DIR}/RenderStateBase.cpp
    ${SOURCES_DIR}/ResourceView.cpp
    ${SOURCES_DIR}/ResourceBarriers.cpp
    ${SOURCES_DIR}/ResourceBase.h
    ${SOURCES_DIR}/ResourceBase.cpp
    ${SOURCES_DIR}/BufferBase.h
    ${SOURCES_DIR}/BufferBase.cpp
    ${SOURCES_DIR}/TextureBase.h
    ${SOURCES_DIR}/TextureBase.cpp
    ${SOURCES_DIR}/SamplerBase.h
    ${SOURCES_DIR}/SamplerBase.cpp
    ${SOURCES_DIR}/RenderPassBase.h
    ${SOURCES_DIR}/RenderPassBase.cpp
    ${SOURCES_DIR}/CommandKitBase.h
    ${SOURCES_DIR}/CommandKitBase.cpp
    ${SOURCES_DIR}/CommandQueueBase.h
    ${SOURCES_DIR}/CommandQueueBase.cpp
    ${SOURCES_DIR}/CommandQueueTrackingBase.h
    ${SOURCES_DIR}/CommandQueueTrackingBase.cpp
    ${SOURCES_DIR}/CommandListBase.h
    ${SOURCES_DIR}/CommandListBase.cpp
    ${SOURCES_DIR}/RenderCommandListBase.h
    ${SOURCES_DIR}/RenderCommandListBase.cpp
    ${SOURCES_DIR}/ParallelRenderCommandListBase.h
    ${SOURCES_DIR}/ParallelRenderCommandListBase.cpp
    ${SOURCES_DIR}/DescriptorManager.h
    ${SOURCES_DIR}/DescriptorManagerBase.h
    ${SOURCES_DIR}/DescriptorManagerBase.cpp
    ${SOURCES_DIR}/QueryPool.h
    ${SOURCES_DIR}/QueryPoolBase.h
    ${SOURCES_DIR}/QueryPoolBase.cpp

    # Native alias headers
    ${SOURCES_DIR}/Native/ContextNT.h

    # Other sources
    ${SOURCES_DIR}/CoreFormatters.hpp
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

target_link_libraries(${TARGET}
    PUBLIC
        MethanePrimitives
        MethaneDataProvider
        MethaneDataRangeSet
        MethaneDataEvents
        MethaneGraphicsTypes
        MethanePlatformAppView
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
        ${PLATFORM_LIBRARIES}
)

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
        $<$<EQUAL:${METHANE_GFX_API},${METHANE_GFX_METAL}>:METHANE_GFX_METAL>
        $<$<EQUAL:${METHANE_GFX_API},${METHANE_GFX_DIRECTX}>:METHANE_GFX_DIRECTX>
        $<$<EQUAL:${METHANE_GFX_API},${METHANE_GFX_VULKAN}>:METHANE_GFX_VULKAN VK_NO_PROTOTYPES>
        $<$<AND:$<EQUAL:${METHANE_GFX_API},${METHANE_GFX_VULKAN}>,$<BOOL:${WIN32}>>:VK_USE_PLATFORM_WIN32_KHR>
        $<$<AND:$<EQUAL:${METHANE_GFX_API},${METHANE_GFX_VULKAN}>,$<BOOL:${APPLE}>>:VK_USE_PLATFORM_METAL_EXT>
        $<$<AND:$<EQUAL:${METHANE_GFX_API},${METHANE_GFX_VULKAN}>,$<BOOL:${LINUX}>>:VK_USE_PLATFORM_XCB_KHR>
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Graphics
        PUBLIC_HEADER "${HEADERS}"
        UNITY_BUILD ${METHANE_UNITY_BUILD_ENABLED}
        UNITY_BUILD_BATCH_SIZE 6
)

if(METHANE_GFX_API EQUAL METHANE_GFX_DIRECTX)
    set_target_properties(${TARGET} PROPERTIES PREREQUISITE_MODULES "DirectXCompiler")
endif()

if (APPLE)
    set_source_files_properties(
        ${PLATFORM_SOURCES_OBJC}
        PROPERTIES
            COMPILE_FLAGS -fobjc-weak # CLANG_ENABLE_OBJC_WEAK = YES
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
