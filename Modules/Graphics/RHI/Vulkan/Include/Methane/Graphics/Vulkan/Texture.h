/******************************************************************************

Copyright 2019-2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/Texture.h
Vulkan implementation of the texture interface.

******************************************************************************/

#pragma once

#include "Resource.hpp"

#include <Methane/Graphics/Base/Texture.h>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics::Vulkan
{

class RenderContext;

class Texture final // NOSONAR - inheritance hierarchy is greater than 5
    : public Resource<Base::Texture, vk::Image, false>
{
public:
    [[nodiscard]] static vk::ImageType        DimensionTypeToImageType(Rhi::TextureDimensionType dimension_type);
    [[nodiscard]] static vk::ImageViewType    DimensionTypeToImageViewType(Rhi::TextureDimensionType dimension_type);
    [[nodiscard]] static vk::ImageAspectFlags GetNativeImageAspectFlags(const Rhi::TextureSettings& settings);
    [[nodiscard]] static vk::ImageUsageFlags  GetNativeImageUsageFlags(const Rhi::TextureSettings& settings,
                                                                       vk::ImageUsageFlags initial_usage_flags = {});

    Texture(const Base::Context& context, const Settings& settings);
    Texture(const RenderContext& render_context, const Settings& settings, Data::Index frame_index);

    void ResetNativeFrameImage();

    // ITexture interface
    void SetData(Rhi::ICommandQueue& target_cmd_queue, const SubResources& sub_resources) override;
    SubResource GetData(Rhi::ICommandQueue& target_cmd_queue,
                        const SubResource::Index& sub_resource_index = {},
                        const BytesRangeOpt& data_range = {}) override;

    // IObject overide
    bool SetName(std::string_view name) override;

    // ITexture overrides
    const vk::Image& GetNativeImage() const noexcept { return GetNativeResource(); }
    vk::ImageSubresourceRange GetNativeSubresourceRange() const;

private:
    Texture(const Base::Context& context, const Settings& settings, vk::UniqueImage&& vk_unique_image);

    void InitializeAsImage();
    void InitializeAsRenderTarget();
    void InitializeAsDepthStencil();

    // Resource override
    Ptr<ResourceView::ViewDescriptorVariant> CreateNativeViewDescriptor(const ResourceView::Id& view_id) override;

    void GenerateMipLevels(Rhi::ICommandQueue& target_cmd_queue, State target_resource_state);

    vk::UniqueImage                  m_vk_unique_image;
    vk::UniqueBuffer                 m_vk_unique_staging_buffer;
    vk::UniqueDeviceMemory           m_vk_unique_staging_memory;
    std::vector<vk::BufferImageCopy> m_vk_copy_regions;
};

} // namespace Methane::Graphics::Vulkan
