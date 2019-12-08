/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
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

#include <Methane/Graphics/RenderPass.h>

namespace Methane::Graphics
{

class ContextBase;
class RenderCommandListBase;

class RenderPassBase
    : public RenderPass
    , public std::enable_shared_from_this<RenderPassBase>
{
public:
    RenderPassBase(ContextBase& context, const Settings& settings);

    // RenderPass interface
    void Update(const Settings& settings) override;
    const Settings& GetSettings() const override    { return m_settings; }

    // RenderPassBase interface
    virtual void Begin(RenderCommandListBase& command_list);
    virtual void End(RenderCommandListBase& command_list);

    Ptr            GetPtr() { return shared_from_this(); }
    Resource::Refs GetColorAttachmentResources() const;
    bool           IsBegun() const { return m_is_begun; }

protected:
    ContextBase& m_context;
    Settings     m_settings;
    bool         m_is_begun = false;
};

} // namespace Methane::Graphics
