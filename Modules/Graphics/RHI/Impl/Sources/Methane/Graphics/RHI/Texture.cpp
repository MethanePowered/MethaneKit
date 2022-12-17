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
using ImageTextureImpl        = Methane::Graphics::DirectX::ImageTexture;
using RenderTargetTextureImpl = Methane::Graphics::DirectX::RenderTargetTexture;
using FrameBufferTextureImpl  = Methane::Graphics::DirectX::FrameBufferTexture;
using DepthStencilTextureImpl = Methane::Graphics::DirectX::DepthStencilTexture;

#elif defined METHANE_GFX_VULKAN

#include <Methane/Graphics/Vulkan/Texture.h>
using ImageTextureImpl        = Methane::Graphics::Vulkan::ImageTexture;
using RenderTargetTextureImpl = Methane::Graphics::Vulkan::RenderTargetTexture;
using FrameBufferTextureImpl  = Methane::Graphics::Vulkan::FrameBufferTexture;
using DepthStencilTextureImpl = Methane::Graphics::Vulkan::DepthStencilTexture;

#elif defined METHANE_GFX_METAL

#include <Methane/Graphics/Metal/Texture.hh>
using ImageTextureImpl        = Methane::Graphics::Metal::Texture;
using RenderTargetTextureImpl = Methane::Graphics::Metal::Texture;
using FrameBufferTextureImpl  = Methane::Graphics::Metal::Texture;
using DepthStencilTextureImpl = Methane::Graphics::Metal::Texture;

#else // METHAN_GFX_[API] is undefined

static_assert(false, "Static graphics API macro-definition is missing.");

#endif

#include "ImplWrapper.hpp"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

template<typename ImplType>
bool SetResourceState(const UniquePtr<ImplType>& impl_ptr, ResourceState state, ResourceBarriers& out_barriers)
{
    Ptr<IResourceBarriers> out_barriers_ptr = out_barriers.GetInterface().GetPtr();
    const bool state_changed = GetPrivateImpl(impl_ptr).SetState(state, out_barriers_ptr);
    if (!out_barriers.IsInitialized() && out_barriers_ptr)
    {
        out_barriers = ResourceBarriers(out_barriers_ptr);
    }
    return state_changed;
}

template<typename ImplType>
bool SetResourceOwnerQueueFamily(const UniquePtr<ImplType>& impl_ptr, uint32_t family_index, ResourceBarriers& out_barriers)
{
    Ptr<IResourceBarriers> out_barriers_ptr = out_barriers.GetInterface().GetPtr();
    const bool             state_changed    = GetPrivateImpl(impl_ptr).SetOwnerQueueFamily(family_index, out_barriers_ptr);
    if (!out_barriers.IsInitialized() && out_barriers_ptr)
    {
        out_barriers = ResourceBarriers(out_barriers_ptr);
    }
    return state_changed;
}

//************ ImageTexture ************

class ImageTexture::Impl
    : public ImplWrapper<ITexture, ImageTextureImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(ImageTexture);

ImageTexture::ImageTexture(const Ptr<ITexture>& interface_ptr)
    : m_impl_ptr(std::make_unique<Impl>(interface_ptr))
{
}

ImageTexture::ImageTexture(ITexture& interface_ref)
    : ImageTexture(std::dynamic_pointer_cast<ITexture>(interface_ref.GetPtr()))
{
}

ImageTexture::ImageTexture(const RenderContext& context, const Dimensions& dimensions, const Opt<uint32_t>& array_length_opt, PixelFormat pixel_format, bool mipmapped)
    : ImageTexture(ITexture::CreateImage(context.GetInterface(), dimensions, array_length_opt, pixel_format, mipmapped))
{
}

void ImageTexture::Init(const RenderContext& context, const Dimensions& dimensions, const Opt<uint32_t>& array_length_opt, PixelFormat pixel_format, bool mipmapped)
{
    m_impl_ptr = std::make_unique<Impl>(ITexture::CreateImage(context.GetInterface(), dimensions, array_length_opt, pixel_format, mipmapped));
}

void ImageTexture::Release()
{
    m_impl_ptr.release();
}

bool ImageTexture::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

ITexture& ImageTexture::GetInterface() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterface(m_impl_ptr);
}

bool ImageTexture::SetState(State state) const
{
    return GetPrivateImpl(m_impl_ptr).SetState(state);
}

bool ImageTexture::SetState(State state, Barriers& out_barriers) const
{
    return SetResourceState(m_impl_ptr, state, out_barriers);
}

bool ImageTexture::SetOwnerQueueFamily(uint32_t family_index) const
{
    return GetPrivateImpl(m_impl_ptr).SetOwnerQueueFamily(family_index);
}

