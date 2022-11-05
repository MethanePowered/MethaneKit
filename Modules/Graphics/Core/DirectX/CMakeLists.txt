set(TARGET MethaneGraphicsCoreDirectX)

include(MethaneModules)

get_module_dirs("Methane/Graphics")

list(APPEND HEADERS
    ${INCLUDE_DIR}/TypesDX.h
    ${INCLUDE_DIR}/DeviceDX.h
    ${INCLUDE_DIR}/FenceDX.h
    ${INCLUDE_DIR}/ContextDX.h
    ${INCLUDE_DIR}/ContextDX.hpp
    ${INCLUDE_DIR}/ShaderDX.h
    ${INCLUDE_DIR}/ProgramDX.h
    ${INCLUDE_DIR}/ProgramArgumentBindingDX.h
    ${INCLUDE_DIR}/ProgramBindingsDX.h
    ${INCLUDE_DIR}/RenderContextDX.h
    ${INCLUDE_DIR}/RenderStateDX.h
    ${INCLUDE_DIR}/ResourceDX.h
    ${INCLUDE_DIR}/ResourceBarriersDX.h
    ${INCLUDE_DIR}/DescriptorManagerDX.h
    ${INCLUDE_DIR}/DescriptorHeapDX.h
    ${INCLUDE_DIR}/QueryPoolDX.h
    ${INCLUDE_DIR}/ResourceDX.hpp
    ${INCLUDE_DIR}/BufferDX.h
    ${INCLUDE_DIR}/TextureDX.h
    ${INCLUDE_DIR}/SamplerDX.h
    ${INCLUDE_DIR}/RenderPassDX.h
    ${INCLUDE_DIR}/CommandQueueDX.h
    ${INCLUDE_DIR}/CommandListDX.h
    ${INCLUDE_DIR}/CommandListDX.hpp
    ${INCLUDE_DIR}/TransferCommandListDX.h
    ${INCLUDE_DIR}/RenderCommandListDX.h
    ${INCLUDE_DIR}/ParallelRenderCommandListDX.h
)

list(APPEND SOURCES
    ${SOURCES_DIR}/TypesDX.cpp
    ${SOURCES_DIR}/DeviceDX.cpp
    ${SOURCES_DIR}/FenceDX.cpp
    ${SOURCES_DIR}/ShaderDX.cpp
    ${SOURCES_DIR}/ProgramDX.cpp
    ${SOURCES_DIR}/ProgramArgumentBindingDX.cpp
    ${SOURCES_DIR}/ProgramBindingsDX.cpp
    ${SOURCES_DIR}/RenderContextDX.cpp
    ${SOURCES_DIR}/RenderStateDX.cpp
    ${SOURCES_DIR}/ResourceDX.cpp
    ${SOURCES_DIR}/ResourceBarriersDX.cpp
    ${SOURCES_DIR}/DescriptorManagerDX.cpp
    ${SOURCES_DIR}/DescriptorHeapDX.cpp
    ${SOURCES_DIR}/QueryPoolDX.cpp
    ${SOURCES_DIR}/BufferDX.cpp
    ${SOURCES_DIR}/TextureDX.cpp
    ${SOURCES_DIR}/SamplerDX.cpp
    ${SOURCES_DIR}/RenderPassDX.cpp
    ${SOURCES_DIR}/CommandQueueDX.cpp
    ${SOURCES_DIR}/CommandListDX.cpp
    ${SOURCES_DIR}/TransferCommandListDX.cpp
    ${SOURCES_DIR}/RenderCommandListDX.cpp
    ${SOURCES_DIR}/ParallelRenderCommandListDX.cpp
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
        # DirectX Libs
        DirectX-Headers
        DirectXTex
        DirectXCompiler
        d3d12
        dxgi
        dxguid
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
        METHANE_GFX_DIRECTX
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Graphics/Core
        UNITY_BUILD ${METHANE_UNITY_BUILD_ENABLED}
        UNITY_BUILD_BATCH_SIZE 4
)

if(METHANE_GFX_API EQUAL METHANE_GFX_DIRECTX)
    set_target_properties(${TARGET} PROPERTIES PREREQUISITE_MODULES "DirectXCompiler")
endif()
