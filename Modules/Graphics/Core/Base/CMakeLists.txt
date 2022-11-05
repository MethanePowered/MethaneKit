set(TARGET MethaneGraphicsCoreBase)

include(MethaneModules)

get_module_dirs("Methane/Graphics")

set(HEADERS
    ${INCLUDE_DIR}/ObjectBase.h
    ${INCLUDE_DIR}/DeviceBase.h
    ${INCLUDE_DIR}/ContextBase.h
    ${INCLUDE_DIR}/RenderContextBase.h
    ${INCLUDE_DIR}/FenceBase.h
    ${INCLUDE_DIR}/ShaderBase.h
    ${INCLUDE_DIR}/ProgramBase.h
    ${INCLUDE_DIR}/ProgramArgumentBindingBase.h
    ${INCLUDE_DIR}/ProgramBindingsBase.h
    ${INCLUDE_DIR}/RenderPassBase.h
    ${INCLUDE_DIR}/RenderStateBase.h
    ${INCLUDE_DIR}/ResourceBarriersBase.h
    ${INCLUDE_DIR}/ResourceBase.h
    ${INCLUDE_DIR}/BufferBase.h
    ${INCLUDE_DIR}/TextureBase.h
    ${INCLUDE_DIR}/SamplerBase.h
    ${INCLUDE_DIR}/CommandKitBase.h
    ${INCLUDE_DIR}/CommandQueueBase.h
    ${INCLUDE_DIR}/CommandQueueTrackingBase.h
    ${INCLUDE_DIR}/CommandListBase.h
    ${INCLUDE_DIR}/RenderCommandListBase.h
    ${INCLUDE_DIR}/ParallelRenderCommandListBase.h
    ${INCLUDE_DIR}/DescriptorManager.h
    ${INCLUDE_DIR}/DescriptorManagerBase.h
    ${INCLUDE_DIR}/QueryPoolBase.h
)

if(METHANE_GFX_API EQUAL METHANE_GFX_VULKAN)

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
    ${SOURCES_DIR}/ObjectBase.cpp
    ${SOURCES_DIR}/DeviceBase.cpp
    ${SOURCES_DIR}/ContextBase.cpp
    ${SOURCES_DIR}/RenderContextBase.cpp
    ${SOURCES_DIR}/FenceBase.cpp
    ${SOURCES_DIR}/ShaderBase.cpp
    ${SOURCES_DIR}/ProgramBase.cpp
    ${SOURCES_DIR}/ProgramArgumentBindingBase.cpp
    ${SOURCES_DIR}/ProgramBindingsBase.cpp
    ${SOURCES_DIR}/RenderStateBase.cpp
    ${SOURCES_DIR}/ResourceBarriersBase.cpp
    ${SOURCES_DIR}/ResourceBase.cpp
    ${SOURCES_DIR}/BufferBase.cpp
    ${SOURCES_DIR}/TextureBase.cpp
    ${SOURCES_DIR}/SamplerBase.cpp
    ${SOURCES_DIR}/RenderPassBase.cpp
    ${SOURCES_DIR}/CommandKitBase.cpp
    ${SOURCES_DIR}/CommandQueueBase.cpp
    ${SOURCES_DIR}/CommandQueueTrackingBase.cpp
    ${SOURCES_DIR}/CommandListBase.cpp
    ${SOURCES_DIR}/RenderCommandListBase.cpp
    ${SOURCES_DIR}/ParallelRenderCommandListBase.cpp
    ${SOURCES_DIR}/DescriptorManagerBase.cpp
    ${SOURCES_DIR}/QueryPoolBase.cpp
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

target_link_libraries(${TARGET}
    PUBLIC
        MethaneGraphicsCoreInterface
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
    PUBLIC
        Include
    PRIVATE
        Sources
)

target_compile_definitions(${TARGET}
    PUBLIC
        $<$<BOOL:${METHANE_COMMAND_DEBUG_GROUPS_ENABLED}>:METHANE_COMMAND_DEBUG_GROUPS_ENABLED>
    PRIVATE
        $<$<BOOL:${METHANE_GPU_INSTRUMENTATION_ENABLED}>:METHANE_GPU_INSTRUMENTATION_ENABLED=1> # 1 - Methane, 2 - Tracy
        $<$<EQUAL:${METHANE_GFX_API},${METHANE_GFX_VULKAN}>:METHANE_GFX_VULKAN VK_NO_PROTOTYPES>
        $<$<AND:$<EQUAL:${METHANE_GFX_API},${METHANE_GFX_VULKAN}>,$<BOOL:${WIN32}>>:VK_USE_PLATFORM_WIN32_KHR>
        $<$<AND:$<EQUAL:${METHANE_GFX_API},${METHANE_GFX_VULKAN}>,$<BOOL:${APPLE}>>:VK_USE_PLATFORM_METAL_EXT>
        $<$<AND:$<EQUAL:${METHANE_GFX_API},${METHANE_GFX_VULKAN}>,$<BOOL:${LINUX}>>:VK_USE_PLATFORM_XCB_KHR>
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Graphics/Core
        UNITY_BUILD ${METHANE_UNITY_BUILD_ENABLED}
        UNITY_BUILD_BATCH_SIZE 6
)

if(METHANE_GFX_API EQUAL METHANE_GFX_DIRECTX)
    set_target_properties(${TARGET} PROPERTIES PREREQUISITE_MODULES "DirectXCompiler")
endif()