bool ImageTexture::SetOwnerQueueFamily(uint32_t family_index, Barriers& out_barriers) const
{
    return SetResourceOwnerQueueFamily(m_impl_ptr, family_index, out_barriers);
}

void ImageTexture::SetData(const SubResources& sub_resources, const CommandQueue& target_cmd_queue) const
{
    GetPrivateImpl(m_impl_ptr).SetData(sub_resources, target_cmd_queue.GetInterface());
}

void ImageTexture::RestoreDescriptorViews(const DescriptorByViewId& descriptor_by_view_id) const
{
    GetPrivateImpl(m_impl_ptr).RestoreDescriptorViews(descriptor_by_view_id);
}

SubResource ImageTexture::GetData(const SubResource::Index& sub_resource_index, const BytesRangeOpt& data_range) const
{
    return GetPrivateImpl(m_impl_ptr).GetData(sub_resource_index, data_range);
}

Data::Size ImageTexture::GetDataSize(Data::MemoryState size_type) const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetDataSize(size_type);
}

Data::Size ImageTexture::GetSubResourceDataSize(const SubResource::Index& sub_resource_index) const
{
    return GetPrivateImpl(m_impl_ptr).GetSubResourceDataSize(sub_resource_index);
}

const SubResource::Count& ImageTexture::GetSubresourceCount() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetSubresourceCount();
}

ResourceType ImageTexture::GetResourceType() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetResourceType();
}

ResourceState ImageTexture::GetState() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetState();
}

ResourceUsageMask ImageTexture::GetUsage() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetUsage();
}

const ImageTexture::DescriptorByViewId& ImageTexture::GetDescriptorByViewId() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetDescriptorByViewId();
}

RenderContext ImageTexture::GetRenderContext() const
{
    IContext& context = const_cast<IContext&>(GetPrivateImpl(m_impl_ptr).GetContext()); // NOSONAR
    META_CHECK_ARG_EQUAL(context.GetType(), ContextType::Render);
    return RenderContext(dynamic_cast<IRenderContext&>(context));
}

const Opt<uint32_t>& ImageTexture::GetOwnerQueueFamily() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetOwnerQueueFamily();
}

const ImageTexture::Settings& ImageTexture::GetSettings() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetSettings();
}

//************ RenderTargetTexture ************

class RenderTargetTexture::Impl
    : public ImplWrapper<ITexture, RenderTargetTextureImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(RenderTargetTexture);

RenderTargetTexture::RenderTargetTexture(const Ptr<ITexture>& interface_ptr)
    : m_impl_ptr(std::make_unique<Impl>(interface_ptr))
{
}

RenderTargetTexture::RenderTargetTexture(ITexture& interface_ref)
    : RenderTargetTexture(std::dynamic_pointer_cast<ITexture>(interface_ref.GetPtr()))
{
}

RenderTargetTexture::RenderTargetTexture(const RenderContext& context, const Settings& settings)
    : RenderTargetTexture(ITexture::CreateRenderTarget(context.GetInterface(), settings))
{
}

void RenderTargetTexture::Init(const RenderContext& context, const Settings& settings)
{
    m_impl_ptr = std::make_unique<Impl>(ITexture::CreateRenderTarget(context.GetInterface(), settings));
}

void RenderTargetTexture::Release()
{
    m_impl_ptr.release();
}

bool RenderTargetTexture::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

ITexture& RenderTargetTexture::GetInterface() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterface(m_impl_ptr);
}

bool RenderTargetTexture::SetState(State state) const
{
    return GetPrivateImpl(m_impl_ptr).SetState(state);
}

bool RenderTargetTexture::SetState(State state, Barriers& out_barriers) const
{
    return SetResourceState(m_impl_ptr, state, out_barriers);
}

bool RenderTargetTexture::SetOwnerQueueFamily(uint32_t family_index) const
{
    return GetPrivateImpl(m_impl_ptr).SetOwnerQueueFamily(family_index);
}

bool RenderTargetTexture::SetOwnerQueueFamily(uint32_t family_index, Barriers& out_barriers) const
{
    return SetResourceOwnerQueueFamily(m_impl_ptr, family_index, out_barriers);
}

void RenderTargetTexture::RestoreDescriptorViews(const DescriptorByViewId& descriptor_by_view_id) const
{
    GetPrivateImpl(m_impl_ptr).RestoreDescriptorViews(descriptor_by_view_id);
}

SubResource RenderTargetTexture::GetData(const SubResource::Index& sub_resource_index, const BytesRangeOpt& data_range) const
{
    return GetPrivateImpl(m_impl_ptr).GetData(sub_resource_index, data_range);
}

