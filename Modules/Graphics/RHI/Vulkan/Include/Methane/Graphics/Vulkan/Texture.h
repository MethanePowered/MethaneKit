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

struct ITexture
{
    [[nodiscard]] static vk::ImageType        DimensionTypeToImageType(Rhi::TextureDimensionType dimension_type);
    [[nodiscard]] static vk::ImageViewType    DimensionTypeToImageViewType(Rhi::TextureDimensionType dimension_type);
    [[nodiscard]] static vk::ImageAspectFlags GetNativeImageAspectFlags(const Rhi::TextureSettings& settings);
    [[nodiscard]] static vk::ImageUsageFlags  GetNativeImageUsageFlags(const Rhi::TextureSettings& settings,
                                                                       vk::ImageUsageFlags initial_usage_flags = {});

    [[nodiscard]] virtual const vk::Image& GetNativeImage() const noexcept = 0;
    [[nodiscard]] virtual vk::ImageSubresourceRange GetNativeSubresourceRange() const noexcept = 0;

    virtual ~ITexture() = default;
};

class FrameBufferTexture final // NOSONAR - inheritance hierarchy is greater than 5
    : public Resource<Base::Texture, vk::Image, false>
    , public ITexture
{
public:
    FrameBufferTexture(const RenderContext& render_context, const Settings& settings, FrameBufferIndex frame_buffer_index);

    [[nodiscard]] FrameBufferIndex GetFrameBufferIndex() const noexcept { return m_frame_buffer_index; }

    // ITexture interface
    const vk::Image& GetNativeImage() const noexcept override { return GetNativeResource(); }
    vk::ImageSubresourceRange GetNativeSubresourceRange() const noexcept override;

    void ResetNativeImage();

private:
    // Resource override
    Ptr<ResourceView::ViewDescriptorVariant> CreateNativeViewDescriptor(const ResourceView::Id& view_id) override;

    // IResource interface
    void SetData(const SubResources& sub_resources, Rhi::ICommandQueue&) override;

    const RenderContext& m_render_context;
    const FrameBufferIndex m_frame_buffer_index;
};

class DepthStencilTexture final // NOSONAR - inheritance hierarchy is greater than 5
    : public Resource<Base::Texture, vk::Image, true>
    , public ITexture
{
public:
    DepthStencilTexture(const RenderContext& render_context, const Settings& settings,
                        const Opt<DepthStencil>& depth_stencil_opt);

    // IResource interface
    void SetData(const SubResources& sub_resources, Rhi::ICommandQueue&) override;

    // ITexture interface
    const vk::Image& GetNativeImage() const noexcept override { return GetNativeResource(); }
    vk::ImageSubresourceRange GetNativeSubresourceRange() const noexcept override;

private:
    // Resource override
    Ptr<ResourceView::ViewDescriptorVariant> CreateNativeViewDescriptor(const ResourceView::Id& view_id) override;

    Opt<DepthStencil>      m_depth_stencil_opt;
};

class RenderTargetTexture final // NOSONAR - inheritance hierarchy is greater than 5
    : public Resource<Base::Texture, vk::Image, true>
    , public ITexture
{
public:
    RenderTargetTexture(const RenderContext& context, const Settings& settings);

    // IResource interface
    void SetData(const SubResources& sub_resources, Rhi::ICommandQueue&) override;

    // ITexture interface
    const vk::Image& GetNativeImage() const noexcept override { return GetNativeResource(); }
    vk::ImageSubresourceRange GetNativeSubresourceRange() const noexcept override;

private:
    // Resource override
    Ptr<ResourceView::ViewDescriptorVariant> CreateNativeViewDescriptor(const ResourceView::Id& view_id) override;
};

class ImageTexture final // NOSONAR - inheritance hierarchy is greater than 5
    : public Resource<Base::Texture, vk::Image, true>
    , public ITexture
{
public:
    ImageTexture(const Base::Context& context, const Settings& settings);

    // IResource interface
    void SetData(const SubResources& sub_resources, Rhi::ICommandQueue&) override;

    // IObject overide
    bool SetName(std::string_view name) override;

    // ITexture overrides
    const vk::Image& GetNativeImage() const noexcept override { return GetNativeResource(); }
    vk::ImageSubresourceRange GetNativeSubresourceRange() const noexcept override;

private:
    // Resource override
    Ptr<ResourceView::ViewDescriptorVariant> CreateNativeViewDescriptor(const ResourceView::Id& view_id) override;

    void GenerateMipLevels(Rhi::ICommandQueue& target_cmd_queue, State target_resource_state);

    vk::UniqueBuffer                 m_vk_unique_staging_buffer;
    vk::UniqueDeviceMemory           m_vk_unique_staging_memory;
    std::vector<vk::BufferImageCopy> m_vk_copy_regions;
};

} // namespace Methane::Graphics::Vulkan
