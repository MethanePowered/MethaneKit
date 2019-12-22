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

FILE: Methane/Graphics/Vulkan/ProgramVK.h
Vulkan implementation of the program interface.

******************************************************************************/

#include "ProgramVK.h"
#include "ShaderVK.h"
#include "BufferVK.h"
#include "ContextVK.h"
#include "RenderCommandListVK.h"

#include <Methane/Data/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics
{

Program::ResourceBindings::Ptr Program::ResourceBindings::Create(const Program::Ptr& sp_program, const ResourceLocationByArgument& resource_location_by_argument)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ProgramVK::ResourceBindingsVK>(sp_program, resource_location_by_argument);
}

Program::ResourceBindings::Ptr Program::ResourceBindings::CreateCopy(const ResourceBindings& other_resource_bingings, const ResourceLocationByArgument& replace_resource_location_by_argument)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ProgramVK::ResourceBindingsVK>(static_cast<const ProgramVK::ResourceBindingsVK&>(other_resource_bingings), replace_resource_location_by_argument);
}

ProgramVK::ResourceBindingsVK::ResourceBindingsVK(const Program::Ptr& sp_program, const ResourceLocationByArgument& resource_location_by_argument)
    : ResourceBindingsBase(sp_program, resource_location_by_argument)
{
    ITT_FUNCTION_TASK();
}

ProgramVK::ResourceBindingsVK::ResourceBindingsVK(const ResourceBindingsVK& other_resource_bindings, const ResourceLocationByArgument& replace_resource_location_by_argument)
    : ResourceBindingsBase(other_resource_bindings, replace_resource_location_by_argument)
{
    ITT_FUNCTION_TASK();
}

void ProgramVK::ResourceBindingsVK::Apply(CommandList& command_list, ApplyBehavior::Mask apply_behavior) const
{
    ITT_FUNCTION_TASK();

    RenderCommandListVK& vulkan_command_list = dynamic_cast<RenderCommandListVK&>(command_list);
    const CommandListBase::CommandState& command_state = vulkan_command_list.GetCommandState();

    for(const auto& resource_binding_by_argument : m_resource_binding_by_argument)
    {
        const Argument& program_argument = resource_binding_by_argument.first;
        const ShaderVK::ResourceBindingVK& vulkan_resource_binding = static_cast<const ShaderVK::ResourceBindingVK&>(*resource_binding_by_argument.second);
        const Resource::Location& bound_resource_location = vulkan_resource_binding.GetResourceLocation();
        if (!bound_resource_location.sp_resource)
        {
#ifndef PROGRAM_IGNORE_MISSING_ARGUMENTS
            throw std::runtime_error(
                "Can not apply resource binding for argument \"" + program_argument.argument_name +
                "\" of \"" + Shader::GetTypeName(program_argument.shader_type) +
                "\" shader because it is not bound to any resource.");
#else
            continue;
#endif
        }
        
        if ((apply_behavior & ApplyBehavior::ConstantOnce || apply_behavior & ApplyBehavior::ChangesOnly) &&
            vulkan_resource_binding.IsAlreadyApplied(*m_sp_program, program_argument, command_state, apply_behavior & ApplyBehavior::ChangesOnly))
            continue;
    }
}

Program::Ptr Program::Create(Context& context, const Settings& settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ProgramVK>(static_cast<ContextBase&>(context), settings);
}

ProgramVK::ProgramVK(ContextBase& context, const Settings& settings)
    : ProgramBase(context, settings)
{
    ITT_FUNCTION_TASK();

    // In case if RT pixel formats are not set, we assume it renders to frame buffer
    // NOTE: even when program has no pixel shaders render, render state must have at least one color format to be valid
    std::vector<PixelFormat> color_formats = settings.color_formats;
    if (color_formats.empty())
    {
        color_formats.push_back(context.GetSettings().color_format);
    }
}

ProgramVK::~ProgramVK()
{
    ITT_FUNCTION_TASK();
}

ContextVK& ProgramVK::GetContextVK() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<class ContextVK&>(m_context);
}

ShaderVK& ProgramVK::GetShaderVK(Shader::Type shader_type) noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<ShaderVK&>(GetShaderRef(shader_type));
}

} // namespace Methane::Graphics
