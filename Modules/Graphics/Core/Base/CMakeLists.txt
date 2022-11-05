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
)

target_precompile_headers(${TARGET} REUSE_FROM MethanePrecompiledHeaders)

target_include_directories(${TARGET}
    PUBLIC
        Include
    PRIVATE
        Sources
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Graphics/Core
        UNITY_BUILD ${METHANE_UNITY_BUILD_ENABLED}
        UNITY_BUILD_BATCH_SIZE 6
)
