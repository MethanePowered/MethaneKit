/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Program.h
Methane program interface: represents a collection of shaders set on graphics 
pipeline via state object and used to create resource binding objects.

******************************************************************************/

#pragma once

#include "Shader.h"
#include "Types.h"

#include <memory>
#include <vector>
#include <string>
#include <map>

//#define PROGRAM_IGNORE_MISSING_ARGUMENTS

namespace Methane::Graphics
{

struct Context;
struct CommandList;

struct Program
{
    using Ptr = std::shared_ptr<Program>;

    struct InputBufferLayout
    {
        enum class StepType
        {
            Undefined,
            PerVertex,
            PerInstance,
        };
        
        struct Argument
        {
            std::string name;
            std::string semantic;
        };
        
        using Arguments = std::vector<Argument>;
        
        Arguments arguments;
        StepType  step_type = StepType::PerVertex;
        uint32_t  step_rate = 1;
    };
    
    using InputBufferLayouts = std::vector<InputBufferLayout>;

    struct Argument
    {
        const Shader::Type shader_type;
        const std::string  argument_name;

        bool operator<(const Argument& other) const;
    };

    using Arguments                 = std::set<Argument>;
    using ResourceBindingByArgument = std::map<Argument, Shader::ResourceBinding::Ptr>;

    struct ResourceBindings
    {
        using Ptr     = std::shared_ptr<ResourceBindings>;
        using WeakPtr = std::weak_ptr<ResourceBindings>;
        using ResourceLocationByArgument = std::map<Argument, Resource::Location>;

        // Create ResourceBindings instance
        static Ptr Create(const Program::Ptr& sp_program, const ResourceLocationByArgument& resource_location_by_argument);
        static Ptr CreateCopy(const ResourceBindings& other_resource_bingings, const ResourceLocationByArgument& replace_resource_location_by_argument = {});

        // ResourceBindings interface
        virtual const Shader::ResourceBinding::Ptr& Get(const Argument& shader_argument) const = 0;
        virtual void Apply(CommandList& command_list) const = 0;

        virtual ~ResourceBindings() = default;
    };

    // Program settings
    struct Settings
    {
        Shaders                  shaders;
        InputBufferLayouts       input_buffer_layouts;
        std::set<std::string>    constant_argument_names;
        std::set<std::string>    addressable_argument_names;
        std::vector<PixelFormat> color_formats;
        PixelFormat              depth_format = PixelFormat::Unknown;
    };

    // Create Program instance
    static Ptr Create(Context& context, const Settings& settings);

    // Program interface
    virtual const Settings&      GetSettings() const = 0;
    virtual void                 SetName(const std::string& name) = 0;
    virtual const std::string&   GetName() const = 0;
    virtual const Shader::Types& GetShaderTypes() const = 0;
    virtual const Shader::Ptr&   GetShader(Shader::Type shader_type) const = 0;

    virtual ~Program() = default;
};

} // namespace Methane::Graphics
