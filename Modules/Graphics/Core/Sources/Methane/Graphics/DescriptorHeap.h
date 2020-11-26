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

FILE: Methane/Graphics/DescriptorHeap.h
Descriptor Heap is a platform abstraction of DirectX 12 descriptor heaps

******************************************************************************/

#pragma once

#include "ObjectBase.h"

#include <Methane/Data/RangeSet.hpp>
#include <Methane/Data/Provider.h>
#include <Methane/Data/Emitter.hpp>
#include <Methane/Memory.hpp>
#include <Methane/Instrumentation.h>

#include <set>
#include <mutex>
#include <functional>

namespace Methane::Graphics
{

class ContextBase;
class ResourceBase;
class DescriptorHeap;

struct IDescriptorHeapCallback
{
    virtual void OnDescriptorHeapAllocated(DescriptorHeap& descriptor_heap) = 0;

    virtual ~IDescriptorHeapCallback() = default;
};

class DescriptorHeap : public Data::Emitter<IDescriptorHeapCallback>
{
public:
    enum class Type : uint32_t
    {
        // Shader visible heap types
        ShaderResources = 0U,
        Samplers,

        // Other heap types
        RenderTargets,
        DepthStencil,

        // Always keep at the end
        Undefined
    };

    struct Settings
    {
        Type       type;
        Data::Size size;
        bool       deferred_allocation;
        bool       shader_visible;
    };

    using Types    = std::set<Type>;
    using Range    = Methane::Data::Range<Data::Index>;

    struct Reservation
    {
        Ref<DescriptorHeap> heap;
        Range constant_range;
        Range mutable_range;

        Reservation(const Ref<DescriptorHeap>& in_heap, const Range& in_constant_range, const Range& in_mutable_range);

        const Range& GetRange(bool is_constant) const { return is_constant ? constant_range : mutable_range; }
    };

    static Ptr<DescriptorHeap> Create(ContextBase& context, const Settings& settings);
    ~DescriptorHeap() override;

    // DescriptorHeap interface
    virtual Data::Index AddResource(const ResourceBase& resource);
    virtual Data::Index ReplaceResource(const ResourceBase& resource, Data::Index at_index);
    virtual void        RemoveResource(Data::Index at_index);
    virtual void        Allocate();

    Range               ReserveRange(Data::Size length);
    void                ReleaseRange(const Range& range);

    void                SetDeferredAllocation(bool deferred_allocation);

    const Settings&     GetSettings() const                             { return m_settings; }
    Data::Size          GetDeferredSize() const                         { return m_deferred_size; }
    Data::Size          GetAllocatedSize() const                        { return m_allocated_size; }
    const ResourceBase* GetResource(uint32_t descriptor_index) const    { return m_resources[descriptor_index]; }
    bool                IsShaderVisible() const                         { return m_settings.shader_visible && IsShaderVisibleHeapType(m_settings.type); }

    static bool         IsShaderVisibleHeapType(Type heap_type)         { return heap_type == Type::ShaderResources || heap_type == Type::Samplers; }

protected:
    DescriptorHeap(ContextBase& context, const Settings& settings);

    ContextBase& GetContext() { return m_context; }

private:
    using ResourcePtrs = std::vector<const ResourceBase*>;
    using RangeSet     = Data::RangeSet<Data::Index>;

    ContextBase&              m_context;
    Settings                  m_settings;
    Data::Size                m_deferred_size;
    Data::Size                m_allocated_size = 0;
    ResourcePtrs              m_resources;
    RangeSet                  m_free_ranges;
    TracyLockable(std::mutex, m_modification_mutex);
};

} // namespace Methane::Graphics
