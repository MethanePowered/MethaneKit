set(TARGET MethaneGraphicsRhiVulkan)

include(MethaneModules)

get_module_dirs("Methane/Graphics/Vulkan")

list(APPEND HEADERS
    ${INCLUDE_DIR}/Platform.h
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
    ${INCLUDE_DIR}/ResourceBarriers.h
    ${INCLUDE_DIR}/DescriptorManager.h
    ${INCLUDE_DIR}/QueryPool.h
    ${INCLUDE_DIR}/Resource.hpp
    ${INCLUDE_DIR}/Buffer.h
    ${INCLUDE_DIR}/Texture.h
    ${INCLUDE_DIR}/Sampler.h
    ${INCLUDE_DIR}/RenderPass.h
    ${INCLUDE_DIR}/CommandQueue.h
    ${INCLUDE_DIR}/CommandList.h
    ${INCLUDE_DIR}/CommandList.hpp
    ${INCLUDE_DIR}/TransferCommandList.h
    ${INCLUDE_DIR}/RenderCommandList.h
    ${INCLUDE_DIR}/ParallelRenderCommandList.h
    ${INCLUDE_DIR}/Utils.hpp
)

list(APPEND SOURCES
    ${SOURCES_DIR}/Platform.cpp
    ${SOURCES_DIR}/${PLATFORM_DIR}/PlatformExt.${CPP_EXT}
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
    ${SOURCES_DIR}/QueryPool.cpp
    ${SOURCES_DIR}/Buffer.cpp
    ${SOURCES_DIR}/Texture.cpp
    ${SOURCES_DIR}/Sampler.cpp
    ${SOURCES_DIR}/RenderPass.cpp
    ${SOURCES_DIR}/CommandQueue.cpp
    ${SOURCES_DIR}/CommandList.cpp
    ${SOURCES_DIR}/TransferCommandList.cpp
    ${SOURCES_DIR}/RenderCommandList.cpp
    ${SOURCES_DIR}/ParallelRenderCommandList.cpp
)

if (APPLE)
    set(METAL_DIR ../Metal)
    set(METAL_SOURCES_DIR ${METAL_DIR}/Sources/Methane/Graphics/Metal)
    set(METAL_INCLUDE_DIR ${METAL_DIR}/Include/Methane/Graphics/Metal)
    list(APPEND METAL_SOURCES
        ${METAL_INCLUDE_DIR}/RenderContextAppView.hh
        ${METAL_INCLUDE_DIR}/Types.hh
        ${METAL_SOURCES_DIR}/Types.mm
    )
    list(APPEND SOURCES
        ${SOURCES_DIR}/RenderContext.mm
    )
    set(PLATFORM_SOURCES_OBJC
        ${METAL_SOURCES}
        ${SOURCES_DIR}/RenderContext.mm
        ${SOURCES_GRAPHICS_DIR}/${PLATFORM_DIR}/PlatformExt.mm
    )
endif()

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
    ${METAL_SOURCES}
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
        # Vulkan Libs
        $<$<NOT:$<BOOL:${APPLE}>>:Vulkan-Headers> # Lin/Win: Dynamic linking with Vulkan, only Vulkan headers are needed
        $<$<BOOL:${LINUX}>:dl>                    # Linux: Link with dynamic linker for vk::DynamicLoader on Linux
        $<$<BOOL:${APPLE}>:Vulkan::Vulkan>        # MacOS: Link statically with Molten framework on MacOS
        spirv-cross-core
        spirv-cross-hlsl
)

# Add cyclic dependency of the RhiBase library from RhiDirectX due to symbolic dependency
# from RhiInterface fabric methods ISomething::Create(...) implementations defined in RhiDirectX
target_link_libraries(MethaneGraphicsRhiBase PRIVATE ${TARGET})

target_include_directories(${TARGET}
    PRIVATE
        Sources
        $<$<BOOL:${APPLE}>:${CMAKE_CURRENT_SOURCE_DIR}/${METAL_DIR}/Include>
    PUBLIC
        Include
)

target_compile_definitions(${TARGET}
    PUBLIC
        $<$<BOOL:${METHANE_COMMAND_DEBUG_GROUPS_ENABLED}>:METHANE_COMMAND_DEBUG_GROUPS_ENABLED>
    PRIVATE
        METHANE_GFX_VULKAN
        _NO_PROTOTYPES
        $<$<BOOL:${METHANE_GPU_INSTRUMENTATION_ENABLED}>:METHANE_GPU_INSTRUMENTATION_ENABLED=1> # 1 - Methane, 2 - Tracy
        $<$<BOOL:${WIN32}>:_USE_PLATFORM_WIN32_KHR>
        $<$<BOOL:${APPLE}>:_USE_PLATFORM_METAL_EXT>
        $<$<BOOL:${LINUX}>:_USE_PLATFORM_XCB_KHR>
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Graphics/RHI
        UNITY_BUILD ${METHANE_UNITY_BUILD_ENABLED}
        UNITY_BUILD_BATCH_SIZE 4
)

if (APPLE)
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/${METAL_DIR} FILES ${METAL_SOURCES})
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