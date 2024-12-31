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

FILE: Methane/Graphics/RHI/Texture.h
Methane Texture PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include <Methane/Pimpl.h>

#include <Methane/Graphics/RHI/ITexture.h>

#include <vector>

namespace Methane::Graphics::META_GFX_NAME
{
class Texture;
}

namespace Methane::Graphics::Rhi
{

class RenderContext;
class ComputeContext;
class ResourceBarriers;
class CommandQueue;

class Texture // NOSONAR - class has more than 35 methods, constructors and assignment operators are required to use forward declared Impl and Ptr<Impl> in header
{
public:
    using AllocationError  = ResourceAllocationError;
    using State            = ResourceState;
    using Barrier          = ResourceBarrier;
    using Barriers         = ResourceBarriers;
    using Type             = TextureType;
    using DimensionType    = TextureDimensionType;
    using View             = TextureView;
    using Views            = TextureViews;
    using Settings         = TextureSettings;

    using Descriptor         = DirectX::ResourceDescriptor;
    using DescriptorByViewId = std::map<ResourceView::Id, Descriptor>;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(Texture);
    META_PIMPL_METHODS_COMPARE_DECLARE(Texture);

    META_PIMPL_API explicit Texture(const Ptr<ITexture>& interface_ptr);
    META_PIMPL_API explicit Texture(ITexture& interface_ref);
    META_PIMPL_API Texture(const IContext& context, const Settings& settings);
    META_PIMPL_API Texture(const RenderContext& render_context, const Settings& settings);
    META_PIMPL_API Texture(const ComputeContext& compute_context, const Settings& settings);

    META_PIMPL_API bool IsInitialized() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API ITexture& GetInterface() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API Ptr<ITexture> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    META_PIMPL_API bool SetName(std::string_view name) const;
    META_PIMPL_API std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // Data::IEmitter<IObjectCallback> interface methods
    META_PIMPL_API void Connect(Data::Receiver<IObjectCallback>& receiver) const;
    META_PIMPL_API void Disconnect(Data::Receiver<IObjectCallback>& receiver) const;

    // IResource interface methods
    META_PIMPL_API bool SetState(State state) const;
    META_PIMPL_API bool SetState(State state, Barriers& out_barriers) const;
    META_PIMPL_API bool SetOwnerQueueFamily(uint32_t family_index) const;
    META_PIMPL_API bool SetOwnerQueueFamily(uint32_t family_index, Barriers& out_barriers) const;
    META_PIMPL_API void RestoreDescriptorViews(const DescriptorByViewId& descriptor_by_view_id) const;

    [[nodiscard]] META_PIMPL_API Data::Size                GetDataSize(Data::MemoryState size_type = Data::MemoryState::Reserved) const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API Data::Size                GetSubResourceDataSize(const SubResource::Index& sub_resource_index = SubResource::Index()) const;
    [[nodiscard]] META_PIMPL_API SubResource::Count        GetSubresourceCount() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API ResourceType              GetResourceType() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API State                     GetState() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API ResourceUsageMask         GetUsage() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API const DescriptorByViewId& GetDescriptorByViewId() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API const IContext&           GetContext() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API const Opt<uint32_t>&      GetOwnerQueueFamily() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API Rhi::ResourceView         GetTextureView(const SubResource::Index& subresource_index,
                                                                          const SubResource::Count& subresource_count = {},
                                                                          Opt<Rhi::TextureDimensionType> texture_dimension_type_opt = std::nullopt) const;
    [[nodiscard]] META_PIMPL_API Rhi::ResourceView         GetResourceView() const;

    // Data::IEmitter<IResourceCallback> interface methods
    META_PIMPL_API void Connect(Data::Receiver<IResourceCallback>& receiver) const;
    META_PIMPL_API void Disconnect(Data::Receiver<IResourceCallback>& receiver) const;

    // ITexture interface methods
    [[nodiscard]] META_PIMPL_API const Settings& GetSettings() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API SubResource GetData(const CommandQueue& target_cmd_queue,
                                                     const SubResource::Index& sub_resource_index = SubResource::Index(),
                                                     const BytesRangeOpt& data_range = {}) const;
    META_PIMPL_API void SetData(const CommandQueue& target_cmd_queue, const SubResources& sub_resources) const;
    
private:
    using Impl = Methane::Graphics::META_GFX_NAME::Texture;

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_PIMPL_INLINE

#include <Methane/Graphics/RHI/Texture.cpp>

#endif // META_PIMPL_INLINE
