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
#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/ResourceBarriers.h>

#include <Methane/Pimpl.hpp>

#ifdef META_GFX_METAL
#include <Texture.hh>
#else
#include <Texture.h>
#endif

namespace Methane::Graphics::Rhi
{

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(Texture);
META_PIMPL_METHODS_COMPARE_IMPLEMENT(Texture);

Texture::Texture(const Ptr<ITexture>& interface_ptr)
    : m_impl_ptr(std::dynamic_pointer_cast<Impl>(interface_ptr))
{
}

Texture::Texture(ITexture& interface_ref)
    : Texture(interface_ref.GetDerivedPtr<ITexture>())
{
}

Texture::Texture(const IContext& context, const Settings& settings)
    : Texture(ITexture::Create(context, settings))
{
}

Texture::Texture(const RenderContext& render_context, const Settings& settings)
    : Texture(render_context.GetInterface(), settings)
{
}

Texture::Texture(const ComputeContext& compute_context, const Settings& settings)
    : Texture(compute_context.GetInterface(), settings)
{
}

bool Texture::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

ITexture& Texture::GetInterface() const META_PIMPL_NOEXCEPT
{
    return *m_impl_ptr;
}

Ptr<ITexture> Texture::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return m_impl_ptr;
}

bool Texture::SetName(std::string_view name) const
{
    return GetImpl(m_impl_ptr).SetName(name);
}

std::string_view Texture::GetName() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetName();
}

void Texture::Connect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Connect(receiver);
}

void Texture::Disconnect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Disconnect(receiver);
}

bool Texture::SetState(State state) const
{
    return GetImpl(m_impl_ptr).SetState(state);
}

bool Texture::SetState(State state, Barriers& out_barriers) const
{
    Ptr<IResourceBarriers> out_barriers_ptr = out_barriers.GetInterfacePtr();
    const bool state_changed = GetImpl(m_impl_ptr).SetState(state, out_barriers_ptr);
    if (!out_barriers.IsInitialized() && out_barriers_ptr)
    {
        out_barriers = ResourceBarriers(out_barriers_ptr);
    }
    return state_changed;
}

bool Texture::SetOwnerQueueFamily(uint32_t family_index) const
{
    return GetImpl(m_impl_ptr).SetOwnerQueueFamily(family_index);
}

bool Texture::SetOwnerQueueFamily(uint32_t family_index, Barriers& out_barriers) const
{
    Ptr<IResourceBarriers> out_barriers_ptr = out_barriers.GetInterfacePtr();
    const bool             state_changed    = GetImpl(m_impl_ptr).SetOwnerQueueFamily(family_index, out_barriers_ptr);
    if (!out_barriers.IsInitialized() && out_barriers_ptr)
    {
        out_barriers = ResourceBarriers(out_barriers_ptr);
    }
    return state_changed;
}

void Texture::SetData(const CommandQueue& target_cmd_queue, const SubResources& sub_resources) const
{
    GetImpl(m_impl_ptr).SetData(target_cmd_queue.GetInterface(), sub_resources);
}

void Texture::RestoreDescriptorViews(const DescriptorByViewId& descriptor_by_view_id) const
{
    GetImpl(m_impl_ptr).RestoreDescriptorViews(descriptor_by_view_id);
}

SubResource Texture::GetData(const CommandQueue& target_cmd_queue, const SubResource::Index& sub_resource_index, const BytesRangeOpt& data_range) const
{
    return GetImpl(m_impl_ptr).GetData(target_cmd_queue.GetInterface(), sub_resource_index, data_range);
}

Data::Size Texture::GetDataSize(Data::MemoryState size_type) const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetDataSize(size_type);
}

Data::Size Texture::GetSubResourceDataSize(const SubResource::Index& sub_resource_index) const
{
    return GetImpl(m_impl_ptr).GetSubResourceDataSize(sub_resource_index);
}

SubResource::Count Texture::GetSubresourceCount() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetSubresourceCount();
}

ResourceType Texture::GetResourceType() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetResourceType();
}

ResourceState Texture::GetState() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetState();
}

ResourceUsageMask Texture::GetUsage() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetUsage();
}

const Texture::DescriptorByViewId& Texture::GetDescriptorByViewId() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetDescriptorByViewId();
}

const IContext& Texture::GetContext() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetContext();
}

const Opt<uint32_t>& Texture::GetOwnerQueueFamily() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetOwnerQueueFamily();
}

Rhi::ResourceView Texture::GetTextureView(const SubResource::Index& subresource_index,
                                          const SubResource::Count& subresource_count,
                                          Opt<Rhi::TextureDimensionType> texture_dimension_type_opt) const
{
    return GetImpl(m_impl_ptr).GetTextureView(subresource_index, subresource_count, texture_dimension_type_opt);
}

Rhi::ResourceView Texture::GetResourceView() const
{
    return GetImpl(m_impl_ptr).GetResourceView();
}

void Texture::Connect(Data::Receiver<IResourceCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IResourceCallback>::Connect(receiver);
}

void Texture::Disconnect(Data::Receiver<IResourceCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IResourceCallback>::Disconnect(receiver);
}

const Texture::Settings& Texture::GetSettings() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetSettings();
}

} // namespace Methane::Graphics::Rhi
