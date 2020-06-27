/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: HelloTriangleApp.cpp
Tutorial demonstrating triangle rendering with Methane graphics API

******************************************************************************/

#pragma once

#include <Methane/Graphics/Kit.h>

#include <string>
#include <array>

namespace Methane::Tutorials
{

namespace gfx = Methane::Graphics;

struct HelloTriangleFrame final : gfx::AppFrame
{
    Ptr<gfx::RenderCommandList> sp_render_cmd_list;
    Ptr<gfx::CommandListSet>    sp_execute_cmd_lists;

    using gfx::AppFrame::AppFrame;
};

using GraphicsApp = gfx::App<HelloTriangleFrame>;

class HelloTriangleApp final : public GraphicsApp
{
public:
    HelloTriangleApp();
    ~HelloTriangleApp() override;

    // GraphicsApp overrides
    void Init() override;
    bool Resize(const gfx::FrameSize& frame_size, bool is_minimized) override;
    bool Render() override;

    // IContextCallback interface
    void OnContextReleased(gfx::Context& context) override;

private:
    Ptr<gfx::RenderState> m_sp_state;
    Ptr<gfx::BufferSet>   m_sp_vertex_buffers;
};

} // namespace Methane::Tutorials
