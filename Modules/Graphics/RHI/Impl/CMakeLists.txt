set(TARGET MethaneGraphicsRhi)

add_library(${TARGET} INTERFACE)

target_link_libraries(${TARGET}
    INTERFACE
        $<$<EQUAL:${METHANE_GFX_API},${METHANE_GFX_DIRECTX}>:MethaneGraphicsRhiDirectX>
        $<$<EQUAL:${METHANE_GFX_API},${METHANE_GFX_VULKAN}>:MethaneGraphicsRhiVulkan>
        $<$<EQUAL:${METHANE_GFX_API},${METHANE_GFX_METAL}>:MethaneGraphicsRhiMetal>
)
