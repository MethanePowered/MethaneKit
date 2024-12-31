/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License"),
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Graphics/RHI/IShader.h
Methane shader interface: defines programmable stage of the graphics pipeline.

******************************************************************************/

#pragma once

#include <Methane/Memory.hpp>

#include <string>
#include <string_view>
#include <set>
#include <map>

namespace Methane::Data
{
struct IProvider;
}

namespace Methane::Graphics::Rhi
{

enum class ShaderType : uint32_t
{
    Vertex,
    Pixel,
    Compute,
    All
};

using ShaderTypes = std::set<ShaderType>;
static const ShaderTypes g_all_shader_types = { ShaderType::Vertex, ShaderType::Pixel };

struct ShaderMacroDefinition
{
    std::string name;
    std::string value;

    explicit ShaderMacroDefinition(std::string name);
    ShaderMacroDefinition(std::string name, std::string value);

    friend bool operator==(const ShaderMacroDefinition& left, const ShaderMacroDefinition& right) noexcept
    {
        return std::tie(left.name, left.value) == std::tie(right.name, right.value);
    }

    friend bool operator!=(const ShaderMacroDefinition& left, const ShaderMacroDefinition& right) noexcept
    {
        return !(left == right);
    }

    [[nodiscard]] static std::string ToString(const std::vector<ShaderMacroDefinition>& macro_definitions,
                                              std::string_view splitter = ", ") noexcept;
};

using ShaderMacroDefinitions = std::vector<ShaderMacroDefinition>;

struct ShaderEntryFunction
{
    std::string file_name;
    std::string function_name;

    friend bool operator==(const ShaderEntryFunction& left, const ShaderEntryFunction& right) noexcept
    {
        return std::tie(left.file_name, left.function_name)
            == std::tie(right.file_name, right.function_name);
    }

    friend bool operator!=(const ShaderEntryFunction& left, const ShaderEntryFunction& right) noexcept
    {
        return !(left == right);
    }
};

struct ShaderSettings
{
    Data::IProvider&       data_provider;
    ShaderEntryFunction    entry_function;
    ShaderMacroDefinitions compile_definitions;

    // Optional parameters (by default shaders are precompiled to application resources and loaded through Data::IProvider)
    std::string source_file_path;
    std::string source_compile_target;

    friend bool operator==(const ShaderSettings& left, const ShaderSettings& right) noexcept
    {
        if (std::addressof(left.data_provider) != std::addressof(right.data_provider))
            return false;

        return std::tie(left.entry_function, left.compile_definitions, left.source_file_path, left.source_compile_target)
            == std::tie(right.entry_function, right.compile_definitions, right.source_file_path, right.source_compile_target);
    }

    friend bool operator!=(const ShaderSettings& left, const ShaderSettings& right) noexcept
    {
        return !(left == right);
    }
};

struct IContext;

struct IShader
{
    using Type             = ShaderType;
    using Types            = ShaderTypes;
    using MacroDefinition  = ShaderMacroDefinition;
    using MacroDefinitions = ShaderMacroDefinitions;
    using EntryFunction    = ShaderEntryFunction;
    using Settings         = ShaderSettings;

    // Create IShader instance
    [[nodiscard]] static Ptr<IShader> Create(Type type, const IContext& context, const Settings& settings);

    // IShader interface
    [[nodiscard]] virtual Ptr<IShader>    GetPtr() = 0;
    [[nodiscard]] virtual Type            GetType() const noexcept = 0;
    [[nodiscard]] virtual const Settings& GetSettings() const noexcept = 0;

    virtual ~IShader() = default;
};

} // namespace Methane::Graphics::Rhi
