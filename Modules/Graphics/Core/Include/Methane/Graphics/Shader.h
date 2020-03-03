/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Graphics/Shader.h
Methane shader interface: defines programmable stage of the graphics pipeline.

******************************************************************************/

#pragma once

#include <Methane/Memory.hpp>

#include <string>
#include <set>
#include <map>

namespace Methane::Data
{
struct Provider;
}

namespace Methane::Graphics
{

struct Context;

struct Shader
{
    enum class Type : uint32_t
    {
        Vertex = 0,
        Pixel,
        
        Count,
        All
    };
    
    using Types = std::set<Shader::Type>;

    using MacroDefinitions = std::map<std::string, std::string>;

    struct EntryFunction
    {
        std::string     file_name;
        std::string     function_name;
    };

    struct Settings
    {
        Data::Provider&  data_provider;
        EntryFunction    entry_function;
        MacroDefinitions compile_definitions;
        // Optional parameters (by default shaders are precompiled to application resources and loaded through Data::Provider)
        std::string      source_file_path;
        std::string      source_compile_target;
    };

    // Create Shader instance
    static Ptr<Shader> Create(Type type, Context& context, const Settings& settings);
    static Ptr<Shader> CreateVertex(Context& context, const Settings& settings) { return Create(Type::Vertex, context, settings); }
    static Ptr<Shader> CreatePixel(Context& context, const Settings& settings)  { return Create(Type::Pixel, context, settings); }

    // Auxiliary functions
    static std::string GetTypeName(Type shader_type) noexcept;

    // Shader interface
    virtual Type            GetType() const noexcept = 0;
    virtual const Settings& GetSettings() const noexcept = 0;

    virtual ~Shader() = default;
};

} // namespace Methane::Graphics
