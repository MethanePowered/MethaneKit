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

FILE: Methane/Graphics/RHI/IProgram.h
Methane program interface: represents a collection of shaders set on graphics 
pipeline via state object and used to create resource binding objects.

******************************************************************************/

#pragma once

#include "IShader.h"
#include "IObject.h"
#include "ResourceView.h"

#include <Methane/Memory.hpp>
#include <Methane/Data/EnumMask.hpp>
#include <Methane/Graphics/Types.h>

#include <vector>
#include <string>
#include <string_view>
#include <unordered_set>
#include <unordered_map>

namespace Methane::Graphics::Rhi
{

struct ProgramInputBufferLayout
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

using ProgramInputBufferLayouts = std::vector<ProgramInputBufferLayout>;

struct IProgram;
class ProgramArgumentNotFoundException;

class ProgramArgument
{
public:
    using NotFoundException = ProgramArgumentNotFoundException;

    struct Hash
    {
        [[nodiscard]] size_t operator()(const ProgramArgument& arg) const { return arg.m_hash; }
    };

    ProgramArgument(ShaderType shader_type, std::string_view argument_name) noexcept;
    virtual ~ProgramArgument() = default;

    [[nodiscard]] ShaderType       GetShaderType() const noexcept { return m_shader_type; }
    [[nodiscard]] std::string_view GetName() const noexcept       { return m_name; }
    [[nodiscard]] size_t           GetHash() const noexcept       { return m_hash; }

    [[nodiscard]] bool operator==(const ProgramArgument& other) const noexcept;
    [[nodiscard]] bool operator<(const ProgramArgument& other) const noexcept;
    [[nodiscard]] virtual explicit operator std::string() const noexcept;

private:
    ShaderType       m_shader_type;
    std::string_view m_name;
    size_t           m_hash;
};

class ProgramArgumentNotFoundException : public std::invalid_argument
{
public:
    ProgramArgumentNotFoundException(const IProgram& program, const ProgramArgument& argument);

    [[nodiscard]] const IProgram&        GetProgram() const noexcept  { return m_program; }
    [[nodiscard]] const ProgramArgument& GetArgument() const noexcept { return m_argument; }

private:
    const IProgram& m_program;
    ProgramArgument m_argument;
};

enum class ProgramArgumentAccessType : uint32_t
{
    Constant,
    FrameConstant,
    Mutable
};

using ProgramArgumentAccessMask = Data::EnumMask<ProgramArgumentAccessType>;

using ProgramArguments = std::unordered_set<ProgramArgument, ProgramArgument::Hash>;

class ProgramArgumentAccessor : public ProgramArgument
{
public:
    using Type = ProgramArgumentAccessType;
    using Mask = ProgramArgumentAccessMask;

    ProgramArgumentAccessor(ShaderType shader_type, std::string_view argument_name, Type accessor_type = Type::Mutable, bool addressable = false) noexcept;
    ProgramArgumentAccessor(const ProgramArgument& argument, Type accessor_type = Type::Mutable, bool addressable = false) noexcept;

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

using ProgramArgumentAccessors = std::unordered_set<ProgramArgumentAccessor, ProgramArgumentAccessor::Hash>;
using ProgramShaders = Ptrs<IShader>;

struct ProgramSettings
{
    ProgramShaders            shaders;
    ProgramInputBufferLayouts input_buffer_layouts;
    ProgramArgumentAccessors  argument_accessors;
    AttachmentFormats         attachment_formats;
};

struct IContext;
struct IProgramBindings;

struct IProgram
    : virtual IObject // NOSONAR
{
    using Shaders                 = ProgramShaders;
    using Settings                = ProgramSettings;
    using InputBufferLayout       = ProgramInputBufferLayout;
    using InputBufferLayouts      = ProgramInputBufferLayouts;
    using Argument                = ProgramArgument;
    using Arguments               = ProgramArguments;
    using ArgumentAccessor        = ProgramArgumentAccessor;
    using ArgumentAccessors       = ProgramArgumentAccessors;
    using ResourceViewsByArgument = std::unordered_map<Argument, ResourceViews, Argument::Hash>;

    static ArgumentAccessors::const_iterator FindArgumentAccessor(const ArgumentAccessors& argument_accessors, const Argument& argument);

    // Create IProgram instance
    [[nodiscard]] static Ptr<IProgram> Create(const IContext& context, const Settings& settings);

    // IProgram interface
    [[nodiscard]] virtual Ptr<IProgramBindings> CreateBindings(const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index = 0U) = 0;
    [[nodiscard]] virtual const Settings&       GetSettings() const noexcept = 0;
    [[nodiscard]] virtual const ShaderTypes&    GetShaderTypes() const noexcept = 0;
    [[nodiscard]] virtual const Ptr<IShader>&   GetShader(ShaderType shader_type) const = 0;
    [[nodiscard]] virtual Data::Size            GetBindingsCount() const noexcept = 0;
};

} // namespace Methane::Graphics::Rhi
