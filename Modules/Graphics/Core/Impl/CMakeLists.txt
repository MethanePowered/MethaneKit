set(TARGET MethaneGraphicsCore)

add_library(${TARGET} INTERFACE)

target_link_libraries(${TARGET}
    INTERFACE
        $<$<EQUAL:${METHANE_GFX_API},${METHANE_GFX_DIRECTX}>:MethaneGraphicsCoreDirectX>
        $<$<EQUAL:${METHANE_GFX_API},${METHANE_GFX_VULKAN}>:MethaneGraphicsCoreVulkan>
        $<$<EQUAL:${METHANE_GFX_API},${METHANE_GFX_METAL}>:MethaneGraphicsCoreMetal>
)
