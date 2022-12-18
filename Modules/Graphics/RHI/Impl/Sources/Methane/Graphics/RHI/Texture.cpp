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

FILE: Methane/Graphics/RHI/Texture.cpp
Methane Texture PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/Texture.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/RenderContext.h>
#include <Methane/Graphics/RHI/ResourceBarriers.h>

#if defined METHANE_GFX_DIRECTX

#include <Methane/Graphics/DirectX/Texture.h>
using TextureImpl = Methane::Graphics::DirectX::Texture;

#elif defined METHANE_GFX_VULKAN

#include <Methane/Graphics/Vulkan/Texture.h>
using TextureImpl = Methane::Graphics::Vulkan::Texture;

#elif defined METHANE_GFX_METAL

#include <Methane/Graphics/Metal/Texture.hh>
using TextureImpl = Methane::Graphics::Metal::Texture;

#else // METHAN_GFX_[API] is undefined

static_assert(false, "Static graphics API macro-definition is missing.");

#endif

#include "ImplWrapper.hpp"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

class Texture::Impl
    : public ImplWrapper<ITexture, TextureImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(Texture);

Texture::Texture(const Ptr<ITexture>& interface_ptr)
    : m_impl_ptr(std::make_unique<Impl>(interface_ptr))
{
}

Texture::Texture(ITexture& interface_ref)
    : Texture(std::dynamic_pointer_cast<ITexture>(interface_ref.GetPtr()))
{
}

Texture::Texture(const RenderContext& context, const Settings& settings)
    : Texture(ITexture::Create(context.GetInterface(), settings))
{
}

void Texture::Init(const RenderContext& context, const Settings& settings)
{
    m_impl_ptr = std::make_unique<Impl>(ITexture::Create(context.GetInterface(), settings));
}

void Texture::InitImage(const RenderContext& context, const Dimensions& dimensions, const Opt<uint32_t>& array_length_opt, PixelFormat pixel_format, bool mipmapped)
{
    m_impl_ptr = std::make_unique<Impl>(ITexture::CreateImage(context.GetInterface(), dimensions, array_length_opt, pixel_format, mipmapped));
}

void Texture::InitFrameBuffer(const RenderContext& context, Data::Index frame_index)
{
    m_impl_ptr = std::make_unique<Impl>(ITexture::CreateFrameBuffer(context.GetInterface(), frame_index));
}

void Texture::InitDepthStencil(const RenderContext& context)
{
    m_impl_ptr = std::make_unique<Impl>(ITexture::CreateDepthStencil(context.GetInterface()));
}

void Texture::Release()
{
    m_impl_ptr.release();
}

bool Texture::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

ITexture& Texture::GetInterface() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterface(m_impl_ptr);
}

bool Texture::SetState(State state) const
{
    return GetPrivateImpl(m_impl_ptr).SetState(state);
}

bool Texture::SetState(State state, Barriers& out_barriers) const
{
    Ptr<IResourceBarriers> out_barriers_ptr = out_barriers.GetInterface().GetPtr();
    const bool state_changed = GetPrivateImpl(m_impl_ptr).SetState(state, out_barriers_ptr);
    if (!out_barriers.IsInitialized() && out_barriers_ptr)
    {
        out_barriers = ResourceBarriers(out_barriers_ptr);
    }
    return state_changed;
}

bool Texture::SetOwnerQueueFamily(uint32_t family_index) const
{
    return GetPrivateImpl(m_impl_ptr).SetOwnerQueueFamily(family_index);
}

bool Texture::SetOwnerQueueFamily(uint32_t family_index, Barriers& out_barriers) const
{
    Ptr<IResourceBarriers> out_barriers_ptr = out_barriers.GetInterface().GetPtr();
    const bool             state_changed    = GetPrivateImpl(m_impl_ptr).SetOwnerQueueFamily(family_index, out_barriers_ptr);
    if (!out_barriers.IsInitialized() && out_barriers_ptr)
    {
        out_barriers = ResourceBarriers(out_barriers_ptr);
    }
    return state_changed;
}

void Texture::SetData(const SubResources& sub_resources, const CommandQueue& target_cmd_queue) const
{
    GetPrivateImpl(m_impl_ptr).SetData(sub_resources, target_cmd_queue.GetInterface());
}

void Texture::RestoreDescriptorViews(const DescriptorByViewId& descriptor_by_view_id) const
{
    GetPrivateImpl(m_impl_ptr).RestoreDescriptorViews(descriptor_by_view_id);
}

SubResource Texture::GetData(const SubResource::Index& sub_resource_index, const BytesRangeOpt& data_range) const
{
    return GetPrivateImpl(m_impl_ptr).GetData(sub_resource_index, data_range);
}

Data::Size Texture::GetDataSize(Data::MemoryState size_type) const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetDataSize(size_type);
}

Data::Size Texture::GetSubResourceDataSize(const SubResource::Index& sub_resource_index) const
{
    return GetPrivateImpl(m_impl_ptr).GetSubResourceDataSize(sub_resource_index);
}

const SubResource::Count& Texture::GetSubresourceCount() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetSubresourceCount();
}

ResourceType Texture::GetResourceType() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetResourceType();
}

ResourceState Texture::GetState() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetState();
}

ResourceUsageMask Texture::GetUsage() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetUsage();
}

const Texture::DescriptorByViewId& Texture::GetDescriptorByViewId() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetDescriptorByViewId();
}

RenderContext Texture::GetRenderContext() const
{
    IContext& context = const_cast<IContext&>(GetPrivateImpl(m_impl_ptr).GetContext()); // NOSONAR
    META_CHECK_ARG_EQUAL(context.GetType(), ContextType::Render);
    return RenderContext(dynamic_cast<IRenderContext&>(context));
}

const Opt<uint32_t>& Texture::GetOwnerQueueFamily() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetOwnerQueueFamily();
}

const Texture::Settings& Texture::GetSettings() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetSettings();
}

} // namespace Methane::Graphics::Rhi
