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
#include "ProgramArgument.h"

#include <Methane/Memory.hpp>

#include <vector>
#include <string_view>
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
    using Shaders                = ProgramShaders;
    using Settings               = ProgramSettings;
    using InputBufferLayout      = ProgramInputBufferLayout;
    using InputBufferLayouts     = ProgramInputBufferLayouts;
    using Argument               = ProgramArgument;
    using Arguments              = ProgramArguments;
    using ArgumentAccessor       = ProgramArgumentAccessor;
    using ArgumentAccessors      = ProgramArgumentAccessors;
    using ArgumentBindingValue   = ProgramArgumentBindingValue;
    using BindingValueByArgument = ProgramBindingValueByArgument;

    static const ArgumentAccessor* FindArgumentAccessor(const ArgumentAccessors& argument_accessors,
                                                        const Argument& argument);

    // Create IProgram instance
    [[nodiscard]] static Ptr<IProgram> Create(IContext& context, const Settings& settings);

    // IProgram interface
    [[nodiscard]] virtual Ptr<IProgramBindings> CreateBindings(const BindingValueByArgument& binding_value_by_argument,
                                                               Data::Index frame_index = 0U) = 0;
    [[nodiscard]] virtual const Settings&       GetSettings() const noexcept = 0;
    [[nodiscard]] virtual const ShaderTypes&    GetShaderTypes() const noexcept = 0;
    [[nodiscard]] virtual const Ptr<IShader>&   GetShader(ShaderType shader_type) const = 0;
    [[nodiscard]] virtual Data::Size            GetBindingsCount() const noexcept = 0;
};

} // namespace Methane::Graphics::Rhi
