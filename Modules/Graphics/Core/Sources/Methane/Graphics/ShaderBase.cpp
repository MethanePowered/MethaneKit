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
#include "ContextBase.h"
#include "Instrumentation.h"

#include <cassert>

using namespace Methane::Graphics;

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
    m_context.AddCallback(*this);
}

ShaderBase::ResourceBindingBase::ResourceBindingBase(const ResourceBindingBase& other)
    : m_context(other.m_context)
    , m_settings(other.m_settings)
    , m_sp_resource(other.m_sp_resource)
{
    ITT_FUNCTION_TASK();
    m_context.AddCallback(*this);
}

ShaderBase::ResourceBindingBase::~ResourceBindingBase()
{
    ITT_FUNCTION_TASK();
    m_context.RemoveCallback(*this);
}

void ShaderBase::ResourceBindingBase::SetResource(const Resource::Ptr& sp_resource)
{
    ITT_FUNCTION_TASK();
    m_sp_resource = sp_resource;
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
    entry_func_steam << m_settings.entry_target.function_name;
    for (const auto& define_and_value : m_settings.compile_definitions)
    {
        entry_func_steam << "_" << define_and_value.first << define_and_value.second;
    }
    return entry_func_steam.str();
}