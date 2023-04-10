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

struct ShaderMacroDefinition
{
    std::string name;
    std::string value;

    explicit ShaderMacroDefinition(std::string name)
        : name(std::move(name))
    { }

    ShaderMacroDefinition(std::string name, std::string value)
        : name(std::move(name))
        , value(std::move(value))
    { }
};

using ShaderMacroDefinitions = std::vector<ShaderMacroDefinition>;

struct ShaderEntryFunction
{
    std::string file_name;
    std::string function_name;

    bool operator==(const ShaderEntryFunction& other) const noexcept;
    bool operator!=(const ShaderEntryFunction& other) const noexcept;
};

struct ShaderSettings
{
    Data::IProvider&       data_provider;
    ShaderEntryFunction    entry_function;
    ShaderMacroDefinitions compile_definitions;

    // Optional parameters (by default shaders are precompiled to application resources and loaded through Data::IProvider)
    std::string source_file_path;
    std::string source_compile_target;
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

    // Auxiliary functions
    [[nodiscard]] static std::string ConvertMacroDefinitionsToString(const MacroDefinitions& macro_definitions, std::string_view splitter = ", ") noexcept;

    // IShader interface
    [[nodiscard]] virtual Ptr<IShader>    GetPtr() = 0;
    [[nodiscard]] virtual Type            GetType() const noexcept = 0;
    [[nodiscard]] virtual const Settings& GetSettings() const noexcept = 0;

    virtual ~IShader() = default;
};

} // namespace Methane::Graphics::Rhi