Data::Size RenderTargetTexture::GetDataSize(Data::MemoryState size_type) const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetDataSize(size_type);
}

Data::Size RenderTargetTexture::GetSubResourceDataSize(const SubResource::Index& sub_resource_index) const
{
    return GetPrivateImpl(m_impl_ptr).GetSubResourceDataSize(sub_resource_index);
}

const SubResource::Count& RenderTargetTexture::GetSubresourceCount() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetSubresourceCount();
}

ResourceType RenderTargetTexture::GetResourceType() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetResourceType();
}

ResourceState RenderTargetTexture::GetState() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetState();
}

ResourceUsageMask RenderTargetTexture::GetUsage() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetUsage();
}

const RenderTargetTexture::DescriptorByViewId& RenderTargetTexture::GetDescriptorByViewId() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetDescriptorByViewId();
}

RenderContext RenderTargetTexture::GetRenderContext() const
{
    IContext& context = const_cast<IContext&>(GetPrivateImpl(m_impl_ptr).GetContext()); // NOSONAR
    META_CHECK_ARG_EQUAL(context.GetType(), ContextType::Render);
    return RenderContext(dynamic_cast<IRenderContext&>(context));
}

const Opt<uint32_t>& RenderTargetTexture::GetOwnerQueueFamily() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetOwnerQueueFamily();
}

const RenderTargetTexture::Settings& RenderTargetTexture::GetSettings() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetSettings();
}

//************ FrameBufferTexture ************

class FrameBufferTexture::Impl
    : public ImplWrapper<ITexture, FrameBufferTextureImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(FrameBufferTexture);

FrameBufferTexture::FrameBufferTexture(const Ptr<ITexture>& interface_ptr)
    : m_impl_ptr(std::make_unique<Impl>(interface_ptr))
{
}

FrameBufferTexture::FrameBufferTexture(ITexture& interface_ref)
    : FrameBufferTexture(std::dynamic_pointer_cast<ITexture>(interface_ref.GetPtr()))
{
}

FrameBufferTexture::FrameBufferTexture(const RenderContext& context, FrameBufferIndex frame_buffer_index)
    : FrameBufferTexture(ITexture::CreateFrameBuffer(context.GetInterface(), frame_buffer_index))
{
}

void FrameBufferTexture::Init(const RenderContext& context, FrameBufferIndex frame_buffer_index)
{
    m_impl_ptr = std::make_unique<Impl>(ITexture::CreateFrameBuffer(context.GetInterface(), frame_buffer_index));
}

void FrameBufferTexture::Release()
{
    m_impl_ptr.release();
}

bool FrameBufferTexture::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

ITexture& FrameBufferTexture::GetInterface() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterface(m_impl_ptr);
}

bool FrameBufferTexture::SetState(State state) const
{
    return GetPrivateImpl(m_impl_ptr).SetState(state);
}

bool FrameBufferTexture::SetState(State state, Barriers& out_barriers) const
{
    return SetResourceState(m_impl_ptr, state, out_barriers);
}

bool FrameBufferTexture::SetOwnerQueueFamily(uint32_t family_index) const
{
    return GetPrivateImpl(m_impl_ptr).SetOwnerQueueFamily(family_index);
}

bool FrameBufferTexture::SetOwnerQueueFamily(uint32_t family_index, Barriers& out_barriers) const
{
    return SetResourceOwnerQueueFamily(m_impl_ptr, family_index, out_barriers);
}

void FrameBufferTexture::RestoreDescriptorViews(const DescriptorByViewId& descriptor_by_view_id) const
{
    GetPrivateImpl(m_impl_ptr).RestoreDescriptorViews(descriptor_by_view_id);
}

SubResource FrameBufferTexture::GetData(const SubResource::Index& sub_resource_index, const BytesRangeOpt& data_range) const
{
    return GetPrivateImpl(m_impl_ptr).GetData(sub_resource_index, data_range);
}

Data::Size FrameBufferTexture::GetDataSize(Data::MemoryState size_type) const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetDataSize(size_type);
}

Data::Size FrameBufferTexture::GetSubResourceDataSize(const SubResource::Index& sub_resource_index) const
{
    return GetPrivateImpl(m_impl_ptr).GetSubResourceDataSize(sub_resource_index);
}

const SubResource::Count& FrameBufferTexture::GetSubresourceCount() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetSubresourceCount();
}

ResourceType FrameBufferTexture::GetResourceType() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetResourceType();
}

ResourceState FrameBufferTexture::GetState() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetState();
}

