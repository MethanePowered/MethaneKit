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

class RenderPassBase
    : public RenderPass
    , public ObjectBase
{
public:
    RenderPassBase(RenderContextBase& context, const Settings& settings);

    // RenderPass interface
    const Settings& GetSettings() const override    { return m_settings; }
    bool Update(const Settings& settings) override;
    void ReleaseAttachmentTextures() override;

    // RenderPassBase interface
    virtual void Begin(RenderCommandListBase& render_command_list);
    virtual void End(RenderCommandListBase& render_command_list);

    const Refs<TextureBase>& GetColorAttachmentTextures() const;
    TextureBase*             GetDepthAttachmentTexture() const;
    const Ptrs<TextureBase>& GetNonFrameBufferAttachmentTextures() const;
    Ptr<RenderPassBase>      GetRenderPassPtr()         { return std::static_pointer_cast<RenderPassBase>(GetBasePtr()); }
    bool                     IsBegun() const            { return m_is_begun; }

protected:
    RenderContextBase&        GetRenderContext()        { return m_render_context; }
    const RenderContextBase&  GetRenderContext() const  { return m_render_context; }

private:
    void InitAttachmentStates() const;
    void SetAttachmentStates(const std::optional<Resource::State>& color_state,
                             const std::optional<Resource::State>& depth_state,
                             Ptr<Resource::Barriers>& transition_barriers_ptr,
                             RenderCommandListBase& render_command_list) const;

    RenderContextBase&          m_render_context;
    Settings                    m_settings;
    bool                        m_is_begun = false;
    mutable Refs<TextureBase>   m_color_attachment_textures;
    mutable Ptrs<TextureBase>   m_non_frame_buffer_attachment_textures;
    mutable TextureBase*        m_p_depth_attachment_texture = nullptr;
    Ptr<Resource::Barriers> m_begin_transition_barriers_ptr;
    Ptr<Resource::Barriers> m_end_transition_barriers_ptr;
};

} // namespace Methane::Graphics
