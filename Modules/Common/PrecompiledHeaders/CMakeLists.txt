set(TARGET MethanePrecompiledHeaders)
set(EXTRA_TARGET MethanePrecompiledExtraHeaders)

if (NOT METHANE_PRECOMPILED_HEADERS_ENABLED)
    add_library(${TARGET} INTERFACE)
    add_library(${EXTRA_TARGET} INTERFACE)
    return()
endif()

# ===== Common precompiled headers =====

if (WIN32)

    list(APPEND PRECOMPILED_HEADERS
        <windows.h>
        <wrl.h>
    )

endif()

list(APPEND PRECOMPILED_HEADERS
    # Common Methane headers
    $<TARGET_PROPERTY:MethanePrimitives,SOURCE_DIR>/Include/Methane/Memory.hpp
    $<TARGET_PROPERTY:MethanePrimitives,SOURCE_DIR>/Include/Methane/Checks.hpp
    $<TARGET_PROPERTY:MethaneInstrumentation,SOURCE_DIR>/Include/Methane/Instrumentation.h

    # Common External headers
    <fmt/format.h>
    <nowide/convert.hpp>

    # types
    <limits>
    <string>
    <string_view>
    <sstream>
    <tuple>
    <optional>
    <exception>

    # containers
    <iterator>
    <set>
    <unordered_set>
    <map>
    <unordered_map>
    <list>
    <vector>

    # utilities
    <sstream>
    <cassert>
    <cmath>
    <memory>
    <functional>
    <algorithm>
    <utility>
    <thread>
)

add_library(${TARGET} INTERFACE)

target_link_libraries(${TARGET}
    INTERFACE
        MethaneBuildOptions
        MethanePrimitives
        MethaneInstrumentation
        fmt
        nowide
)

target_precompile_headers(${TARGET}
    INTERFACE
        ${PRECOMPILED_HEADERS}
)

# ===== Extra precompiled headers =====

list(APPEND PRECOMPILED_EXTRA_HEADERS
    ${PRECOMPILED_HEADERS}
    <hlsl++.h>
    <taskflow/taskflow.hpp>
)

if (METHANE_GFX_API EQUAL METHANE_GFX_DIRECTX)

    list(APPEND PRECOMPILED_EXTRA_HEADERS
        <directx/d3d12.h>
        <directx/d3dx12.h>
    )

    list(APPEND PLATFORM_LIBRARIES
        DirectX-Headers
    )

endif()

if (METHANE_GFX_API EQUAL METHANE_GFX_VULKAN)

    list(APPEND PRECOMPILED_EXTRA_HEADERS
        <vulkan/vulkan.hpp>
    )

    list(APPEND PLATFORM_LIBRARIES
        Vulkan-Headers
    )

    list(APPEND PRECOMPILED_EXTRA_DEFINITIONS
        VK_NO_PROTOTYPES
        $<$<BOOL:${WIN32}>:VK_USE_PLATFORM_WIN32_KHR>
        $<$<BOOL:${APPLE}>:VK_USE_PLATFORM_METAL_EXT>
        $<$<BOOL:${LINUX}>:VK_USE_PLATFORM_XCB_KHR>
    )

endif()

add_library(${EXTRA_TARGET} INTERFACE)

target_link_libraries(${EXTRA_TARGET}
    INTERFACE
        MethaneBuildOptions
        MethaneInstrumentation
        TaskFlow
        HLSLpp
        fmt
        nowide
        ${PLATFORM_LIBRARIES}
)

target_precompile_headers(${EXTRA_TARGET}
    INTERFACE
        ${PRECOMPILED_EXTRA_HEADERS}
)

target_compile_definitions(${EXTRA_TARGET}
    INTERFACE
        ${PRECOMPILED_EXTRA_DEFINITIONS}
)
