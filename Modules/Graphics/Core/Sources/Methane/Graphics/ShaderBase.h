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

namespace Methane::Graphics
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
            bool         is_addressable;
        };

        ResourceBindingBase(ContextBase& context, const Settings& settings);
        ResourceBindingBase(const ResourceBindingBase& other) = default;

        // ResourceBinding interface
        Shader::Type              GetShaderType() const override        { return m_settings.shader_type; }
        const std::string&        GetArgumentName() const override      { return m_settings.argument_name; }
        bool                      IsConstant() const override           { return m_settings.is_constant; }
        bool                      IsAddressable() const override        { return m_settings.is_addressable; }
        const Resource::Location& GetResourceLocation() const override  { return m_resource_location; }
        void                      SetResourceLocation(Resource::Location resource_location) override;

        // ResourceBindingBase interface
        virtual DescriptorHeap::Type GetDescriptorHeapType() const = 0;

        Ptr  GetPtr()               { return shared_from_this(); }
        bool HasResource() const    { return !!m_resource_location.sp_resource; }

    protected:
        ContextBase&        m_context;
        const Settings      m_settings;
        Resource::Location  m_resource_location;
    };

    ShaderBase(Type type, ContextBase& context, const Settings& settings);

    // Shader interface
    Type             GetType() const noexcept override       { return m_type; }
    const Settings&  GetSettings() const noexcept override   { return m_settings; }

    // ShaderBase interface
    virtual ResourceBindings GetResourceBindings(const std::set<std::string>& constant_argument_names,
                                                 const std::set<std::string>& addressable_argument_names) const = 0;

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

} // namespace Methane::Graphics
