set(TARGET MethaneGraphicsCore)

add_library(${TARGET} INTERFACE)

target_link_libraries(${TARGET}
    INTERFACE
        MethaneGraphicsCoreBase
        $<$<EQUAL:${METHANE_GFX_API},${METHANE_GFX_METAL}>:MethaneGraphicsCoreMetal>
)
