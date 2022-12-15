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

FILE: Methane/Graphics/DirectX/ViewState.h
DirectX 12 implementation of the view state interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/ViewState.h>

#include <wrl.h>
#include <directx/d3d12.h>
#include <directx/d3dx12.h>

namespace Methane::Graphics::Base
{

class RenderCommandList;

} // namespace Methane::Graphics::Base

namespace Methane::Graphics::DirectX
{

namespace wrl = Microsoft::WRL;

class ViewState final
    : public Base::ViewState
{
public:
    explicit ViewState(const Settings& settings);

    // IViewState overrides
    bool Reset(const Settings& settings) override;
    bool SetViewports(const Viewports& viewports) override;
    bool SetScissorRects(const ScissorRects& scissor_rects) override;

    // Base::ViewState interface
    void Apply(Base::RenderCommandList& command_list) override;

private:
    std::vector<CD3DX12_VIEWPORT> m_dx_viewports;
    std::vector<CD3DX12_RECT>     m_dx_scissor_rects;
};

} // namespace Methane::Graphics::DirectX