ResourceUsageMask FrameBufferTexture::GetUsage() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetUsage();
}

const FrameBufferTexture::DescriptorByViewId& FrameBufferTexture::GetDescriptorByViewId() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetDescriptorByViewId();
}

RenderContext FrameBufferTexture::GetRenderContext() const
{
    IContext& context = const_cast<IContext&>(GetPrivateImpl(m_impl_ptr).GetContext()); // NOSONAR
    META_CHECK_ARG_EQUAL(context.GetType(), ContextType::Render);
    return RenderContext(dynamic_cast<IRenderContext&>(context));
}

const Opt<uint32_t>& FrameBufferTexture::GetOwnerQueueFamily() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetOwnerQueueFamily();
}

const FrameBufferTexture::Settings& FrameBufferTexture::GetSettings() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetSettings();
}

//************ DepthStencilTexture ************

class DepthStencilTexture::Impl
    : public ImplWrapper<ITexture, DepthStencilTextureImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(DepthStencilTexture);

DepthStencilTexture::DepthStencilTexture(const Ptr<ITexture>& interface_ptr)
    : m_impl_ptr(std::make_unique<Impl>(interface_ptr))
{
}

DepthStencilTexture::DepthStencilTexture(ITexture& interface_ref)
    : DepthStencilTexture(std::dynamic_pointer_cast<ITexture>(interface_ref.GetPtr()))
{
}

DepthStencilTexture::DepthStencilTexture(const RenderContext& context)
    : DepthStencilTexture(ITexture::CreateDepthStencilBuffer(context.GetInterface()))
{
}

void DepthStencilTexture::Init(const RenderContext& context)
{
    m_impl_ptr = std::make_unique<Impl>(ITexture::CreateDepthStencilBuffer(context.GetInterface()));
}

void DepthStencilTexture::Release()
{
    m_impl_ptr.release();
}

bool DepthStencilTexture::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

ITexture& DepthStencilTexture::GetInterface() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterface(m_impl_ptr);
}

bool DepthStencilTexture::SetState(State state) const
{
    return GetPrivateImpl(m_impl_ptr).SetState(state);
}

bool DepthStencilTexture::SetState(State state, Barriers& out_barriers) const
{
    return SetResourceState(m_impl_ptr, state, out_barriers);
}

bool DepthStencilTexture::SetOwnerQueueFamily(uint32_t family_index) const
{
    return GetPrivateImpl(m_impl_ptr).SetOwnerQueueFamily(family_index);
}

bool DepthStencilTexture::SetOwnerQueueFamily(uint32_t family_index, Barriers& out_barriers) const
{
    return SetResourceOwnerQueueFamily(m_impl_ptr, family_index, out_barriers);
}

void DepthStencilTexture::RestoreDescriptorViews(const DescriptorByViewId& descriptor_by_view_id) const
{
    GetPrivateImpl(m_impl_ptr).RestoreDescriptorViews(descriptor_by_view_id);
}

SubResource DepthStencilTexture::GetData(const SubResource::Index& sub_resource_index, const BytesRangeOpt& data_range) const
{
    return GetPrivateImpl(m_impl_ptr).GetData(sub_resource_index, data_range);
}

Data::Size DepthStencilTexture::GetDataSize(Data::MemoryState size_type) const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetDataSize(size_type);
}

Data::Size DepthStencilTexture::GetSubResourceDataSize(const SubResource::Index& sub_resource_index) const
{
    return GetPrivateImpl(m_impl_ptr).GetSubResourceDataSize(sub_resource_index);
}

const SubResource::Count& DepthStencilTexture::GetSubresourceCount() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetSubresourceCount();
}

ResourceType DepthStencilTexture::GetResourceType() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetResourceType();
}

ResourceState DepthStencilTexture::GetState() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetState();
}

ResourceUsageMask DepthStencilTexture::GetUsage() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetUsage();
}

const DepthStencilTexture::DescriptorByViewId& DepthStencilTexture::GetDescriptorByViewId() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetDescriptorByViewId();
}

RenderContext DepthStencilTexture::GetRenderContext() const
{
    IContext& context = const_cast<IContext&>(GetPrivateImpl(m_impl_ptr).GetContext()); // NOSONAR
    META_CHECK_ARG_EQUAL(context.GetType(), ContextType::Render);
    return RenderContext(dynamic_cast<IRenderContext&>(context));
}

const Opt<uint32_t>& DepthStencilTexture::GetOwnerQueueFamily() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetOwnerQueueFamily();
}

const DepthStencilTexture::Settings& DepthStencilTexture::GetSettings() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetSettings();
}

} // namespace Methane::Graphics::Rhi
