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

FILE: Methane/Graphics/RenderPassBase.h
Base implementation of the render pass interface.

******************************************************************************/

#pragma once

#include "ObjectBase.h"
#include "ResourceBase.h"

#include <Methane/Graphics/RenderPass.h>

namespace Methane::Graphics
{

class RenderContextBase;
class RenderCommandListBase;
class TextureBase;

class RenderPatternBase
    : public RenderPattern
    , public ObjectBase
{
public:
    RenderPatternBase(RenderContextBase& render_context, const Settings& settings);

    // RenderPattern overrides
    [[nodiscard]] const IRenderContext& GetRenderContext() const noexcept final;
    [[nodiscard]] IRenderContext&       GetRenderContext() noexcept final;
    [[nodiscard]] const Settings&       GetSettings() const noexcept final { return m_settings; }
    [[nodiscard]] Data::Size            GetAttachmentCount() const noexcept final;
    [[nodiscard]] AttachmentFormats     GetAttachmentFormats() const noexcept final;

    [[nodiscard]] const RenderContextBase& GetRenderContextBase() const noexcept { return *m_render_context_ptr; }
    [[nodiscard]] RenderContextBase&       GetRenderContextBase() noexcept       { return *m_render_context_ptr; }

private:
    const Ptr<RenderContextBase> m_render_context_ptr;
    Settings m_settings;
};

class RenderPassBase
    : public RenderPass
    , public ObjectBase
    , public Data::Emitter<IRenderPassCallback>
{
public:
    RenderPassBase(RenderPatternBase& pattern, const Settings& settings,
                   bool update_attachment_states = true);

    // RenderPass interface
    const Pattern&  GetPattern() const noexcept final  { return *m_pattern_base_ptr; }
    const Settings& GetSettings() const noexcept final { return m_settings; }
    bool Update(const Settings& settings) override;
    void ReleaseAttachmentTextures() override;

    // RenderPassBase interface
    virtual void Begin(RenderCommandListBase& render_command_list);
    virtual void End(RenderCommandListBase& render_command_list);

    const Texture::View&     GetAttachmentTextureView(const Attachment& attachment) const;
    const Refs<TextureBase>& GetColorAttachmentTextures() const;
    TextureBase*             GetDepthAttachmentTexture() const;
    TextureBase*             GetStencilAttachmentTexture() const;
    const Ptrs<TextureBase>& GetNonFrameBufferAttachmentTextures() const;
    bool                     IsBegun() const noexcept   { return m_is_begun; }

protected:
    RenderPatternBase& GetPatternBase() const noexcept { return *m_pattern_base_ptr; }

    void SetAttachmentStates(const std::optional<Resource::State>& color_state,
                             const std::optional<Resource::State>& depth_state) const;
    void SetAttachmentStates(const std::optional<Resource::State>& color_state,
                             const std::optional<Resource::State>& depth_state,
                             Ptr<Resource::Barriers>& transition_barriers_ptr,
                             RenderCommandListBase& render_command_list) const;

private:
    void InitAttachmentStates() const;

    Ptr<RenderPatternBase>    m_pattern_base_ptr;
    Settings                  m_settings;
    const bool                m_update_attachment_states;
    bool                      m_is_begun = false;
    mutable Refs<TextureBase> m_color_attachment_textures;
    mutable Ptrs<TextureBase> m_non_frame_buffer_attachment_textures;
    mutable TextureBase*      m_p_depth_attachment_texture = nullptr;
    mutable TextureBase*      m_p_stencil_attachment_texture = nullptr;
};

} // namespace Methane::Graphics
