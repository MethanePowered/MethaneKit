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

FILE: Methane/Graphics/Metal/SamplerMT.hh
Metal implementation of the sampler interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/SamplerBase.h>

#import <Metal/Metal.h>

namespace Methane
{
namespace Graphics
{

class ContextMT;

class SamplerMT : public SamplerBase
{
public:
    SamplerMT(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage);
    ~SamplerMT() override;

    // Object interface
    void SetName(const std::string& name) override;
    
    const id<MTLSamplerState>& GetNativeSamplerState() const noexcept { return m_mtl_sampler_state; }

protected:
    void ResetSampletState();

    ContextMT& GetContextMT() noexcept;
    
    MTLSamplerDescriptor* m_mtl_sampler_desc = nullptr;
    id<MTLSamplerState>   m_mtl_sampler_state = nil;
};

} // namespace Graphics
} // namespace Methane
