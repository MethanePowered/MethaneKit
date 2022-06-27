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

FILE: Methane/Graphics/Vulkan/TextureVK.h
Vulkan implementation of the texture interface.

******************************************************************************/

#pragma once

#include "ResourceVK.hpp"

#include <Methane/Graphics/TextureBase.h>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics
{

class RenderContextVK;

struct ITextureVK
{
    [[nodiscard]] static vk::ImageType        DimensionTypeToImageType(Texture::DimensionType dimension_type);
    [[nodiscard]] static vk::ImageViewType    DimensionTypeToImageViewType(Texture::DimensionType dimension_type);
    [[nodiscard]] static vk::ImageAspectFlags GetNativeImageAspectFlags(const Texture::Settings& settings);
    [[nodiscard]] static vk::ImageUsageFlags  GetNativeImageUsageFlags(const Texture::Settings& settings,
                                                                       vk::ImageUsageFlags initial_usage_flags = {});

    [[nodiscard]] virtual const vk::Image& GetNativeImage() const noexcept = 0;
    [[nodiscard]] virtual vk::ImageSubresourceRange GetNativeSubresourceRange() const noexcept = 0;

    virtual ~ITextureVK() = default;
};

class FrameBufferTextureVK final
    : public ResourceVK<TextureBase, vk::Image, false>
    , public ITextureVK
{
public:
    FrameBufferTextureVK(const RenderContextVK& render_context, const Settings& settings, FrameBufferIndex frame_buffer_index);

    [[nodiscard]] FrameBufferIndex GetFrameBufferIndex() const noexcept { return m_frame_buffer_index; }

    // ITextureVK interface
    const vk::Image& GetNativeImage() const noexcept override { return GetNativeResource(); }
    vk::ImageSubresourceRange GetNativeSubresourceRange() const noexcept override;

    void ResetNativeImage();

private:
    // ResourceVK override
    Ptr<ResourceViewVK::ViewDescriptorVariant> CreateNativeViewDescriptor(const View::Id& view_id) override;

    // Resource interface
    void SetData(const SubResources& sub_resources, CommandQueue&) override;

    const RenderContextVK& m_render_context;
    const FrameBufferIndex m_frame_buffer_index;
};

class DepthStencilTextureVK final
    : public ResourceVK<TextureBase, vk::Image, true>
    , public ITextureVK
{
public:
    DepthStencilTextureVK(const RenderContextVK& render_context, const Settings& settings,
                          const Opt<DepthStencil>& depth_stencil_opt);

    // Resource interface
    void SetData(const SubResources& sub_resources, CommandQueue&) override;

    // ITextureVK interface
    const vk::Image& GetNativeImage() const noexcept override { return GetNativeResource(); }
    vk::ImageSubresourceRange GetNativeSubresourceRange() const noexcept override;

private:
    // ResourceVK override
    Ptr<ResourceViewVK::ViewDescriptorVariant> CreateNativeViewDescriptor(const View::Id& view_id) override;

    Opt<DepthStencil>      m_depth_stencil_opt;
};

class RenderTargetTextureVK final
    : public ResourceVK<TextureBase, vk::Image, true>
    , public ITextureVK
{
public:
    RenderTargetTextureVK(const RenderContextVK& context, const Settings& settings);

    // Resource interface
    void SetData(const SubResources& sub_resources, CommandQueue&) override;

    // ITextureVK interface
    const vk::Image& GetNativeImage() const noexcept override { return GetNativeResource(); }
    vk::ImageSubresourceRange GetNativeSubresourceRange() const noexcept override;

private:
    // ResourceVK override
    Ptr<ResourceViewVK::ViewDescriptorVariant> CreateNativeViewDescriptor(const View::Id& view_id) override;
};

class ImageTextureVK final
    : public ResourceVK<TextureBase, vk::Image, true>
    , public ITextureVK
{
public:
    ImageTextureVK(const ContextBase& context, const Settings& settings);

    // Resource interface
    void SetData(const SubResources& sub_resources, CommandQueue&) override;

    // Object overide
    bool SetName(const std::string& name) override;

    // ITextureVK overrides
    const vk::Image& GetNativeImage() const noexcept override { return GetNativeResource(); }
    vk::ImageSubresourceRange GetNativeSubresourceRange() const noexcept override;

private:
    // ResourceVK override
    Ptr<ResourceViewVK::ViewDescriptorVariant> CreateNativeViewDescriptor(const View::Id& view_id) override;

    void GenerateMipLevels(CommandQueue& target_cmd_queue, State target_resource_state);

    vk::UniqueBuffer                 m_vk_unique_staging_buffer;
    vk::UniqueDeviceMemory           m_vk_unique_staging_memory;
    std::vector<vk::BufferImageCopy> m_vk_copy_regions;
};

} // namespace Methane::Graphics
