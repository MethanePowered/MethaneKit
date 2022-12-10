/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/RenderContext.h
Methane RenderContext PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RHI/IRenderContext.h>

namespace Methane::Graphics::Rhi
{

class Device;

class RenderContext
{
public:
    using Settings              = RenderContextSettings;
    using Type                  = ContextType;
    using WaitFor               = ContextWaitFor;
    using DeferredAction        = ContextDeferredAction;
    using Option                = ContextOption;
    using OptionMask            = ContextOptionMask;
    using IncompatibleException = ContextIncompatibleException;

    RenderContext() = default;
    RenderContext(const Ptr<IRenderContext>& render_context_ptr);
    RenderContext(const Platform::AppEnvironment& env, const Device& device, tf::Executor& parallel_executor, const Settings& settings);

    void Init(const Platform::AppEnvironment& env, const Device& device, tf::Executor& parallel_executor, const Settings& settings);
    void Release();

    bool IsInitialized() const noexcept;
    IRenderContext& GetInterface() const noexcept;

    bool SetName(const std::string& name) const;
    const std::string& GetName() const noexcept;

private:
    class Impl;

    UniquePtr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi
