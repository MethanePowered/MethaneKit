set(TARGET MethaneGraphicsRhiImpl)

include(MethaneModules)

get_module_dirs("Methane/Graphics/RHI")

list(APPEND HEADERS
    ${INCLUDE_DIR}/Implementations.h
    ${INCLUDE_DIR}/Device.h
    ${INCLUDE_DIR}/System.h
    ${INCLUDE_DIR}/CommandListSet.h
    ${INCLUDE_DIR}/CommandQueue.h
    ${INCLUDE_DIR}/CommandListDebugGroup.h
    ${INCLUDE_DIR}/CommandKit.h
    ${INCLUDE_DIR}/Fence.h
    ${INCLUDE_DIR}/Shader.h
    ${INCLUDE_DIR}/Program.h
    ${INCLUDE_DIR}/ProgramBindings.h
    ${INCLUDE_DIR}/RenderPattern.h
    ${INCLUDE_DIR}/RenderPass.h
    ${INCLUDE_DIR}/RenderContext.h
    ${INCLUDE_DIR}/ComputeContext.h
    ${INCLUDE_DIR}/RenderState.h
    ${INCLUDE_DIR}/ViewState.h
    ${INCLUDE_DIR}/ComputeState.h
    ${INCLUDE_DIR}/Buffer.h
    ${INCLUDE_DIR}/BufferSet.h
    ${INCLUDE_DIR}/Texture.h
    ${INCLUDE_DIR}/Sampler.h
    ${INCLUDE_DIR}/ResourceBarriers.h
    ${INCLUDE_DIR}/RenderCommandList.h
    ${INCLUDE_DIR}/ParallelRenderCommandList.h
    ${INCLUDE_DIR}/TransferCommandList.h
    ${INCLUDE_DIR}/ComputeCommandList.h
)

list(APPEND SOURCES
    ${SOURCES_DIR}/Device.cpp
    ${SOURCES_DIR}/System.cpp
    ${SOURCES_DIR}/CommandListSet.cpp
    ${SOURCES_DIR}/CommandQueue.cpp
    ${SOURCES_DIR}/CommandListDebugGroup.cpp
    ${SOURCES_DIR}/CommandKit.cpp
    ${SOURCES_DIR}/Fence.cpp
    ${SOURCES_DIR}/Shader.cpp
    ${SOURCES_DIR}/Program.cpp
    ${SOURCES_DIR}/ProgramBindings.cpp
    ${SOURCES_DIR}/RenderPattern.cpp
    ${SOURCES_DIR}/RenderPass.cpp
    ${SOURCES_DIR}/RenderContext.cpp
    ${SOURCES_DIR}/ComputeContext.cpp
    ${SOURCES_DIR}/RenderState.cpp
    ${SOURCES_DIR}/ViewState.cpp
    ${SOURCES_DIR}/ComputeState.cpp
    ${SOURCES_DIR}/Buffer.cpp
    ${SOURCES_DIR}/BufferSet.cpp
    ${SOURCES_DIR}/Texture.cpp
    ${SOURCES_DIR}/Sampler.cpp
    ${SOURCES_DIR}/ResourceBarriers.cpp
    ${SOURCES_DIR}/RenderCommandList.cpp
    ${SOURCES_DIR}/ParallelRenderCommandList.cpp
    ${SOURCES_DIR}/TransferCommandList.cpp
    ${SOURCES_DIR}/ComputeCommandList.cpp
)

if (METHANE_GFX_API EQUAL METHANE_GFX_DIRECTX)
    set(METHANE_GRAPHICS_RHI_IMPL_TARGET MethaneGraphicsRhiDirectX)
    set(METHANE_GRAPHICS_API_NAME DirectX)
elseif(METHANE_GFX_API EQUAL METHANE_GFX_VULKAN)
    set(METHANE_GRAPHICS_RHI_IMPL_TARGET MethaneGraphicsRhiVulkan)
    set(METHANE_GRAPHICS_API_NAME Vulkan)
elseif(METHANE_GFX_API EQUAL METHANE_GFX_METAL)
    set(METHANE_GRAPHICS_RHI_IMPL_TARGET MethaneGraphicsRhiMetal)
    set(METHANE_GRAPHICS_API_NAME Metal)
else()
    message(FATAL_ERROR "Methane Graphics API is undefined!")
endif()

get_target_property(METHANE_GRAPHICS_RHI_IMPL_INCLUDE_DIR ${METHANE_GRAPHICS_RHI_IMPL_TARGET} METHANE_INCLUDE_DIR)

