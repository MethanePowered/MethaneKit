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

FILE: Methane/Graphics/DirectX12/ProgramArgumentBinding.h
Vulkan implementation of the program argument binding interface.

******************************************************************************/

#include <Methane/Graphics/Vulkan/ProgramArgumentBinding.h>
#include <Methane/Graphics/Vulkan/IContext.h>
#include <Methane/Graphics/Vulkan/Device.h>

#include <Methane/Graphics/Base/Context.h>

namespace Methane::Graphics::Base
{

Ptr<Base::ProgramArgumentBinding> ProgramArgumentBinding::CreateCopy(const Base::ProgramArgumentBinding& other_argument_binding)
{
    META_FUNCTION_TASK();
    return std::make_shared<Vulkan::ProgramArgumentBinding>(static_cast<const Vulkan::ProgramArgumentBinding&>(other_argument_binding));
}

} // namespace Methane::Graphics::Base

namespace Methane::Graphics::Vulkan
{

ProgramArgumentBinding::ProgramArgumentBinding(const Base::Context& context, const Settings& settings)
    : Base::ProgramArgumentBinding(context, settings)
    , m_settings_vk(settings)
{
    META_FUNCTION_TASK();
}

void ProgramArgumentBinding::SetDescriptorSetBinding(const vk::DescriptorSet& descriptor_set, uint32_t binding_value) noexcept
{
    META_FUNCTION_TASK();
    m_vk_descriptor_set_ptr = &descriptor_set;
    m_vk_binding_value      = binding_value;
}

void ProgramArgumentBinding::SetDescriptorSet(const vk::DescriptorSet& descriptor_set) noexcept
{
    META_FUNCTION_TASK();
    m_vk_descriptor_set_ptr = &descriptor_set;
}

void ProgramArgumentBinding::MergeSettings(const Base::ProgramArgumentBinding& other)
{
    META_FUNCTION_TASK();
    Base::ProgramArgumentBinding::MergeSettings(other);

    const Settings& settings_vk = dynamic_cast<const ProgramArgumentBinding&>(other).GetVulkanSettings();
    META_CHECK_ARG_EQUAL(m_settings_vk.descriptor_type, settings_vk.descriptor_type);
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

bool ProgramArgumentBinding::SetResourceViews(const Rhi::IResource::Views& resource_views)
{
    META_FUNCTION_TASK();
    if (!Base::ProgramArgumentBinding::SetResourceViews(resource_views))
        return false;

    META_CHECK_ARG_NOT_NULL(m_vk_descriptor_set_ptr);

    m_vk_descriptor_images.clear();
    m_vk_descriptor_buffers.clear();
    m_vk_buffer_views.clear();

    const size_t total_resources_count = resource_views.size();
    for(const Rhi::IResource::View& resource_view : resource_views)
    {
        const IResource::View resource_view_vk(resource_view, Rhi::ResourceUsage({ Rhi::ResourceUsage::Bit::ShaderRead }));

        if (AddDescriptor(m_vk_descriptor_images, total_resources_count, resource_view_vk.GetNativeDescriptorImageInfoPtr()))
            continue;

        if (AddDescriptor(m_vk_descriptor_buffers, total_resources_count, resource_view_vk.GetNativeDescriptorBufferInfoPtr()))
            continue;

        AddDescriptor(m_vk_buffer_views, total_resources_count, resource_view_vk.GetNativeBufferViewPtr());
    }

    m_vk_write_descriptor_set = vk::WriteDescriptorSet(
        *m_vk_descriptor_set_ptr,
        m_vk_binding_value,
        0U,
        m_settings_vk.descriptor_type,
        m_vk_descriptor_images,
        m_vk_descriptor_buffers,
        m_vk_buffer_views
    );

    // Descriptions are updated on GPU during context initialization complete
#ifdef DEFERRED_PROGRAM_BINDINGS_INITIALIZATION
    GetContext().RequestDeferredAction(Rhi::IContext::DeferredAction::CompleteInitialization);
#else
    UpdateDescriptorSetsOnGpu();
#endif
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

} // namespace Methane::Graphics::Vulkan
