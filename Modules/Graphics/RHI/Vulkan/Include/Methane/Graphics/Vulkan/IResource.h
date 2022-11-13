/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/IResource.h
Vulkan specialization of the resource interface.

******************************************************************************/

#pragma once

#include "ResourceView.h"

#include <Methane/Graphics/RHI/IResource.h>

#include <vulkan/vulkan.hpp>

#include <variant>
#include <vector>

namespace Methane::Graphics::Vulkan
{

struct IResource
    : virtual Rhi::IResource // NOSONAR
{
public:
    using Barrier  = Rhi::ResourceBarrier;
    using Barriers = Rhi::IResourceBarriers;
    using State    = Rhi::ResourceState;
    using View     = ResourceView;
    using Views    = ResourceViews;

    [[nodiscard]] virtual const IContext&         GetVulkanContext() const noexcept = 0;
    [[nodiscard]] virtual const vk::DeviceMemory& GetNativeDeviceMemory() const noexcept = 0;
    [[nodiscard]] virtual const vk::Device&       GetNativeDevice() const noexcept = 0;
    [[nodiscard]] virtual const Opt<uint32_t>&    GetOwnerQueueFamilyIndex() const noexcept = 0;

    virtual const Ptr<ResourceView::ViewDescriptorVariant>& InitializeNativeViewDescriptor(const View::Id& view_id) = 0;

    [[nodiscard]] static vk::AccessFlags        GetNativeAccessFlagsByResourceState(Rhi::ResourceState resource_state);
    [[nodiscard]] static vk::ImageLayout        GetNativeImageLayoutByResourceState(Rhi::ResourceState resource_state);
    [[nodiscard]] static vk::PipelineStageFlags GetNativePipelineStageFlagsByResourceState(Rhi::ResourceState resource_state);
};

} // namespace Methane::Graphics::Vulkan
