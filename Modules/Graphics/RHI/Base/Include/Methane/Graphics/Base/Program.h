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

FILE: Methane/Graphics/Base/Program.h
Base implementation of the program interface.

******************************************************************************/

#pragma once

#include "Shader.h"
#include "ProgramBindings.h"
#include "RootConstantBuffer.h"

#include <Methane/Graphics/RHI/IProgram.h>

#include <memory>
#include <array>
#include <atomic>

namespace Methane::Graphics::Base
{

class Context;
class CommandList;

class Program
    : public Rhi::IProgram
    , public Object
{
    friend class Shader;
    friend class ProgramBindings;

public:
    Program(const Context& context, const Settings& settings);

    // IProgram interface
    const Settings&          GetSettings() const noexcept final           { return m_settings; }
    const Rhi::ShaderTypes&  GetShaderTypes() const noexcept final        { return m_shader_types; }
    const Ptr<Rhi::IShader>& GetShader(Rhi::ShaderType shader_type) const final;
    bool       HasShader(Rhi::ShaderType shader_type) const { return !!GetShader(shader_type); }
    Data::Size GetBindingsCount() const noexcept final      { return m_bindings_count; }

    // IObject overrides
    bool SetName(std::string_view name) override;

    const Context&       GetContext() const       { return m_context; }
    RootConstantStorage& GetRootConstantStorage() { return m_root_constant_storage; }
    RootConstantBuffer&  GetRootConstantBuffer()  { return m_root_constant_buffer; }
    RootConstantBuffer&  GetRootMutableBuffer()   { return m_root_mutable_buffer; }
    RootConstantBuffer&  GetRootFrameConstantBuffer(Data::Index frame_index);
    RootConstantBuffer&  GetRootConstantBuffer(Rhi::ProgramArgumentAccessType access_type, uint32_t frame_index = 0U);

protected:
    using ArgumentBinding       = ProgramBindings::ArgumentBinding;
    using ArgumentBindings      = ProgramBindings::ArgumentBindings;
    using FrameArgumentBindings = std::unordered_map<Rhi::ProgramArgument, Ptrs<ArgumentBinding>, Rhi::ProgramArgument::Hash>;

    const ArgumentBindings&      GetArgumentBindings() const noexcept      { return m_binding_by_argument; }
    const FrameArgumentBindings& GetFrameArgumentBindings() const noexcept { return m_frame_bindings_by_argument; }
    const Ptr<ArgumentBinding>&  GetFrameArgumentBinding(Data::Index frame_index, const Rhi::ProgramArgumentAccessor& argument_accessor) const;

    virtual void                 InitArgumentBindings();
    virtual Ptr<ArgumentBinding> CreateArgumentBindingInstance(const Ptr<ArgumentBinding>& argument_binding_ptr, Data::Index frame_index) const;

    Rhi::IShader& GetShaderRef(Rhi::ShaderType shader_type) const;
    uint32_t GetInputBufferIndexByArgumentSemantic(const std::string& argument_semantic) const;

    using ShadersByType = std::array<Ptr<Rhi::IShader>, magic_enum::enum_count<Rhi::ShaderType>() - 1>;
    static ShadersByType CreateShadersByType(const Ptrs<Rhi::IShader>& shaders);

    Data::Size GetBindingsCountAndIncrement() noexcept { return m_bindings_count++; }
    void       DecrementBindingsCount() noexcept       { m_bindings_count--; }

    template<typename ShaderFuncType>
    void ForEachShader(const ShaderFuncType& shader_functor)
    {
        for(const Ptr<Rhi::IShader>& shader_ptr : m_shaders_by_type)
            if (shader_ptr)
                shader_functor(static_cast<Base::Shader&>(*shader_ptr));
    }

private:
    using RootFrameConstantBuffers = std::vector<UniquePtr<RootConstantBuffer>>;

    void ExtractShaderTypesByArgumentName(Rhi::ShaderTypes& all_shader_types,
                std::map<std::string_view, Rhi::ShaderTypes, std::less<>>& shader_types_by_argument_name_map);
    void MergeAllShaderBindings(const Rhi::ShaderTypes& all_shader_types,
                const std::map<std::string_view, Rhi::ShaderTypes, std::less<>>& shader_types_by_argument_name_map);
    void InitFrameConstantArgumentBindings();

    const Context&           m_context;
    Settings                 m_settings;
    const ShadersByType      m_shaders_by_type;
    const Rhi::ShaderTypes   m_shader_types;
    RootConstantStorage      m_root_constant_storage;
    RootFrameConstantBuffers m_root_frame_constant_buffers;
    RootConstantBuffer       m_root_constant_buffer;
    RootConstantBuffer       m_root_mutable_buffer;
    ArgumentBindings         m_binding_by_argument;
    FrameArgumentBindings    m_frame_bindings_by_argument;
    std::atomic<Data::Size>  m_bindings_count{ 0u };
};

} // namespace Methane::Graphics::Base
