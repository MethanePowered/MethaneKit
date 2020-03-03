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

FILE: Methane/Graphics/Metal/RenderStateMT.hh
Metal implementation of the render state interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RenderStateBase.h>

#import <Metal/Metal.h>

#include <vector>

namespace Methane::Graphics
{

class RenderContextMT;

class RenderStateMT final : public RenderStateBase
{
public:
    RenderStateMT(RenderContextBase& context, const Settings& settings);
    ~RenderStateMT() override;
    
    // RenderState interface
    void Reset(const Settings& settings) override;
    void SetViewports(const Viewports& viewports) override;
    void SetScissorRects(const ScissorRects& scissor_rects) override;

    // RenderStateBase interface
    void Apply(RenderCommandListBase& command_list, Group::Mask state_groups) override;

    // Object interface
    void SetName(const std::string& name) override;
    
    void InitializeNativeStates();
    void InitializeNativePipelineState();
    void InitializeNativeDepthStencilState();
    
    id<MTLRenderPipelineState>& GetNativePipelineState();
    id<MTLDepthStencilState>&   GetNativeDepthStencilState();
    MTLCullMode                 GetNativeCullMode() const noexcept         { return m_mtl_cull_mode; }
    MTLWinding                  GetNativeFrontFaceWinding() const noexcept { return m_mtl_front_face_winding; }

private:
    RenderContextMT& GetRenderContextMT();
    
    void ResetNativeState();
    
    MTLRenderPipelineDescriptor* m_mtl_pipeline_state_desc = nil;
    MTLDepthStencilDescriptor*   m_mtl_depth_stencil_state_desc = nil;
    id<MTLRenderPipelineState>   m_mtl_pipeline_state = nil;
    id<MTLDepthStencilState>     m_mtl_depth_state = nil;
    std::vector<MTLViewport>     m_mtl_viewports;
    std::vector<MTLScissorRect>  m_mtl_scissor_rects;
    MTLTriangleFillMode          m_mtl_fill_mode = MTLTriangleFillModeFill;
    MTLCullMode                  m_mtl_cull_mode = MTLCullModeBack;
    MTLWinding                   m_mtl_front_face_winding = MTLWindingClockwise;
};

} // namespace Methane::Graphics
