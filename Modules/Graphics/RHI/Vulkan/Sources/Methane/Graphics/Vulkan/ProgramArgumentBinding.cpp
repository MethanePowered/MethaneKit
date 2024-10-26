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

FILE: Methane/Graphics/Vulkan/ProgramArgumentBinding.h
Vulkan implementation of the program argument binding interface.

******************************************************************************/

#include <Methane/Graphics/Vulkan/ProgramArgumentBinding.h>
#include <Methane/Graphics/Vulkan/IContext.h>
#include <Methane/Graphics/Vulkan/Device.h>

#include <Methane/Graphics/Base/Context.h>

namespace Methane::Graphics::Vulkan
{

static Rhi::ResourceUsageMask GetResourceUsageByDescriptorType(vk::DescriptorType descriptor_type)
{
    META_FUNCTION_TASK();
    Rhi::ResourceUsageMask resource_usage{ Rhi::ResourceUsage::ShaderRead };
    if (descriptor_type == vk::DescriptorType::eStorageImage         ||
        descriptor_type == vk::DescriptorType::eStorageTexelBuffer   ||
        descriptor_type == vk::DescriptorType::eStorageBuffer        ||
        descriptor_type == vk::DescriptorType::eStorageBufferDynamic)
    {
        resource_usage.SetBitOn(Rhi::ResourceUsage::ShaderWrite);
    }
    return resource_usage;
}

ProgramArgumentBinding::ProgramArgumentBinding(const Base::Context& context, const Settings& settings)
    : Base::ProgramArgumentBinding(context, settings)
    , m_settings_vk(settings)
{ }

vk::ShaderStageFlagBits ProgramArgumentBinding::GetNativeShaderStageFlags() const
{
    META_FUNCTION_TASK();
    return Shader::ConvertTypeToStageFlagBits(m_settings_vk.argument.GetShaderType());
}

void ProgramArgumentBinding::SetDescriptorSetBinding(const vk::DescriptorSet& descriptor_set, uint32_t binding_value) noexcept
{
    META_FUNCTION_TASK();
    m_vk_binding_value = binding_value;
    SetDescriptorSet(descriptor_set);
}

void ProgramArgumentBinding::SetDescriptorSet(const vk::DescriptorSet& descriptor_set) noexcept
{
    META_FUNCTION_TASK();
    if (m_vk_descriptor_set == descriptor_set)
        return;

    m_vk_descriptor_set = descriptor_set;

    if (const Rhi::ResourceViews& resource_views = GetResourceViews();
        !resource_views.empty())
    {
        SetDescriptorsForResourceViews(resource_views);
    }
}

void ProgramArgumentBinding::SetPushConstantsOffset(uint32_t push_constant_offset) noexcept
{
    META_FUNCTION_TASK();
    m_vk_push_constants_offset = push_constant_offset;
}

Ptr<Base::ProgramArgumentBinding> ProgramArgumentBinding::CreateCopy() const
{
    META_FUNCTION_TASK();
    return std::make_shared<ProgramArgumentBinding>(*this);
}

void ProgramArgumentBinding::MergeSettings(const Base::ProgramArgumentBinding& other)
{
    META_FUNCTION_TASK();
    Base::ProgramArgumentBinding::MergeSettings(other);
    m_settings_vk.argument = Base::ProgramArgumentBinding::GetSettings().argument;

    const Settings& settings_vk = dynamic_cast<const ProgramArgumentBinding&>(other).GetVulkanSettings();
    META_CHECK_EQUAL(m_settings_vk.descriptor_type, settings_vk.descriptor_type);
    m_settings_vk.byte_code_maps.insert(m_settings_vk.byte_code_maps.end(), settings_vk.byte_code_maps.begin(), settings_vk.byte_code_maps.end());
}

template<typename VkDescriptorType>
bool AddDescriptor(std::vector<VkDescriptorType>& descriptors, size_t total_descriptors_count, const VkDescriptorType* descriptor_ptr)
{
    META_FUNCTION_TASK();
    if (!descriptor_ptr)
        return false;

    if (descriptors.empty())
        descriptors.reserve(total_descriptors_count);

    descriptors.push_back(*descriptor_ptr);
    return true;
}

bool ProgramArgumentBinding::SetResourceViews(const Rhi::ResourceViews& resource_views)
{
    META_FUNCTION_TASK();
    if (!Base::ProgramArgumentBinding::SetResourceViews(resource_views))
        return false;

    SetDescriptorsForResourceViews(resource_views);
    return true;
}

void ProgramArgumentBinding::UpdateDescriptorSetsOnGpu()
{
    META_FUNCTION_TASK();
    if (m_vk_descriptor_images.empty() && m_vk_descriptor_buffers.empty() && m_vk_buffer_views.empty())
        return;

    const auto& vulkan_context = dynamic_cast<const IContext&>(GetContext());
    vulkan_context.GetVulkanDevice().GetNativeDevice().updateDescriptorSets(m_vk_write_descriptor_set, {});

    m_vk_descriptor_images.clear();
    m_vk_descriptor_buffers.clear();
    m_vk_buffer_views.clear();
}

bool ProgramArgumentBinding::UpdateRootConstantResourceViews()
{
    if (!Base::ProgramArgumentBinding::UpdateRootConstantResourceViews())
        return false;

    if (m_vk_descriptor_set)
    {
        SetDescriptorsForResourceViews(GetResourceViews());
    }

    const Rhi::RootConstant root_constants = GetRootConstant();
    Data::Emitter<Rhi::IProgramBindings::IArgumentBindingCallback>::Emit(
        &Rhi::IProgramArgumentBindingCallback::OnProgramArgumentBindingRootConstantChanged,
        std::cref(*this), std::cref(root_constants)
    );
    return true;
}

void ProgramArgumentBinding::SetDescriptorsForResourceViews(const Rhi::ResourceViews& resource_views)
{
    META_FUNCTION_TASK();
    META_CHECK_TRUE_DESCR(!!m_vk_descriptor_set, "program argument binding descriptor set was not initialized!");

    m_vk_descriptor_images.clear();
    m_vk_descriptor_buffers.clear();
    m_vk_buffer_views.clear();

    const size_t total_resources_count = resource_views.size();
    const Rhi::ResourceUsageMask resource_usage = GetResourceUsageByDescriptorType(m_settings_vk.descriptor_type);

    for(const Rhi::ResourceView& resource_view : resource_views)
    {
        const ResourceView resource_view_vk(resource_view, resource_usage);

        if (AddDescriptor(m_vk_descriptor_images, total_resources_count, resource_view_vk.GetNativeDescriptorImageInfoPtr()))
            continue;

        if (AddDescriptor(m_vk_descriptor_buffers, total_resources_count, resource_view_vk.GetNativeDescriptorBufferInfoPtr()))
            continue;

        AddDescriptor(m_vk_buffer_views, total_resources_count, resource_view_vk.GetNativeBufferViewPtr());
    }

    m_vk_write_descriptor_set = vk::WriteDescriptorSet(
        m_vk_descriptor_set,
        m_vk_binding_value,
        0U,
        m_settings_vk.descriptor_type,
        m_vk_descriptor_images,
        m_vk_descriptor_buffers,
        m_vk_buffer_views
    );

    // Descriptions are updated on GPU during context initialization complete
    if (GetContext().GetOptions().HasBit(Rhi::ContextOption::DeferredProgramBindingsInitialization))
    {
        GetContext().RequestDeferredAction(Rhi::IContext::DeferredAction::CompleteInitialization);
    }
    else
    {
        UpdateDescriptorSetsOnGpu();
    }
}

} // namespace Methane::Graphics::Vulkan
