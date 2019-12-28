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

FILE: Methane/Graphics/ShaderBase.cpp
Base implementation of the shader interface.

******************************************************************************/

#include "ShaderBase.h"
#include "ProgramBase.h"

#include <Methane/Data/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics
{

std::string Shader::GetTypeName(Type shader_type) noexcept
{
    ITT_FUNCTION_TASK();
    switch (shader_type)
    {
    case Type::Vertex:    return "Vertex";
    case Type::Pixel:     return "Pixel";
    case Type::All:       return "All";
    default:              assert(0);
    }
    return "Unknown";
}

ShaderBase::ResourceBindingBase::ResourceBindingBase(ContextBase& context, const Settings& settings)
    : m_context(context)
    , m_settings(settings)
{
    ITT_FUNCTION_TASK();
}

void ShaderBase::ResourceBindingBase::SetResourceLocations(const Resource::Locations& resource_locations)
{
    ITT_FUNCTION_TASK();
    if (resource_locations.empty())
        throw std::invalid_argument("Can not set empty resources for resource binding.");

    const bool is_addressable_binding = IsAddressable();

    for (const Resource::Location& resource_location : resource_locations)
    {
        if (!resource_location.sp_resource)
            throw std::invalid_argument("Can not use empty resource for resource binding.");

        if (resource_location.sp_resource->GetResourceType() != m_settings.resource_type)
        {
            throw std::invalid_argument("Incompatible resource type \"" + Resource::GetTypeName(resource_location.sp_resource->GetResourceType()) +
                                        "\" is bound to argument \"" + GetArgumentName() +
                                        "\" of type \"" + Resource::GetTypeName(m_settings.resource_type) + "\".");
        }

        const Resource::Usage::Mask resource_usage_mask = resource_location.sp_resource->GetUsageMask();
        if (static_cast<bool>(resource_usage_mask & Resource::Usage::Addressable) != is_addressable_binding)
            throw std::invalid_argument("Resource addressable usage flag does not match with resource binding state.");

        if (!is_addressable_binding && resource_location.offset > 0)
            throw std::invalid_argument("Can not set resource location with non-zero offset to non-addressable resource binding.");
    }

    m_resource_locations = resource_locations;
}

DescriptorHeap::Type ShaderBase::ResourceBindingBase::GetDescriptorHeapType() const
{
    ITT_FUNCTION_TASK();
    return (m_settings.resource_type == Resource::Type::Sampler)
          ? DescriptorHeap::Type::Samplers
          : DescriptorHeap::Type::ShaderResources;
}
    
bool ShaderBase::ResourceBindingBase::IsAlreadyApplied(const Program& program, const Program::Argument& program_argument,
                                                       const CommandListBase::CommandState& command_state,
                                                       bool check_binding_value_changes) const
{
    ITT_FUNCTION_TASK();
    if (!command_state.sp_resource_bindings)
        return false;
    
    const ProgramBase::ResourceBindingsBase& previous_resource_bindings = static_cast<const ProgramBase::ResourceBindingsBase&>(*command_state.sp_resource_bindings);
    
    if (std::addressof(previous_resource_bindings.GetProgram()) != std::addressof(program))
        return false;
    
    // 1) No need in setting constant resource binding
    //    when another binding was previously set in the same command list for the same program
    if (m_settings.is_constant)
        return true;

    if (!check_binding_value_changes)
        return false;

    const Shader::ResourceBinding::Ptr& previous_argument_resource_binding = command_state.sp_resource_bindings->Get(program_argument);
    if (!previous_argument_resource_binding)
        return false;
    
    // 2) No need in setting resource binding to the same location
    //    as a previous resource binding set in the same command list for the same program
    if (previous_argument_resource_binding->GetResourceLocations() == m_resource_locations)
        return true;
    
    return false;
}

ShaderBase::ShaderBase(Type type, ContextBase& context, const Settings& settings)
    : m_type(type)
    , m_context(context)
    , m_settings(settings)
{
    ITT_FUNCTION_TASK();
}

uint32_t ShaderBase::GetProgramInputBufferIndexByArgumentName(const ProgramBase& program, const std::string& argument_name) const
{
    ITT_FUNCTION_TASK();
    return program.GetInputBufferIndexByArgumentName(argument_name);
}

uint32_t ShaderBase::GetProgramInputBufferIndexByArgumentSemantic(const ProgramBase& program, const std::string& argument_semantic) const
{
    ITT_FUNCTION_TASK();
    return program.GetInputBufferIndexByArgumentSemantic(argument_semantic);
}

std::string ShaderBase::GetCompiledEntryFunctionName() const
{
    ITT_FUNCTION_TASK();
    std::stringstream entry_func_steam;
    entry_func_steam << m_settings.entry_function.file_name << "_" << m_settings.entry_function.function_name;
    for (const auto& define_and_value : m_settings.compile_definitions)
    {
        entry_func_steam << "_" << define_and_value.first << define_and_value.second;
    }
    return entry_func_steam.str();
}

} // namespace Methane::Graphics
