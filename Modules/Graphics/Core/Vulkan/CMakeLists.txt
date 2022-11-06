set(TARGET MethaneGraphicsCoreVulkan)

include(MethaneModules)

get_module_dirs("Methane/Graphics")

list(APPEND HEADERS
    ${INCLUDE_DIR}/PlatformVK.h
    ${INCLUDE_DIR}/TypesVK.h
    ${INCLUDE_DIR}/DeviceVK.h
    ${INCLUDE_DIR}/FenceVK.h
    ${INCLUDE_DIR}/ContextVK.h
    ${INCLUDE_DIR}/ContextVK.hpp
    ${INCLUDE_DIR}/ShaderVK.h
    ${INCLUDE_DIR}/ProgramVK.h
    ${INCLUDE_DIR}/ProgramArgumentBindingVK.h
    ${INCLUDE_DIR}/ProgramBindingsVK.h
    ${INCLUDE_DIR}/RenderContextVK.h
    ${INCLUDE_DIR}/RenderStateVK.h
    ${INCLUDE_DIR}/ResourceVK.h
    ${INCLUDE_DIR}/ResourceBarriersVK.h
    ${INCLUDE_DIR}/DescriptorManagerVK.h
    ${INCLUDE_DIR}/QueryPoolVK.h
    ${INCLUDE_DIR}/ResourceVK.hpp
    ${INCLUDE_DIR}/BufferVK.h
    ${INCLUDE_DIR}/TextureVK.h
    ${INCLUDE_DIR}/SamplerVK.h
    ${INCLUDE_DIR}/RenderPassVK.h
    ${INCLUDE_DIR}/CommandQueueVK.h
    ${INCLUDE_DIR}/CommandListVK.h
    ${INCLUDE_DIR}/CommandListVK.hpp
    ${INCLUDE_DIR}/TransferCommandListVK.h
    ${INCLUDE_DIR}/RenderCommandListVK.h
    ${INCLUDE_DIR}/ParallelRenderCommandListVK.h
    ${INCLUDE_DIR}/UtilsVK.hpp
)

list(APPEND SOURCES
    ${SOURCES_DIR}/PlatformVK.cpp
    ${SOURCES_DIR}/${PLATFORM_DIR}/PlatformExtVK.${CPP_EXT}
    ${SOURCES_DIR}/TypesVK.cpp
    ${SOURCES_DIR}/DeviceVK.cpp
    ${SOURCES_DIR}/FenceVK.cpp
    ${SOURCES_DIR}/ShaderVK.cpp
    ${SOURCES_DIR}/ProgramVK.cpp
    ${SOURCES_DIR}/ProgramArgumentBindingVK.cpp
    ${SOURCES_DIR}/ProgramBindingsVK.cpp
    ${SOURCES_DIR}/RenderContextVK.cpp
    ${SOURCES_DIR}/RenderStateVK.cpp
    ${SOURCES_DIR}/ResourceVK.cpp
    ${SOURCES_DIR}/ResourceBarriersVK.cpp
    ${SOURCES_DIR}/DescriptorManagerVK.cpp
    ${SOURCES_DIR}/QueryPoolVK.cpp
    ${SOURCES_DIR}/BufferVK.cpp
    ${SOURCES_DIR}/TextureVK.cpp
    ${SOURCES_DIR}/SamplerVK.cpp
    ${SOURCES_DIR}/RenderPassVK.cpp
    ${SOURCES_DIR}/CommandQueueVK.cpp
    ${SOURCES_DIR}/CommandListVK.cpp
    ${SOURCES_DIR}/TransferCommandListVK.cpp
    ${SOURCES_DIR}/RenderCommandListVK.cpp
    ${SOURCES_DIR}/ParallelRenderCommandListVK.cpp
)

if (APPLE)
    set(METAL_SOURCE_DIR ../Metal)
    list(APPEND METAL_SOURCES
        ${METAL_SOURCE_DIR}/${INCLUDE_DIR}/RenderContextAppViewMT.hh
        ${METAL_SOURCE_DIR}/${INCLUDE_DIR}/TypesMT.hh
        ${METAL_SOURCE_DIR}/${SOURCES_DIR}/TypesMT.mm
    )
    list(APPEND SOURCES
        ${SOURCES_DIR}/RenderContextVK.mm
    )
    set(PLATFORM_SOURCES_OBJC
        ${METAL_SOURCES}
        ${SOURCES_DIR}/RenderContextVK.mm
        ${SOURCES_GRAPHICS_DIR}/${PLATFORM_DIR}/PlatformExtVK.mm
    )
endif()

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
    ${METAL_SOURCES}
)

target_link_libraries(${TARGET}
    PUBLIC
        MethaneGraphicsCoreBase
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
        # Vulkan Libs
        $<$<NOT:$<BOOL:${APPLE}>>:Vulkan-Headers> # Lin/Win: Dynamic linking with Vulkan, only Vulkan headers are needed
        $<$<BOOL:${LINUX}>:dl>                    # Linux: Link with dynamic linker for vk::DynamicLoader on Linux
        $<$<BOOL:${APPLE}>:Vulkan::Vulkan>        # MacOS: Link statically with MoltenVK framework on MacOS
        spirv-cross-core
        spirv-cross-hlsl
)

# Add cyclic dependency of the CoreBase library from CoreVulkan due to symbolic dependency
# from CoreInterface fabric methods ISomething::Create(...), which is required for correct linking with GCC
target_link_libraries(MethaneGraphicsCoreBase PRIVATE ${TARGET})

target_include_directories(${TARGET}
    PRIVATE
        Sources
        $<$<BOOL:${APPLE}>:${CMAKE_CURRENT_SOURCE_DIR}/${METAL_SOURCE_DIR}/Include>
    PUBLIC
        Include
)

target_compile_definitions(${TARGET}
    PUBLIC
        $<$<BOOL:${METHANE_COMMAND_DEBUG_GROUPS_ENABLED}>:METHANE_COMMAND_DEBUG_GROUPS_ENABLED>
    PRIVATE
        METHANE_GFX_VULKAN
        VK_NO_PROTOTYPES
        $<$<BOOL:${METHANE_GPU_INSTRUMENTATION_ENABLED}>:METHANE_GPU_INSTRUMENTATION_ENABLED=1> # 1 - Methane, 2 - Tracy
        $<$<BOOL:${WIN32}>:VK_USE_PLATFORM_WIN32_KHR>
        $<$<BOOL:${APPLE}>:VK_USE_PLATFORM_METAL_EXT>
        $<$<BOOL:${LINUX}>:VK_USE_PLATFORM_XCB_KHR>
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Graphics/Core
        UNITY_BUILD ${METHANE_UNITY_BUILD_ENABLED}
        UNITY_BUILD_BATCH_SIZE 4
)

if (APPLE)
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/${METAL_SOURCE_DIR} FILES ${METAL_SOURCES})
    set_source_files_properties(
        ${PLATFORM_SOURCES_OBJC}
        PROPERTIES
            COMPILE_FLAGS -fobjc-weak # CLANG_ENABLE_OBJC_WEAK = YES
            SKIP_PRECOMPILE_HEADERS ON
            SKIP_UNITY_BUILD_INCLUSION ON
    )
else()
    # Precompiled headers are disable on Apple because of weird CMake error:
    # Unable to resolve full path of PCH-header
    target_precompile_headers(${TARGET} REUSE_FROM MethanePrecompiledExtraHeaders)
endif()
