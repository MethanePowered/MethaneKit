set(TARGET MethaneGraphicsCoreMetal)

include(MethaneModules)

get_module_dirs("Methane/Graphics")

list(APPEND HEADERS
    ${INCLUDE_DIR}/TypesMT.hh
    ${INCLUDE_DIR}/DeviceMT.hh
    ${INCLUDE_DIR}/FenceMT.hh
    ${INCLUDE_DIR}/ContextMT.h
    ${INCLUDE_DIR}/RenderContextMT.hh
    ${INCLUDE_DIR}/RenderContextAppViewMT.hh
    ${INCLUDE_DIR}/ShaderMT.hh
    ${INCLUDE_DIR}/ProgramMT.hh
    ${INCLUDE_DIR}/ProgramLibraryMT.hh
    ${INCLUDE_DIR}/ProgramArgumentBindingMT.hh
    ${INCLUDE_DIR}/ProgramBindingsMT.hh
    ${INCLUDE_DIR}/RenderStateMT.hh
    ${INCLUDE_DIR}/ResourceMT.hh
    ${INCLUDE_DIR}/DescriptorManagerMT.h
    ${INCLUDE_DIR}/BufferMT.hh
    ${INCLUDE_DIR}/TextureMT.hh
    ${INCLUDE_DIR}/SamplerMT.hh
    ${INCLUDE_DIR}/RenderPassMT.hh
    ${INCLUDE_DIR}/CommandQueueMT.hh
    ${INCLUDE_DIR}/CommandListMT.hh
    ${INCLUDE_DIR}/TransferCommandListMT.hh
    ${INCLUDE_DIR}/RenderCommandListMT.hh
    ${INCLUDE_DIR}/ParallelRenderCommandListMT.hh
)

list(APPEND SOURCES
    ${SOURCES_DIR}/TypesMT.mm
    ${SOURCES_DIR}/DeviceMT.mm
    ${SOURCES_DIR}/FenceMT.mm
    ${SOURCES_DIR}/ContextMT.hpp
    ${SOURCES_DIR}/RenderContextMT.mm
    ${SOURCES_DIR}/ShaderMT.mm
    ${SOURCES_DIR}/ProgramMT.mm
    ${SOURCES_DIR}/ProgramLibraryMT.mm
    ${SOURCES_DIR}/ProgramArgumentBindingMT.mm
    ${SOURCES_DIR}/ProgramBindingsMT.mm
    ${SOURCES_DIR}/RenderStateMT.mm
    ${SOURCES_DIR}/ResourceMT.mm
    ${SOURCES_DIR}/QueryPoolMT.mm
    ${SOURCES_DIR}/BufferMT.mm
    ${SOURCES_DIR}/TextureMT.mm
    ${SOURCES_DIR}/SamplerMT.mm
    ${SOURCES_DIR}/RenderPassMT.mm
    ${SOURCES_DIR}/CommandQueueMT.mm
    ${SOURCES_DIR}/CommandListMT.hpp
    ${SOURCES_DIR}/CommandListMT.mm
    ${SOURCES_DIR}/TransferCommandListMT.mm
    ${SOURCES_DIR}/RenderCommandListMT.mm
    ${SOURCES_DIR}/ParallelRenderCommandListMT.mm
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
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
        "-framework Metal"
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
        METHANE_GFX_METAL
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Graphics/Core
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
