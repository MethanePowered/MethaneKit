set(COM_TARGET MethaneCommonPrecompiledHeaders)
set(MAT_TARGET MethaneMathPrecompiledHeaders)

if (NOT METHANE_PRECOMPILED_HEADERS_ENABLED)
    add_library(${COM_TARGET} INTERFACE)
    add_library(${MAT_TARGET} INTERFACE)
    return()
endif()

include(MethaneModules)
get_module_dirs("Methane")

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

list(APPEND PRECOMPILED_LIBS
    MethaneBuildOptions
    MethanePrimitives
    MethaneInstrumentation
    fmt
    nowide
)

add_library(${COM_TARGET} STATIC
    ${SOURCES_DIR}/PrecompiledHeaders.cpp
)

target_link_libraries(${COM_TARGET}
    PUBLIC
        ${PRECOMPILED_LIBS}
)

target_precompile_headers(${COM_TARGET}
    PUBLIC
        ${PRECOMPILED_HEADERS}
)

set_target_properties(${COM_TARGET}
    PROPERTIES
        FOLDER Modules/Common
)

# ===== Math precompiled headers =====

list(APPEND PRECOMPILED_MATH_HEADERS
    ${PRECOMPILED_HEADERS}
    <hlsl++.h>
    <taskflow/taskflow.hpp>
)

list(APPEND PRECOMPILED_MATH_LIBS
    ${PRECOMPILED_LIBS}
    HLSLpp
    TaskFlow
)

add_library(${MAT_TARGET} STATIC
    ${SOURCES_DIR}/PrecompiledHeaders.cpp
)

target_link_libraries(${MAT_TARGET}
    PUBLIC
        ${PRECOMPILED_MATH_LIBS}
)

target_precompile_headers(${MAT_TARGET}
    PUBLIC
        ${PRECOMPILED_MATH_HEADERS}
)

set_target_properties(${MAT_TARGET}
    PROPERTIES
        FOLDER Modules/Common
)
