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

FILE: Methane/Graphics/ShaderBase.h
Base implementation of the shader interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Shader.h>

#include "DescriptorHeap.h"

namespace Methane
{
namespace Graphics
{

class ContextBase;
class ProgramBase;

class ShaderBase
    : public Shader
    , public std::enable_shared_from_this<ShaderBase>
{
public:
    class ResourceBindingBase
        : public ResourceBinding
        , public std::enable_shared_from_this<ResourceBindingBase>
    {
    public:
        struct Settings
        {
            Shader::Type shader_type;
            std::string  argument_name;
            bool         is_constant;
        };

        ResourceBindingBase(ContextBase& context, const Settings& settings);
        ResourceBindingBase(const ResourceBindingBase& other) = default;
        virtual ~ResourceBindingBase() override = default;

        // ResourceBinding interface
        virtual Shader::Type         GetShaderType() const override     { return m_settings.shader_type; }
        virtual const std::string&   GetArgumentName() const override   { return m_settings.argument_name; }
        virtual bool                 IsConstant() const override        { return m_settings.is_constant; }
        virtual const Resource::Ptr& GetResource() const override       { return m_sp_resource; }
        virtual void                 SetResource(const Resource::Ptr& sp_resource) override;

        // ResourceBindingBase interface
        virtual DescriptorHeap::Type GetDescriptorHeapType() const = 0;

        Ptr  GetPtr()               { return shared_from_this(); }
        bool HasResource() const    { return !!m_sp_resource; }

    protected:
        ContextBase&    m_context;
        const Settings  m_settings;
        Resource::Ptr   m_sp_resource;
    };

    ShaderBase(Type type, ContextBase& context, const Settings& settings);
    virtual ~ShaderBase() override = default;

    // Shader interface
    virtual Type             GetType() const noexcept override       { return m_type; }
    virtual const Settings&  GetSettings() const noexcept override   { return m_settings; }

    // ShaderBase interface
    virtual ResourceBindings GetResourceBindings(const std::set<std::string>& constant_argument_names) const = 0;

    Ptr         GetPtr()                     { return shared_from_this(); }
    std::string GetTypeName() const noexcept { return Shader::GetTypeName(m_type); }

protected:
    uint32_t    GetProgramInputBufferIndexByArgumentName(const ProgramBase& program, const std::string& argument_name) const;
    uint32_t    GetProgramInputBufferIndexByArgumentSemantic(const ProgramBase& program, const std::string& argument_semantic) const;
    std::string GetCompiledEntryFunctionName() const;

    const Type        m_type;
    ContextBase&      m_context;
    const Settings    m_settings;
};

using Shaders = std::vector<Shader::Ptr>;

} // namespace Graphics
} // namespace Methane
