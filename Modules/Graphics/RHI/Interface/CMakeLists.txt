set(TARGET MethaneGraphicsRhiInterface)

include(MethaneModules)

get_module_dirs("Methane/Graphics/RHI")

list(APPEND HEADERS
    ${INCLUDE_DIR}/IObject.h
    ${INCLUDE_DIR}/IDevice.h
    ${INCLUDE_DIR}/ISystem.h
    ${INCLUDE_DIR}/IContext.h
    ${INCLUDE_DIR}/IRenderContext.h
    ${INCLUDE_DIR}/IComputeContext.h
    ${INCLUDE_DIR}/IFence.h
    ${INCLUDE_DIR}/IShader.h
    ${INCLUDE_DIR}/IProgram.h
    ${INCLUDE_DIR}/IProgramBindings.h
    ${INCLUDE_DIR}/IRenderState.h
    ${INCLUDE_DIR}/IViewState.h
    ${INCLUDE_DIR}/IComputeState.h
    ${INCLUDE_DIR}/IResource.h
    ${INCLUDE_DIR}/IResourceBarriers.h
    ${INCLUDE_DIR}/ResourceView.h
    ${INCLUDE_DIR}/IBuffer.h
    ${INCLUDE_DIR}/IBufferSet.h
    ${INCLUDE_DIR}/ITexture.h
    ${INCLUDE_DIR}/ISampler.h
    ${INCLUDE_DIR}/IRenderPattern.h
    ${INCLUDE_DIR}/IRenderPass.h
    ${INCLUDE_DIR}/ICommandKit.h
    ${INCLUDE_DIR}/ICommandQueue.h
    ${INCLUDE_DIR}/ICommandList.h
    ${INCLUDE_DIR}/ICommandListDebugGroup.h
    ${INCLUDE_DIR}/ICommandListSet.h
    ${INCLUDE_DIR}/ITransferCommandList.h
    ${INCLUDE_DIR}/IComputeCommandList.h
    ${INCLUDE_DIR}/IRenderCommandList.h
    ${INCLUDE_DIR}/IParallelRenderCommandList.h
    ${INCLUDE_DIR}/IQueryPool.h
    ${INCLUDE_DIR}/IDescriptorManager.h
    ${INCLUDE_DIR}/TypeFormatters.hpp
)

list(APPEND SOURCES
    ${SOURCES_DIR}/IObject.cpp
    ${SOURCES_DIR}/IDevice.cpp
    ${SOURCES_DIR}/ISystem.cpp
    ${SOURCES_DIR}/IFence.cpp
    ${SOURCES_DIR}/IContext.cpp
    ${SOURCES_DIR}/IRenderContext.cpp
    ${SOURCES_DIR}/IComputeContext.cpp
    ${SOURCES_DIR}/IShader.cpp
    ${SOURCES_DIR}/IProgram.cpp
    ${SOURCES_DIR}/IProgramBindings.cpp
    ${SOURCES_DIR}/IRenderState.cpp
    ${SOURCES_DIR}/IViewState.cpp
    ${SOURCES_DIR}/IComputeState.cpp
    ${SOURCES_DIR}/IResource.cpp
    ${SOURCES_DIR}/IResourceBarriers.cpp
    ${SOURCES_DIR}/IBuffer.cpp
    ${SOURCES_DIR}/ITexture.cpp
    ${SOURCES_DIR}/ISampler.cpp
    ${SOURCES_DIR}/IQueryPool.cpp
    ${SOURCES_DIR}/IRenderPattern.cpp
    ${SOURCES_DIR}/IRenderPass.cpp
    ${SOURCES_DIR}/ICommandKit.cpp
    ${SOURCES_DIR}/ICommandQueue.cpp
    ${SOURCES_DIR}/ITransferCommandList.cpp
    ${SOURCES_DIR}/IComputeCommandList.cpp
    ${SOURCES_DIR}/IRenderCommandList.cpp
    ${SOURCES_DIR}/IParallelRenderCommandList.cpp
    ${SOURCES_DIR}/ResourceView.cpp
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
        MethaneDataPrimitives
        MethaneGraphicsTypes
        MethanePlatformAppView
    PRIVATE
        MethaneBuildOptions
        MethaneInstrumentation
        MethaneMathPrecompiledHeaders
        nowide
        magic_enum
)

if(METHANE_PRECOMPILED_HEADERS_ENABLED)
    target_precompile_headers(${TARGET} REUSE_FROM MethaneMathPrecompiledHeaders)
endif()

target_include_directories(${TARGET}
    PUBLIC
        Include
)

target_compile_definitions(${TARGET}
    PUBLIC
        $<$<BOOL:${METHANE_COMMAND_DEBUG_GROUPS_ENABLED}>:METHANE_COMMAND_DEBUG_GROUPS_ENABLED>
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

install(TARGETS ${TARGET}
    PUBLIC_HEADER
        DESTINATION ${INCLUDE_DIR}
        COMPONENT Development
    ARCHIVE
        DESTINATION Lib
        COMPONENT Development
)
