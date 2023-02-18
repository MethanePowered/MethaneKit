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

FILE: Methane/Graphics/Null/Context.hpp
Null template implementation of the base context interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/Device.h>
#include <Methane/Graphics/Base/Context.h>
#include <Methane/Graphics/Base/DescriptorManager.h>

namespace Methane::Graphics::Null
{

template<class ContextBaseT, typename = std::enable_if_t<std::is_base_of_v<Base::Context, ContextBaseT>>>
class Context
    : public ContextBaseT
{
public:
    Context(Base::Device& device, tf::Executor& parallel_executor, const typename ContextBaseT::Settings& settings)
        : ContextBaseT(device, std::make_unique<Base::DescriptorManager>(*this), parallel_executor, settings)
    {
    }

    // IContext overrides

    [[nodiscard]] Ptr<Rhi::IComputeState> CreateComputeState(const Rhi::ComputeStateSettings& settings) const final
    {
        META_UNUSED(settings);
        META_FUNCTION_NOT_IMPLEMENTED();
    }
};

} // namespace Methane::Graphics::Null
