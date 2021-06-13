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

FILE: Methane/Graphics/Vulkan/ShaderVK.h
Vulkan implementation of the shader interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ShaderBase.h>
#include <Methane/Memory.hpp>

#include <vulkan/vulkan.hpp>

#include <string>

namespace spirv_cross
{
class Compiler;
}

namespace Methane::Data
{
class Chunk;
}

namespace Methane::Graphics
{

struct IContextVK;
class ProgramVK;

class ShaderVK final : public ShaderBase
{
public:
    ShaderVK(Shader::Type shader_type, const ContextBase& context, const Settings& settings);
    ~ShaderVK() override;

    // ShaderBase interface
    ArgumentBindings GetArgumentBindings(const Program::ArgumentAccessors& argument_accessors) const override;

    const Data::Chunk&                     GetNativeByteCode() const noexcept { return *m_byte_code_chunk_ptr; }
    const vk::ShaderModule&                GetNativeModule() const noexcept { return m_vk_module; }
    const spirv_cross::Compiler&           GetNativeCompiler() const;
    vk::PipelineShaderStageCreateInfo      GetNativeStageCreateInfo() const;
    vk::PipelineVertexInputStateCreateInfo GetNativeVertexInputStateCreateInfo(const ProgramVK& program);

private:
    void InitializeVertexInputDescriptions(const ProgramVK& program);

    const IContextVK& GetContextVK() const noexcept;

    UniquePtr<Data::Chunk>                           m_byte_code_chunk_ptr;
    vk::ShaderModule                                 m_vk_module;
    mutable UniquePtr<spirv_cross::Compiler>         m_spirv_compiler_ptr;
    std::vector<vk::VertexInputBindingDescription>   m_vertex_input_binding_descriptions;
    std::vector<vk::VertexInputAttributeDescription> m_vertex_input_attribute_descriptions;
    bool                                             m_vertex_input_initialized = false;
};

} // namespace Methane::Graphics
