/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/Shader.h
Vulkan implementation of the shader interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/Shader.h>
#include <Methane/Data/MutableChunk.hpp>
#include <Methane/Memory.hpp>
#include <Methane/Instrumentation.h>

#include <vulkan/vulkan.hpp>

#include <string>
#include <mutex>

namespace spirv_cross // NOSONAR
{
class Compiler;
}

namespace Methane::Data
{
class Chunk;
}

namespace Methane::Graphics::Vulkan
{

struct IContext;
class Program;

class Shader final
    : public Base::Shader
{
public:
    Shader(Type shader_type, const Base::Context& context, const Settings& settings);
    ~Shader() override;

    // Base::Shader interface
    Ptrs<Base::ProgramArgumentBinding> GetArgumentBindings(const Rhi::ProgramArgumentAccessors& argument_accessors) const override;

    const Data::Chunk&                     GetNativeByteCode() const noexcept { return m_byte_code_chunk.AsConstChunk(); }
    const vk::ShaderModule&                GetNativeModule() const;
    const spirv_cross::Compiler&           GetNativeCompiler() const;
    vk::PipelineShaderStageCreateInfo      GetNativeStageCreateInfo() const;
    vk::PipelineVertexInputStateCreateInfo GetNativeVertexInputStateCreateInfo(const Program& program);

    Data::MutableChunk& GetMutableByteCode() noexcept;

    static vk::ShaderStageFlagBits ConvertTypeToStageFlagBits(Type shader_type);

private:
    void InitializeVertexInputDescriptions(const Program& program);

    const IContext&                                  m_vk_context;
    Data::MutableChunk                               m_byte_code_chunk;
    mutable vk::UniqueShaderModule                   m_vk_unique_module;
    mutable UniquePtr<spirv_cross::Compiler>         m_spirv_compiler_ptr;
    std::vector<vk::VertexInputBindingDescription>   m_vertex_input_binding_descriptions;
    std::vector<vk::VertexInputAttributeDescription> m_vertex_input_attribute_descriptions;
    bool                                             m_vertex_input_initialized = false;
    TracyLockable(std::mutex,                        m_mutex);
};

} // namespace Methane::Graphics::Vulkan
