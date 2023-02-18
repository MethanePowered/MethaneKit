/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Metal/ComputeState.hh
Metal implementation of the render state interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/ComputeState.h>

#import <Metal/Metal.h>

#include <vector>

namespace Methane::Graphics::Metal
{

class Device;

class ComputeState final
    : public Base::ComputeState
{
public:
    ComputeState(const Rhi::IContext& context, const Settings& settings);

    // IComputeState interface
    void Reset(const Settings& settings) override;

    // Base::ComputeState interface
    void Apply(Base::ComputeCommandList& command_list) override;

    // IObject interface
    bool SetName(std::string_view name) override;
    
    void InitializeNativeStates();
    void InitializeNativePipelineState();
    void InitializeNativeDepthStencilState();
    
    id<MTLComputePipelineState> GetNativePipelineState();

private:
    void ResetNativeState();

    const Device&                 m_device;
    MTLComputePipelineDescriptor* m_mtl_pipeline_state_desc = nil;
    id<MTLComputePipelineState>   m_mtl_pipeline_state = nil;
};

} // namespace Methane::Graphics::Metal
