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

FILE: Methane/Graphics/Program.h
Methane program interface: represents a collection of shaders set on graphics 
pipeline via state object and used to create resource binding objects.

******************************************************************************/

#pragma once

#include "Shader.h"
#include "Object.h"
#include "Types.h"

#include <Methane/Memory.hpp>

#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>

//#define PROGRAM_IGNORE_MISSING_ARGUMENTS

namespace Methane::Graphics
{

struct Context;
struct CommandList;

struct Program : virtual Object
{
    struct InputBufferLayout
    {
        enum class StepType
        {
            Undefined,
            PerVertex,
            PerInstance,
        };

        using ArgumentSemantics = std::vector<std::string>;

        ArgumentSemantics argument_semantics;
        StepType          step_type = StepType::PerVertex;
        uint32_t          step_rate = 1;
    };
    
    using InputBufferLayouts = std::vector<InputBufferLayout>;

    struct Argument
    {
        struct Modifiers
        {
            using Mask = uint32_t;
            enum Value : Mask
            {
                None        = 0u,
                Constant    = 1u << 0u,
                Addressable = 1u << 1u,
                All         = ~0u,
            };

            Modifiers() = delete;
        };

        const Shader::Type shader_type;
        const std::string  name;
        const size_t       hash;

        Argument(Shader::Type shader_type, std::string argument_name);
        Argument(const Argument& argument) = default;

        bool operator==(const Argument& other) const;

        struct Hash
        {
            size_t operator()(const Argument& arg) const { return arg.hash; }
        };
    };

    using Arguments = std::unordered_set<Argument, Argument::Hash>;

    struct ArgumentDesc : Argument
    {
        const Modifiers::Mask modifiers;

        ArgumentDesc(Shader::Type shader_type, std::string argument_name,
                     Modifiers::Mask modifiers_mask = Modifiers::None);
        ArgumentDesc(const Argument& argument,
                     Modifiers::Mask modifiers_mask = Modifiers::None);
        ArgumentDesc(const ArgumentDesc& argument_desc) = default;

        inline bool IsConstant() const    { return modifiers & Modifiers::Constant; }
        inline bool IsAddressable() const { return modifiers & Modifiers::Addressable; }
    };

    using ArgumentDescriptions = std::unordered_set<ArgumentDesc, ArgumentDesc::Hash>;
    static ArgumentDescriptions::const_iterator FindArgumentDescription(const ArgumentDescriptions& argument_descriptions, const Argument& argument);
    using Shaders = Ptrs<Shader>;

    // Program settings
    struct Settings
    {
        Shaders              shaders;
        InputBufferLayouts   input_buffer_layouts;
        ArgumentDescriptions argument_descriptions;
        PixelFormats         color_formats;
        PixelFormat          depth_format = PixelFormat::Unknown;
    };

    // Create Program instance
    static Ptr<Program> Create(Context& context, const Settings& settings);

    // Program interface
    virtual const Settings&      GetSettings() const = 0;
    virtual const Shader::Types& GetShaderTypes() const = 0;
    virtual const Ptr<Shader>&   GetShader(Shader::Type shader_type) const = 0;

    virtual ~Program() = default;
};

} // namespace Methane::Graphics
