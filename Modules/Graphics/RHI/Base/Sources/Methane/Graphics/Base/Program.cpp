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

FILE: Methane/Graphics/Base/Program.cpp
Base implementation of the program interface.

******************************************************************************/

#include <Methane/Graphics/Base/Program.h>
#include <Methane/Graphics/Base/RenderContext.h>

#include <Methane/Instrumentation.h>
#include <Methane/Platform/Utils.h>

#include <magic_enum.hpp>
#include <algorithm>

namespace Methane::Graphics::Base
{

Program::ShadersByType Program::CreateShadersByType(const Ptrs<Rhi::IShader>& shaders)
{
    META_FUNCTION_TASK();
    Program::ShadersByType shaders_by_type;
    for (const Ptr<Rhi::IShader>& shader_ptr : shaders)
    {
        META_CHECK_ARG_NOT_NULL_DESCR(shader_ptr, "can not use empty shader pointer for program creation");
        shaders_by_type[magic_enum::enum_index(shader_ptr->GetType()).value()] = shader_ptr;
    }
    return shaders_by_type;
}

static Rhi::ShaderTypes CreateShaderTypes(const Ptrs<Rhi::IShader>& shaders)
{
    META_FUNCTION_TASK();
    Rhi::ShaderTypes shader_types;
    for (const Ptr<Rhi::IShader>& shader_ptr : shaders)
    {
        META_CHECK_ARG_NOT_NULL_DESCR(shader_ptr, "can not use empty shader pointer for program creation");
        shader_types.insert(shader_ptr->GetType());
    }
    return shader_types;
}

Program::Program(const Context& context, const Settings& settings)
    : m_context(context)
    , m_settings(settings)
    , m_shaders_by_type(CreateShadersByType(settings.shaders))
    , m_shader_types(CreateShaderTypes(settings.shaders))
{
    META_FUNCTION_TASK();
}

const Ptr<Rhi::IShader>& Program::GetShader(Rhi::ShaderType shader_type) const
{
    return m_shaders_by_type[magic_enum::enum_index(shader_type).value()];
}

void Program::InitArgumentBindings(const ArgumentAccessors& argument_accessors)
{
    META_FUNCTION_TASK();
    Rhi::ShaderTypes all_shader_types;
    std::map<std::string_view, Rhi::ShaderTypes, std::less<>> shader_types_by_argument_name_map;
    
    m_binding_by_argument.clear();
    for (const Ptr<Rhi::IShader>& shader_ptr : m_settings.shaders)
    {
        META_CHECK_ARG_NOT_NULL_DESCR(shader_ptr, "empty shader pointer in program is not allowed");
        const Rhi::ShaderType shader_type = shader_ptr->GetType();
        all_shader_types.insert(shader_type);
        
        const Ptrs<ProgramArgumentBinding> argument_bindings = static_cast<const Shader&>(*shader_ptr).GetArgumentBindings(argument_accessors);
        for (const Ptr<ProgramBindings::ArgumentBinding>& argument_binging_ptr : argument_bindings)
        {
            META_CHECK_ARG_NOT_NULL_DESCR(argument_binging_ptr, "empty resource binding provided by shader");
            const Argument& shader_argument = argument_binging_ptr->GetSettings().argument;
            if (const auto [it, added] = m_binding_by_argument.try_emplace(shader_argument, argument_binging_ptr);
                !added)
            {
                it->second->MergeSettings(*argument_binging_ptr);
            }
            shader_types_by_argument_name_map[shader_argument.GetName()].insert(shader_argument.GetShaderType());
        }
    }
    
    // Replace bindings for argument set for all shader types in program to one binding set for argument with ShaderType::All
    for (const auto& [argument_name, shader_types] : shader_types_by_argument_name_map)
    {
        if (shader_types != all_shader_types)
            continue;

        Ptr<ProgramBindings::ArgumentBinding> argument_binding_ptr;
        for(Rhi::ShaderType shader_type : all_shader_types)
        {
            const Argument argument{ shader_type, argument_name };
            auto binding_by_argument_it = m_binding_by_argument.find(argument);
            META_CHECK_ARG_DESCR(argument, binding_by_argument_it != m_binding_by_argument.end(), "Resource binding was not initialized for for argument");
            if (argument_binding_ptr)
            {
                argument_binding_ptr->MergeSettings(*binding_by_argument_it->second);
            }
            else
            {
                argument_binding_ptr = binding_by_argument_it->second;
            }
            m_binding_by_argument.erase(binding_by_argument_it);
        }

        META_CHECK_ARG_NOT_NULL_DESCR(argument_binding_ptr, "failed to create resource binding for argument '{}'", argument_name);
        m_binding_by_argument.try_emplace(Argument{ Rhi::ShaderType::All, argument_name }, argument_binding_ptr);
    }

    if (m_context.GetType() != Rhi::IContext::Type::Render)
        return;

    // Create frame-constant argument bindings only when program is created in render context
    m_frame_bindings_by_argument.clear();
    const auto& render_context = static_cast<const RenderContext&>(m_context);
    const uint32_t frame_buffers_count = render_context.GetSettings().frame_buffers_count;
    META_CHECK_ARG_GREATER_OR_EQUAL(frame_buffers_count, 2);

    for (const auto& [program_argument, argument_binding_ptr] : m_binding_by_argument)
    {
        if (!argument_binding_ptr->GetSettings().argument.IsFrameConstant())
            continue;

        Ptrs<ProgramBindings::ArgumentBinding> per_frame_argument_bindings(frame_buffers_count);
        per_frame_argument_bindings[0] = argument_binding_ptr;
        for(uint32_t frame_index = 1; frame_index < frame_buffers_count; ++frame_index)
        {
            per_frame_argument_bindings[frame_index] = ProgramBindings::ArgumentBinding::CreateCopy(*argument_binding_ptr);
        }
        m_frame_bindings_by_argument.try_emplace(program_argument, std::move(per_frame_argument_bindings));
    }
}

const Ptr<ProgramBindings::ArgumentBinding>& Program::GetFrameArgumentBinding(Data::Index frame_index, const Rhi::ProgramArgumentAccessor& argument_accessor) const
{
    META_FUNCTION_TASK();
    const auto argument_frame_bindings_it = m_frame_bindings_by_argument.find(argument_accessor);
    META_CHECK_ARG_TRUE_DESCR(argument_frame_bindings_it != m_frame_bindings_by_argument.end(), "can not find frame-constant argument binding in program");
    return argument_frame_bindings_it->second.at(frame_index);
}

Ptr<ProgramBindings::ArgumentBinding> Program::CreateArgumentBindingInstance(const Ptr<ProgramBindings::ArgumentBinding>& argument_binding_ptr, Data::Index frame_index) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(argument_binding_ptr);

