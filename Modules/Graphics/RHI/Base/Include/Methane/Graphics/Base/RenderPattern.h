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

FILE: Methane/Graphics/Base/RenderPattern.h
Base implementation of the render pattern interface.

******************************************************************************/

#pragma once

#include "Object.h"

#include <Methane/Graphics/RHI/IRenderPattern.h>

namespace Methane::Graphics::Base
{

class RenderContext;

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

} // namespace Methane::Graphics::Base
