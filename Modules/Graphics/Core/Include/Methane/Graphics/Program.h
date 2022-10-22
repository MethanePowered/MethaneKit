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

#include "IShader.h"
#include "IObject.h"

#include <Methane/Memory.hpp>
#include <Methane/Graphics/Types.h>

#include <vector>
#include <string>
#include <string_view>
#include <unordered_set>

namespace Methane::Graphics
{

struct IContext;
struct CommandList;

struct Program : virtual IObject // NOSONAR
{
    struct InputBufferLayout
    {
        enum class StepType
        {
            Undefined,
            PerVertex,
            PerInstance,
        };

        using ArgumentSemantics = std::vector<std::string_view>;

        ArgumentSemantics argument_semantics;
        StepType          step_type = StepType::PerVertex;
        uint32_t          step_rate = 1U;
    };
    
    using InputBufferLayouts = std::vector<InputBufferLayout>;

    class Argument
    {
    public:
        class NotFoundException : public std::invalid_argument
        {
        public:
            NotFoundException(const Program& program, const Argument& argument);

            [[nodiscard]] const Program&  GetProgram() const noexcept  { return m_program; }
            [[nodiscard]] const Argument& GetArgument() const noexcept { return *m_argument_ptr; }

        private:
            const Program& m_program;
            UniquePtr<Argument> m_argument_ptr;
        };

        struct Hash
        {
            [[nodiscard]] size_t operator()(const Argument& arg) const { return arg.m_hash; }
        };

        Argument(ShaderType shader_type, std::string_view argument_name) noexcept;
        virtual ~Argument() = default;

        [[nodiscard]] ShaderType       GetShaderType() const noexcept { return m_shader_type; }
        [[nodiscard]] std::string_view GetName() const noexcept       { return m_name; }
        [[nodiscard]] size_t           GetHash() const noexcept       { return m_hash; }

        [[nodiscard]] bool operator==(const Argument& other) const noexcept;
        [[nodiscard]] virtual explicit operator std::string() const noexcept;

    private:
        ShaderType       m_shader_type;
        std::string_view m_name;
        size_t           m_hash;
    };

    using Arguments = std::unordered_set<Argument, Argument::Hash>;

    class ArgumentAccessor : public Argument
    {
    public:
        enum class Type : uint32_t
        {
            Constant      = 1U << 0U,
            FrameConstant = 1U << 1U,
            Mutable       = 1U << 2U,
        };

        ArgumentAccessor(ShaderType shader_type, std::string_view argument_name, Type accessor_type = Type::Mutable, bool addressable = false) noexcept;
        ArgumentAccessor(const Argument& argument, Type accessor_type = Type::Mutable, bool addressable = false) noexcept;

        [[nodiscard]] size_t GetAccessorIndex() const noexcept;
        [[nodiscard]] Type   GetAccessorType() const noexcept  { return m_accessor_type; }
        [[nodiscard]] bool   IsAddressable() const noexcept    { return m_addressable; }
        [[nodiscard]] bool   IsConstant() const noexcept       { return m_accessor_type == Type::Constant; }
        [[nodiscard]] bool   IsFrameConstant() const noexcept  { return m_accessor_type == Type::FrameConstant; }
        [[nodiscard]] explicit operator std::string() const noexcept final;

    private:
        Type m_accessor_type = Type::Mutable;
        bool m_addressable   = false;
    };

    using ArgumentAccessors = std::unordered_set<ArgumentAccessor, ArgumentAccessor::Hash>;
    static ArgumentAccessors::const_iterator FindArgumentAccessor(const ArgumentAccessors& argument_accessors, const Argument& argument);
    using Shaders = Ptrs<IShader>;

    // Program settings
    struct Settings
    {
        Shaders            shaders;
        InputBufferLayouts input_buffer_layouts;
        ArgumentAccessors  argument_accessors;
        AttachmentFormats  attachment_formats;
    };

    // Create Program instance
    [[nodiscard]] static Ptr<Program> Create(const IContext& context, const Settings& settings);

    // Program interface
    [[nodiscard]] virtual const Settings&      GetSettings() const noexcept = 0;
    [[nodiscard]] virtual const ShaderTypes&   GetShaderTypes() const noexcept = 0;
    [[nodiscard]] virtual const Ptr<IShader>&  GetShader(ShaderType shader_type) const = 0;
    [[nodiscard]] virtual Data::Size           GetBindingsCount() const noexcept = 0;
};

} // namespace Methane::Graphics
