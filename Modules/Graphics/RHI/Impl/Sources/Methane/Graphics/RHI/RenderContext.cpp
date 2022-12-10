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

FILE: Methane/Graphics/RHI/RenderContext.cpp
Methane RenderContext PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/RenderContext.h>
#include <Methane/Graphics/RHI/Device.h>

#if defined METHANE_GFX_DIRECTX

#include <Methane/Graphics/DirectX/RenderContext.h>
using RenderContextImpl = Methane::Graphics::DirectX::RenderContext;

#elif defined METHANE_GFX_VULKAN

#include <Methane/Graphics/Vulkan/RenderContext.h>
using RenderContextImpl = Methane::Graphics::Vulkan::RenderContext;

#elif defined METHANE_GFX_METAL

#include <Methane/Graphics/Metal/RenderContext.hh>
using RenderContextImpl = Methane::Graphics::Metal::RenderContext;

#else // METHAN_GFX_[API] is undefined

static_assert(false, "Static graphics API macro-definition is missing.");

#endif

#include <Methane/Instrumentation.h>

#include "ImplWrapper.hpp"

namespace Methane::Graphics::Rhi
{

class RenderContext::Impl : public ImplWrapper<IRenderContext, RenderContextImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

RenderContext::RenderContext(const Ptr <IRenderContext>& interface_ptr)
    : m_impl_ptr(std::make_unique<Impl>(interface_ptr))
{
}

RenderContext::RenderContext(const Platform::AppEnvironment& env, const Device& device, tf::Executor& parallel_executor, const Settings& settings)
    : RenderContext(IRenderContext::Create(env, device.GetInterface(), parallel_executor, settings))
{
}

void RenderContext::Init(const Platform::AppEnvironment& env, const Device& device, tf::Executor& parallel_executor, const Settings& settings)
{
    m_impl_ptr = std::make_unique<Impl>(IRenderContext::Create(env, device.GetInterface(), parallel_executor, settings));
}

void RenderContext::Release()
{
    m_impl_ptr.release();
}

bool RenderContext::IsInitialized() const noexcept
{
    return static_cast<bool>(m_impl_ptr);
}

IRenderContext& RenderContext::GetInterface() const noexcept
{
    return m_impl_ptr->GetInterface();
}

bool RenderContext::SetName(const std::string& name) const
{
    return m_impl_ptr->Get().SetName(name);
}

const std::string& RenderContext::GetName() const noexcept
{
    return m_impl_ptr->Get().GetName();
}

} // namespace Methane::Graphics::Rhi
