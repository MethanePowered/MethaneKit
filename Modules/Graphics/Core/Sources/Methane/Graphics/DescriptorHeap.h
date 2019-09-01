/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
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

#include <Methane/Data/RangeSet.hpp>

#include <memory>
#include <vector>
#include <set>

namespace Methane
{
namespace Graphics
{

class ContextBase;
class ResourceBase;

class DescriptorHeap
{
public:
    enum class Type : uint32_t
    {
        // Shader visible heap types
        ShaderResources = 0,
        Samplers,

        // Other heap types
        RenderTargets,
        DepthStencil,

        Count,
        Undefined,
    };

    struct Settings
    {
        Type     type;
        uint32_t size;
        bool     deferred_allocation;
        bool     shader_visible;
    };

    using Ref      = std::reference_wrapper<DescriptorHeap>;
    using Types    = std::set<Type>;
    using Index    = uint32_t;
    using Range    = Methane::Data::Range<Index>;
    using RangePtr = std::unique_ptr<Range>;

    struct Reservation
    {
        Ref   heap;
        Range constant_range;
        Range mutable_range;

        Reservation(Ref in_heap, const Range& in_constant_range, const Range& in_mutable_range)
            : heap(in_heap), constant_range(in_constant_range), mutable_range(in_mutable_range)
        { }

        const Range& GetRange(bool is_constant) const { return is_constant ? constant_range : mutable_range; }
    };

    using  Ptr = std::shared_ptr<DescriptorHeap>;
    static Ptr Create(ContextBase& context, const Settings& settings);
    virtual ~DescriptorHeap();

    // DescriptorHeap interface
    virtual int32_t AddResource(const ResourceBase& resource);
    virtual int32_t ReplaceResource(const ResourceBase& resource, uint32_t at_index);
    virtual void    RemoveResource(uint32_t at_index);
    virtual void    Allocate() { m_allocated_size = m_deferred_size; }

    RangePtr ReserveRange(Index length);
    void     ReleaseRange(const Range& range);

    const Settings&     GetSettings() const                             { return m_settings; }
    uint32_t            GetDeferredSize() const                         { return m_deferred_size; }
    uint32_t            GetAllocatedSize() const                        { return m_allocated_size; }
    std::string         GetTypeName() const                             { return GetTypeName(m_settings.type); }
    const ResourceBase* GetResource(uint32_t descriptor_index) const    { return m_resources[descriptor_index]; }
    bool                IsShaderVisible() const                         { return m_settings.shader_visible && IsShaderVisibileHeapType(m_settings.type); }

    static bool         IsShaderVisibileHeapType(Type heap_type)        { return heap_type == Type::ShaderResources || heap_type == Type::Samplers; }
    static std::string  GetTypeName(Type heap_type);

protected:
    DescriptorHeap(ContextBase& context, const Settings& settings);

    using ResourcePtrs = std::vector<const ResourceBase*>;
    using RangeSet     = Methane::Data::RangeSet<Index>;

    ContextBase&    m_context;
    const Settings  m_settings;
    uint32_t        m_deferred_size;
    uint32_t        m_allocated_size = 0;
    ResourcePtrs    m_resources;
    RangeSet        m_free_ranges;
};

using DescriptorHeaps      = std::vector<DescriptorHeap::Ptr>;
using DescriptorHeapRefs   = std::vector<DescriptorHeap::Ref>;

} // namespace Graphics
} // namespace Methane
