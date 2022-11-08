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

FILE: Methane/Graphics/Metal/Sampler.hh
Metal implementation of the sampler interface.

******************************************************************************/

#pragma once

#include "Resource.hh"

#include <Methane/Graphics/Base/Sampler.h>

#import <Metal/Metal.h>

namespace Methane::Graphics::Metal
{

struct IContext;

class Sampler final : public Resource<Base::Sampler>
{
public:
    Sampler(const Base::Context& context, const Settings& settings);

    // IObject interface
    bool SetName(const std::string& name) override;
    
    const id<MTLSamplerState>& GetNativeSamplerState() const noexcept { return m_mtl_sampler_state; }

private:
    void ResetSamplerState();

    MTLSamplerDescriptor* m_mtl_sampler_desc = nullptr;
    id<MTLSamplerState>   m_mtl_sampler_state = nil;
};

} // namespace Methane::Graphics::Metal
