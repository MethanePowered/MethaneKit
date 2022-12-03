set(TARGET MethaneGraphicsRhiMetal)

include(MethaneModules)

get_module_dirs("Methane/Graphics/Metal")

list(APPEND HEADERS
    ${INCLUDE_DIR}/Types.hh
    ${INCLUDE_DIR}/Device.hh
    ${INCLUDE_DIR}/Fence.hh
    ${INCLUDE_DIR}/IContext.h
    ${INCLUDE_DIR}/Context.hpp
    ${INCLUDE_DIR}/RenderContext.hh
    ${INCLUDE_DIR}/RenderContextAppView.hh
    ${INCLUDE_DIR}/Shader.hh
    ${INCLUDE_DIR}/Program.hh
    ${INCLUDE_DIR}/ProgramLibrary.hh
    ${INCLUDE_DIR}/ProgramArgumentBinding.hh
    ${INCLUDE_DIR}/ProgramBindings.hh
    ${INCLUDE_DIR}/RenderState.hh
    ${INCLUDE_DIR}/Resource.hh
    ${INCLUDE_DIR}/DescriptorManager.h
    ${INCLUDE_DIR}/Buffer.hh
    ${INCLUDE_DIR}/Texture.hh
    ${INCLUDE_DIR}/Sampler.hh
    ${INCLUDE_DIR}/RenderPass.hh
    ${INCLUDE_DIR}/CommandQueue.hh
    ${INCLUDE_DIR}/CommandList.hh
    ${INCLUDE_DIR}/CommandList.hpp
    ${INCLUDE_DIR}/TransferCommandList.hh
    ${INCLUDE_DIR}/RenderCommandList.hh
    ${INCLUDE_DIR}/ParallelRenderCommandList.hh
)

list(APPEND SOURCES
    ${SOURCES_DIR}/Types.mm
    ${SOURCES_DIR}/Device.mm
    ${SOURCES_DIR}/Fence.mm
    ${SOURCES_DIR}/RenderContext.mm
    ${SOURCES_DIR}/Shader.mm
    ${SOURCES_DIR}/Program.mm
    ${SOURCES_DIR}/ProgramLibrary.mm
    ${SOURCES_DIR}/ProgramArgumentBinding.mm
    ${SOURCES_DIR}/ProgramBindings.mm
    ${SOURCES_DIR}/RenderState.mm
    ${SOURCES_DIR}/Resource.mm
    ${SOURCES_DIR}/QueryPool.mm
    ${SOURCES_DIR}/Buffer.mm
    ${SOURCES_DIR}/Texture.mm
    ${SOURCES_DIR}/Sampler.mm
    ${SOURCES_DIR}/RenderPass.mm
    ${SOURCES_DIR}/CommandQueue.mm
    ${SOURCES_DIR}/CommandList.mm
    ${SOURCES_DIR}/TransferCommandList.mm
    ${SOURCES_DIR}/RenderCommandList.mm
    ${SOURCES_DIR}/ParallelRenderCommandList.mm
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
        "-framework Metal"
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
        METHANE_GFX_METAL
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Graphics/RHI
        UNITY_BUILD ${METHANE_UNITY_BUILD_ENABLED}
        UNITY_BUILD_BATCH_SIZE 4
)

set_source_files_properties(
    ${SOURCES}
    PROPERTIES
        COMPILE_FLAGS -fobjc-weak # CLANG_ENABLE_OBJC_WEAK = YES
        SKIP_PRECOMPILE_HEADERS ON
        SKIP_UNITY_BUILD_INCLUSION ON
)
