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

class FrameBufferTextureVK final : public ResourceVK<TextureBase, vk::ImageView>
{
public:
    FrameBufferTextureVK(const RenderContextVK& render_context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage,
                         FrameBufferIndex frame_buffer_index);

    [[nodiscard]] FrameBufferIndex     GetFrameBufferIndex() const noexcept { return m_frame_buffer_index; }
    [[nodiscard]] const vk::Image&     GetNativeImage() const noexcept      { return m_vk_image; }
    [[nodiscard]] const vk::ImageView& GetNativeImageView() const noexcept  { return GetNativeResource(); }

    void ResetNativeImage();

private:
    FrameBufferTextureVK(const RenderContextVK& render_context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage,
                         FrameBufferIndex frame_buffer_index, const vk::Image& image);

    // Resource interface
    void SetData(const SubResources& sub_resources, CommandQueue*) override;

    const RenderContextVK& m_render_context;
    const FrameBufferIndex m_frame_buffer_index;
    vk::Image              m_vk_image;
};

class DepthStencilTextureVK final : public ResourceVK<TextureBase, vk::ImageView>
{
public:
    DepthStencilTextureVK(const RenderContextVK& render_context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage,
                          const Opt<DepthStencil>& depth_stencil_opt);

    // Resource interface
    void SetData(const SubResources& sub_resources, CommandQueue*) override;

private:
    const RenderContextVK& m_render_context;
    Opt<DepthStencil>      m_depth_stencil_opt;
};

class RenderTargetTextureVK final : public ResourceVK<TextureBase, vk::ImageView>
{
public:
    RenderTargetTextureVK(const RenderContextVK& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage);

    // Resource interface
    void SetData(const SubResources& sub_resources, CommandQueue*) override;

private:
    const RenderContextVK& m_render_context;
};

class ImageTextureVK final : public ResourceVK<TextureBase, vk::Image>
{
public:
    ImageTextureVK(const ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage);

    // Resource interface
    void SetData(const SubResources& sub_resources, CommandQueue*) override;

private:
    void GenerateMipLevels();

    vk::ImageView          m_vk_image_view;
    vk::UniqueBuffer       m_vk_unique_staging_buffer;
    vk::UniqueDeviceMemory m_vk_unique_staging_memory;
};

} // namespace Methane::Graphics
