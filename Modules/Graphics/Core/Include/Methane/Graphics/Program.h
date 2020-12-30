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

FILE: Methane/Graphics/Program.h
Methane program interface: represents a collection of shaders set on graphics 
pipeline via state object and used to create resource binding objects.

******************************************************************************/

#pragma once

#include "Shader.h"
#include "Object.h"

#include <Methane/Memory.hpp>
#include <Methane/Graphics/Types.h>

#include <magic_enum.hpp>
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
        class NotFoundException : public std::invalid_argument
        {
        public:
            NotFoundException(const Program& program, const Argument& argument);

            [[nodiscard]] const Program&  GetProgram() const noexcept  { return m_program; }
            [[nodiscard]] const Argument& GetArgument() const noexcept { return *m_argument_ptr; }

        private:
            const Program& m_program;
            const UniquePtr<Argument> m_argument_ptr;
        };

        enum class Modifiers : uint32_t
        {
            None        = 0U,
            Constant    = 1U << 0U,
            Addressable = 1U << 1U,
            All         = ~0U
        };

        const Shader::Type shader_type;
        const std::string  name;
        const size_t       hash;

        Argument(Shader::Type shader_type, const std::string& argument_name) noexcept;
        Argument(const Argument& argument) = default;
        Argument(Argument&& argument) noexcept = default;

        [[nodiscard]] bool operator==(const Argument& other) const noexcept;
        [[nodiscard]] explicit operator std::string() const noexcept;

        struct Hash
        {
            [[nodiscard]] size_t operator()(const Argument& arg) const { return arg.hash; }
        };
    };

    using Arguments = std::unordered_set<Argument, Argument::Hash>;

    struct ArgumentDesc : Argument
    {
        const Modifiers modifiers;

        ArgumentDesc(Shader::Type shader_type, const std::string& argument_name,
                     Modifiers modifiers_mask = Modifiers::None) noexcept;
        ArgumentDesc(const Argument& argument,
                     Modifiers modifiers_mask = Modifiers::None) noexcept;
        ArgumentDesc(const ArgumentDesc& argument_desc) = default;
        ArgumentDesc(ArgumentDesc&& argument_desc) noexcept = default;

        [[nodiscard]] inline bool IsConstant() const    { using namespace magic_enum::bitwise_operators; return magic_enum::flags::enum_contains(modifiers & Modifiers::Constant); }
        [[nodiscard]] inline bool IsAddressable() const { using namespace magic_enum::bitwise_operators; return magic_enum::flags::enum_contains(modifiers & Modifiers::Addressable); }
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
    [[nodiscard]] static Ptr<Program> Create(Context& context, const Settings& settings);

    // Program interface
    [[nodiscard]] virtual const Settings&      GetSettings() const = 0;
    [[nodiscard]] virtual const Shader::Types& GetShaderTypes() const = 0;
    [[nodiscard]] virtual const Ptr<Shader>&   GetShader(Shader::Type shader_type) const = 0;
};

} // namespace Methane::Graphics
