/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/ComputeState.cpp
Vulkan implementation of the compute state interface.

******************************************************************************/

#include <Methane/Graphics/Vulkan/ComputeState.h>
#include <Methane/Graphics/Vulkan/IContext.h>
#include <Methane/Graphics/Vulkan/RenderContext.h>
#include <Methane/Graphics/Vulkan/ComputeContext.h>
#include <Methane/Graphics/Vulkan/Device.h>
#include <Methane/Graphics/Vulkan/ComputeCommandList.h>
#include <Methane/Graphics/Vulkan/Program.h>
#include <Methane/Graphics/Vulkan/Shader.h>
#include <Methane/Graphics/Vulkan/Types.h>
#include <Methane/Graphics/Vulkan/Utils.hpp>

#include <Methane/Graphics/Base/RenderContext.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <algorithm>

namespace Methane::Graphics::Vulkan
{

static const Device& GetVulkanDeviceFromContext(const Rhi::IContext& context)
{
    META_FUNCTION_TASK();
    switch (context.GetType())
    {
    case Rhi::ContextType::Render:  return dynamic_cast<const RenderContext&>(context).GetVulkanDevice();
    case Rhi::ContextType::Compute: return dynamic_cast<const ComputeContext&>(context).GetVulkanDevice();
    }
    return dynamic_cast<const RenderContext&>(context).GetVulkanDevice();
}

ComputeState::ComputeState(const Rhi::IContext& context, const Settings& settings)
    : Base::ComputeState(context, settings)
    , m_device(GetVulkanDeviceFromContext(context))
    , m_vk_context(dynamic_cast<const IContext&>(context))
{
    META_FUNCTION_TASK();
    Reset(settings);
}

void ComputeState::Reset(const Settings& settings)
{
    META_FUNCTION_TASK();
    Base::ComputeState::Reset(settings);

    auto& program = static_cast<Program&>(*GetSettings().program_ptr);
    const std::vector<vk::PipelineShaderStageCreateInfo> vk_stages_info = program.GetNativeShaderStageCreateInfos();

    const vk::ComputePipelineCreateInfo vk_pipeline_create_info(
        vk::PipelineCreateFlags(),
        vk_stages_info.back(),
        program.AcquireNativePipelineLayout()
    );

    auto pipe = m_vk_context.GetVulkanDevice().GetNativeDevice().createComputePipelineUnique(nullptr, vk_pipeline_create_info);
    META_CHECK_ARG_EQUAL_DESCR(pipe.result, vk::Result::eSuccess, "Vulkan pipeline creation has failed");
    m_vk_unique_pipeline = std::move(pipe.value);
}

void ComputeState::Apply(Base::ComputeCommandList& compute_command_list)
{
    META_FUNCTION_TASK();
    const auto& vulkan_compute_command_list = static_cast<ComputeCommandList&>(compute_command_list);
    vulkan_compute_command_list.GetNativeCommandBufferDefault().bindPipeline(vk::PipelineBindPoint::eCompute, GetNativePipeline());
}

bool ComputeState::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (!Base::ComputeState::SetName(name))
        return false;

    SetVulkanObjectName(m_vk_context.GetVulkanDevice().GetNativeDevice(), m_vk_unique_pipeline.get(), name);
    return true;
}

} // namespace Methane::Graphics::Vulkan