    const Rhi::ProgramArgumentAccessor& argument_accessor = argument_binding_ptr->GetSettings().argument;
    switch(argument_accessor.GetAccessorType())
    {
    case ArgumentAccessor::Type::Mutable:       return ProgramBindings::ArgumentBinding::CreateCopy(*argument_binding_ptr);
    case ArgumentAccessor::Type::Constant:      return argument_binding_ptr;
    case ArgumentAccessor::Type::FrameConstant: return GetFrameArgumentBinding(frame_index, argument_accessor);
    default:                                    META_UNEXPECTED_ARG_RETURN(argument_accessor.GetAccessorType(), nullptr);
    }
}

Rhi::IShader& Program::GetShaderRef(Rhi::ShaderType shader_type) const
{
    META_FUNCTION_TASK();
    const Ptr<Rhi::IShader>& shader_ptr = GetShader(shader_type);
    META_CHECK_ARG_DESCR(shader_type, shader_ptr, "{} shader was not found in program '{}'", magic_enum::enum_name(shader_type), GetName());
    return *shader_ptr;
}

uint32_t Program::GetInputBufferIndexByArgumentSemantic(const std::string& argument_semantic) const
{
    META_FUNCTION_TASK();
    for (size_t buffer_index = 0; buffer_index < m_settings.input_buffer_layouts.size(); buffer_index++)
    {
        const InputBufferLayout& input_buffer_layout = m_settings.input_buffer_layouts[buffer_index];
        if (auto argument_it = std::find(input_buffer_layout.argument_semantics.begin(), input_buffer_layout.argument_semantics.end(), argument_semantic);
            argument_it != input_buffer_layout.argument_semantics.end())
            return static_cast<uint32_t>(buffer_index);
    }
    META_INVALID_ARG_DESCR(argument_semantic, "input argument with semantic name was not found for program '{}'", GetName());
#ifndef METHANE_CHECKS_ENABLED
    return 0;
#endif
}

} // namespace Methane::Graphics::Base
