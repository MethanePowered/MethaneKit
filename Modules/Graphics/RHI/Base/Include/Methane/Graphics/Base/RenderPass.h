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

FILE: Methane/Graphics/Base/RenderPass.h
Base implementation of the render pass interface.

******************************************************************************/

#pragma once

#include "Object.h"
#include "Resource.h"

#include <Methane/Graphics/RHI/IRenderPass.h>

namespace Methane::Graphics::Base
{

class RenderContext;
class RenderCommandList;
class Texture;

class RenderPattern
    : public Rhi::IRenderPattern
    , public Object
{
public:
    RenderPattern(RenderContext& render_context, const Settings& settings);

    // IRenderPattern overrides
    [[nodiscard]] const Rhi::IRenderContext& GetRenderContext() const noexcept final;
    [[nodiscard]] Rhi::IRenderContext&       GetRenderContext() noexcept final;
    [[nodiscard]] const Settings&            GetSettings() const noexcept final { return m_settings; }
    [[nodiscard]] Data::Size                 GetAttachmentCount() const noexcept final;
    [[nodiscard]] AttachmentFormats          GetAttachmentFormats() const noexcept final;

    [[nodiscard]] const RenderContext& GetBaseRenderContext() const noexcept { return *m_render_context_ptr; }
    [[nodiscard]] RenderContext&       GetBaseRenderContext() noexcept       { return *m_render_context_ptr; }

private:
    const Ptr<RenderContext> m_render_context_ptr;
    Settings m_settings;
};

class RenderPass
    : public Rhi::IRenderPass
    , public Object
    , public Data::Emitter<Rhi::IRenderPassCallback>
{
public:
    RenderPass(RenderPattern& pattern, const Settings& settings,
               bool update_attachment_states = true);

    // IRenderPass interface
    const IPattern&  GetPattern() const noexcept final { return *m_pattern_base_ptr; }
    IPattern& GetPattern() noexcept final              { return *m_pattern_base_ptr; }
    const Settings& GetSettings() const noexcept final { return m_settings; }
    bool Update(const Settings& settings) override;
    void ReleaseAttachmentTextures() override;

    // RenderPass interface
    virtual void Begin(RenderCommandList& render_command_list);
    virtual void End(RenderCommandList& render_command_list);

    const Rhi::TextureView& GetAttachmentTextureView(const Attachment& attachment) const;
    const Refs<Texture>&    GetColorAttachmentTextures() const;
    Texture*                GetDepthAttachmentTexture() const;
    Texture*                GetStencilAttachmentTexture() const;
    const Ptrs<Texture>&    GetNonFrameBufferAttachmentTextures() const;
    bool                    IsBegun() const noexcept   { return m_is_begun; }

protected:
    RenderPattern& GetBasePattern() const noexcept { return *m_pattern_base_ptr; }

    void SetAttachmentStates(const Opt<Rhi::ResourceState>& color_state,
                             const Opt<Rhi::ResourceState>& depth_state) const;
    void SetAttachmentStates(const Opt<Rhi::ResourceState>& color_state,
                             const Opt<Rhi::ResourceState>& depth_state,
                             Ptr<Rhi::IResourceBarriers>& transition_barriers_ptr,
                             RenderCommandList& render_command_list) const;

private:
    void InitAttachmentStates() const;

    Ptr<RenderPattern>    m_pattern_base_ptr;
    Settings              m_settings;
    const bool            m_update_attachment_states;
    bool                  m_is_begun = false;
    mutable Refs<Texture> m_color_attachment_textures;
    mutable Ptrs<Texture> m_non_frame_buffer_attachment_textures;
    mutable Texture*      m_p_depth_attachment_texture = nullptr;
    mutable Texture*      m_p_stencil_attachment_texture = nullptr;
};

} // namespace Methane::Graphics::Base
