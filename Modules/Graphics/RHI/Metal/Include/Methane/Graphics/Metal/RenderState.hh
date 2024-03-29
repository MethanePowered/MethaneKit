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

FILE: Methane/Graphics/Metal/RenderState.hh
Metal implementation of the render state interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/RenderState.h>

#import <Metal/Metal.h>

#include <vector>

namespace Methane::Graphics::Metal
{

class RenderContext;

class RenderState final
    : public Base::RenderState
{
public:
    RenderState(const Base::RenderContext& context, const Settings& settings);

    // IRenderState interface
    void Reset(const Settings& settings) override;

    // Base::RenderState interface
    void Apply(Base::RenderCommandList& command_list, Groups state_groups) override;

    // IObject interface
    bool SetName(std::string_view name) override;
    
    void InitializeNativeStates();
    void InitializeNativePipelineState();
    void InitializeNativeDepthStencilState();
    
    id<MTLRenderPipelineState> GetNativePipelineState();
    id<MTLDepthStencilState>   GetNativeDepthStencilState();
    MTLCullMode                GetNativeCullMode() const noexcept         { return m_mtl_cull_mode; }
    MTLWinding                 GetNativeFrontFaceWinding() const noexcept { return m_mtl_front_face_winding; }

private:
    const RenderContext& GetMetalRenderContext() const;
    
    void ResetNativeState();
    
    MTLRenderPipelineDescriptor* m_mtl_pipeline_state_desc = nil;
    MTLDepthStencilDescriptor*   m_mtl_depth_stencil_state_desc = nil;
    id<MTLRenderPipelineState>   m_mtl_pipeline_state = nil;
    id<MTLDepthStencilState>     m_mtl_depth_state = nil;
    MTLTriangleFillMode          m_mtl_fill_mode = MTLTriangleFillModeFill;
    MTLCullMode                  m_mtl_cull_mode = MTLCullModeBack;
    MTLWinding                   m_mtl_front_face_winding = MTLWindingClockwise;
};

} // namespace Methane::Graphics::Metal
