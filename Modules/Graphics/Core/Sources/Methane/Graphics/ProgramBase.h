/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/ProgramBase.h
Base implementation of the program interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Program.h>

#include "ShaderBase.h"
#include "DescriptorHeap.h"

#include <memory>
#include <array>
#include <optional>
#include <mutex>

namespace Methane::Graphics
{

class ContextBase;
class CommandListBase;

class ProgramBase
    : public Program
    , public ObjectBase
    , public std::enable_shared_from_this<ProgramBase>
{
    friend class ShaderBase;
    friend class ProgramBindingsBase;

public:
    ProgramBase(ContextBase& context, const Settings& settings);
    ~ProgramBase() override;

    // Program interface
    const Settings&      GetSettings() const override                       { return m_settings; }
    const Shader::Types& GetShaderTypes() const override                    { return m_shader_types; }
    const Ptr<Shader>&   GetShader(Shader::Type shader_type) const override { return m_shaders_by_type[static_cast<size_t>(shader_type)]; }
    bool                 HasShader(Shader::Type shader_type) const          { return !!GetShader(shader_type); }

    ContextBase&         GetContext()       { return m_context; }
    const ContextBase&   GetContext() const { return m_context; }
    Ptr<ProgramBase>     GetPtr()           { return shared_from_this(); }

protected:
    void InitArgumentBindings(const ArgumentDescriptions& argument_descriptions);
    const ProgramBindings::ArgumentBindings& GetArgumentBindings() const { return m_binding_by_argument; }
    const DescriptorHeap::Range& ReserveConstantDescriptorRange(DescriptorHeap& heap, uint32_t range_length);

    Shader& GetShaderRef(Shader::Type shader_type);
    uint32_t GetInputBufferIndexByArgumentSemantic(const std::string& argument_semantic) const;

    using ShadersByType = std::array<Ptr<Shader>, static_cast<size_t>(Shader::Type::Count)>;
    static ShadersByType CreateShadersByType(const Ptrs<Shader>& shaders);

private:
    struct DescriptorHeapReservation
    {
        Ref<DescriptorHeap>   heap;
        DescriptorHeap::Range range;
    };

    using DescriptorRangeByHeapType = std::map<DescriptorHeap::Type, DescriptorHeapReservation>;

    ContextBase&                        m_context;
    const Settings                      m_settings;
    const ShadersByType                 m_shaders_by_type;
    const Shader::Types                 m_shader_types;
    ProgramBindings::ArgumentBindings   m_binding_by_argument;
    DescriptorRangeByHeapType           m_constant_descriptor_range_by_heap_type;
    std::mutex                          m_constant_descriptor_ranges_reservation_mutex;
};

} // namespace Methane::Graphics
