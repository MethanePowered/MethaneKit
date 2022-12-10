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

FILE: Methane/Graphics/RHI/RenderCommandList.cpp
Methane RenderCommandList PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/RenderCommandList.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/RenderPass.h>


#if defined METHANE_GFX_DIRECTX

#include <Methane/Graphics/DirectX/RenderCommandList.h>
using RenderCommandListImpl = Methane::Graphics::DirectX::RenderCommandList;

#elif defined METHANE_GFX_VULKAN

#include <Methane/Graphics/Vulkan/RenderCommandList.h>
using RenderCommandListImpl = Methane::Graphics::Vulkan::RenderCommandList;

#elif defined METHANE_GFX_METAL

#include <Methane/Graphics/Metal/RenderCommandList.hh>
using RenderCommandListImpl = Methane::Graphics::Metal::RenderCommandList;

#else // METHAN_GFX_[API] is undefined

static_assert(false, "Static graphics API macro-definition is missing.");

#endif

#include "ImplWrapper.hpp"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

class RenderCommandList::Impl : public ImplWrapper<IRenderCommandList, RenderCommandListImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

RenderCommandList::RenderCommandList(const Ptr<IRenderCommandList>& interface_ptr)
    : m_impl_ptr(std::make_unique<Impl>(interface_ptr))
{
}

RenderCommandList::RenderCommandList(const CommandQueue& command_queue, const RenderPass& render_pass)
    : RenderCommandList(IRenderCommandList::Create(command_queue.GetInterface(), render_pass.GetInterface()))
{
}

void RenderCommandList::Init(const CommandQueue& command_queue, const RenderPass& render_pass)
{
    m_impl_ptr = std::make_unique<Impl>(IRenderCommandList::Create(command_queue.GetInterface(), render_pass.GetInterface()));
}

void RenderCommandList::Release()
{
    m_impl_ptr.release();
}

bool RenderCommandList::IsInitialized() const noexcept
{
    return static_cast<bool>(m_impl_ptr);
}

IRenderCommandList& RenderCommandList::GetInterface() const noexcept
{
    return m_impl_ptr->GetInterface();
}

bool RenderCommandList::SetName(const std::string& name) const
{
    return m_impl_ptr->Get().SetName(name);
}

const std::string& RenderCommandList::GetName() const noexcept
{
    return m_impl_ptr->Get().GetName();
}

} // namespace Methane::Graphics::Rhi
