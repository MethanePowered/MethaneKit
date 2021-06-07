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

FILE: Methane/Graphics/Vulkan/ShaderVK.mm
Vulkan implementation of the shader interface.

******************************************************************************/

#include "ShaderVK.h"
#include "ContextVK.h"
#include "DeviceVK.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

static vk::ShaderStageFlagBits ConvertShaderTypeToStageFlagBits(Shader::Type shader_type)
{
    META_FUNCTION_TASK();
    switch(shader_type)
    {
    case Shader::Type::All:    return vk::ShaderStageFlagBits::eAll;
    case Shader::Type::Vertex: return vk::ShaderStageFlagBits::eVertex;
    case Shader::Type::Pixel:  return vk::ShaderStageFlagBits::eFragment;
    default:                   META_UNEXPECTED_ARG_RETURN(shader_type, vk::ShaderStageFlagBits::eAll);
    }
}

static vk::ShaderModule CreateShaderModuleFromSpirvFile(const vk::Device& vk_device, const Data::Chunk& spirv_data_chunk)
{
    META_FUNCTION_TASK();
    const vk::ShaderModuleCreateInfo module_create_info(
        vk::ShaderModuleCreateFlags{},
        spirv_data_chunk.GetDataSize(),
        reinterpret_cast<const uint32_t*>(spirv_data_chunk.GetDataPtr())
    );
    return vk_device.createShaderModule(module_create_info);
}

Ptr<Shader> Shader::Create(Shader::Type shader_type, const Context& context, const Settings& settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<ShaderVK>(shader_type, dynamic_cast<const ContextBase&>(context), settings);
}

ShaderVK::ShaderVK(Shader::Type shader_type, const ContextBase& context, const Settings& settings)
    : ShaderBase(shader_type, context, settings)
    , m_vk_module(CreateShaderModuleFromSpirvFile(GetContextVK().GetDeviceVK().GetNativeDevice(),
                                                  settings.data_provider.GetData(fmt::format("{}.spirv", GetCompiledEntryFunctionName()))))
{
    META_FUNCTION_TASK();
}

ShaderVK::~ShaderVK()
{
    META_FUNCTION_TASK();
    GetContextVK().GetDeviceVK().GetNativeDevice().destroyShaderModule(m_vk_module);
}

ShaderBase::ArgumentBindings ShaderVK::GetArgumentBindings(const Program::ArgumentAccessors&) const
{
    META_FUNCTION_TASK();
    ArgumentBindings argument_bindings;
    return argument_bindings;
}

vk::PipelineShaderStageCreateInfo ShaderVK::GetNativeStageCreateInfo() const
{
    META_FUNCTION_TASK();
    return vk::PipelineShaderStageCreateInfo(
        vk::PipelineShaderStageCreateFlags{},
        ConvertShaderTypeToStageFlagBits(GetType()),
        m_vk_module,
        GetSettings().entry_function.function_name.c_str()
    );
}

const IContextVK& ShaderVK::GetContextVK() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<const IContextVK&>(GetContext());
}

} // namespace Methane::Graphics