if(METHANE_RHI_PIMPL_INLINE_ENABLED)

    list(APPEND PUBLIC_HEADERS
        ${HEADERS}
        ${SOURCES}
    )

    add_library(${TARGET} STATIC
        ${HEADERS}
        ${SOURCES_DIR}/Implementations.cpp
    )

    target_link_libraries(${TARGET}
        PUBLIC
            MethaneBuildOptions
            ${METHANE_GRAPHICS_RHI_IMPL_TARGET}
    )

    target_include_directories(${TARGET}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/Include
            ${CMAKE_CURRENT_SOURCE_DIR}/Sources
            ${METHANE_GRAPHICS_RHI_IMPL_INCLUDE_DIR}
    )

    target_compile_definitions(${TARGET}
        PUBLIC
            META_PIMPL_INLINE
            META_GFX_NAME=${METHANE_GRAPHICS_API_NAME}
            $<$<EQUAL:${METHANE_GFX_API},${METHANE_GFX_METAL}>:META_GFX_METAL>
    )

else() # METHANE_RHI_PIMPL_INLINE_ENABLED

    list(APPEND PUBLIC_HEADERS
        ${HEADERS}
    )

    add_library(${TARGET} STATIC
        ${HEADERS}
        ${SOURCES}
    )

    target_link_libraries(${TARGET}
        PUBLIC
            MethaneGraphicsRhiInterface
        PRIVATE
            ${METHANE_GRAPHICS_RHI_IMPL_TARGET}
    )

    target_include_directories(${TARGET}
        PUBLIC
            Include
        PRIVATE
            ${METHANE_GRAPHICS_RHI_IMPL_INCLUDE_DIR}
    )

    target_compile_definitions(${TARGET}
        PUBLIC
            META_GFX_NAME=${METHANE_GRAPHICS_API_NAME}
            $<$<EQUAL:${METHANE_GFX_API},${METHANE_GFX_METAL}>:META_GFX_METAL>
            # Precompiled headers is going to be reused by other targets and it has to be included with the same definitions
            $<$<BOOL:METHANE_PRECOMPILED_HEADERS_ENABLED>:$<TARGET_PROPERTY:${METHANE_GRAPHICS_RHI_IMPL_TARGET},COMPILE_DEFINITIONS>>
        )

    if (METHANE_GFX_API EQUAL METHANE_GFX_METAL)
        set_source_files_properties(${SOURCES}
            PROPERTIES
                COMPILE_FLAGS "-x objective-c++"
                SKIP_PRECOMPILE_HEADERS ON
                SKIP_UNITY_BUILD_INCLUSION ON
        )
    endif()

endif() # METHANE_RHI_PIMPL_INLINE_ENABLED

if(METHANE_PRECOMPILED_HEADERS_ENABLED)
    target_precompile_headers(${TARGET}
        PUBLIC
            <Methane/Graphics/RHI/Implementations.h>
    )
endif()

add_prerequisite_modules(${TARGET} "${METHANE_GRAPHICS_RHI_IMPL_TARGET}")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Graphics/RHI
        PUBLIC_HEADER "${PUBLIC_HEADERS}"
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

if(METHANE_TESTS_BUILD_ENABLED)

    set(TEST_TARGET MethaneGraphicsRhiNullImpl)

    add_library(${TEST_TARGET} STATIC
        ${HEADERS}
        ${SOURCES}
    )

    target_link_libraries(${TEST_TARGET}
        PUBLIC
            MethaneGraphicsRhiInterface
        PRIVATE
            MethaneBuildOptions
            MethaneGraphicsRhiNull
    )

    target_include_directories(${TEST_TARGET}
        PUBLIC
            Include
        PRIVATE
            $<TARGET_PROPERTY:MethaneGraphicsRhiNull,METHANE_INCLUDE_DIR>
    )

    target_compile_definitions(${TEST_TARGET}
        PUBLIC
            META_GFX_NAME=Null
            # Precompiled headers is going to be reused by other targets and it has to be included with the same definitions
            $<$<BOOL:METHANE_PRECOMPILED_HEADERS_ENABLED>:$<TARGET_PROPERTY:${METHANE_GRAPHICS_RHI_IMPL_TARGET},COMPILE_DEFINITIONS>>
    )

    if(METHANE_PRECOMPILED_HEADERS_ENABLED)
        target_precompile_headers(${TEST_TARGET}
            PUBLIC
                <Methane/Graphics/RHI/Implementations.h>
        )
    endif()

endif() # METHANE_TESTS_BUILD_ENABLED
