set(TARGET MethanePrecompiledHeaders)

include(MethaneModules)

get_module_dirs("Methane")

# ===== Common precompiled headers =====

if (WIN32)

    set(PRECOMPILED_HEADERS
        <windows.h>
        <wrl.h>
    )

endif()

set(PRECOMPILED_HEADERS ${PRECOMPILED_HEADERS}
    # Common Methane headers
    $<TARGET_PROPERTY:MethanePrimitives,SOURCE_DIR>/Include/Methane/Memory.hpp
    $<TARGET_PROPERTY:MethanePrimitives,SOURCE_DIR>/Include/Methane/Checks.hpp
    $<TARGET_PROPERTY:MethaneInstrumentation,SOURCE_DIR>/Include/Methane/Instrumentation.h

    # Common External headers
    <fmt/format.h>
    <nowide/convert.hpp>
    <magic_enum.hpp>

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

add_library(${TARGET} STATIC
    ${SOURCES_DIR}/PrecompiledHeaders.cpp
)

target_link_libraries(${TARGET}
    PUBLIC
        MethaneBuildOptions
        MethanePrimitives
        MethaneInstrumentation
        fmt
        nowide
        magic_enum
)

target_precompile_headers(${TARGET}
    PUBLIC
        ${PRECOMPILED_HEADERS}
)

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Common
)

# ===== Extra precompiled headers =====

set(EXTRA_TARGET MethanePrecompiledExtraHeaders)

set(PRECOMPILED_EXTRA_HEADERS ${PRECOMPILED_HEADERS}
    <hlsl++.h>
    <taskflow/taskflow.hpp>
)

if (WIN32)

    set(PRECOMPILED_EXTRA_HEADERS ${PRECOMPILED_EXTRA_HEADERS}
        <d3d12.h>
        <d3dx12.h>
    )

    set(PLATFORM_LIBRARIES
        D3DX12
    )

endif()

add_library(${EXTRA_TARGET} STATIC
    ${SOURCES_DIR}/PrecompiledHeaders.cpp
)

target_link_libraries(${EXTRA_TARGET}
    PUBLIC
        MethaneBuildOptions
        MethaneInstrumentation
        TaskFlow
        HLSLpp
        fmt
        nowide
        magic_enum
        ${PLATFORM_LIBRARIES}
)

target_precompile_headers(${EXTRA_TARGET}
    PUBLIC
        ${PRECOMPILED_EXTRA_HEADERS}
)

set_target_properties(${EXTRA_TARGET}
    PROPERTIES
        FOLDER Modules/Common
)