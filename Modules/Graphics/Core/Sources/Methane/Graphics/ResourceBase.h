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

FILE: Methane/Graphics/ResourceBase.h
Base implementation of the resource interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Resource.h>
#include <Methane/Data/Emitter.hpp>

#include "ObjectBase.h"

#include <set>
#include <map>
#include <mutex>

namespace Methane::Graphics
{

class ContextBase;

class ResourceBase
    : public virtual Resource // NOSONAR
    , public ObjectBase
    , public Data::Emitter<IResourceCallback>
{
public:
    ResourceBase(Type type, Usage usage_mask, const ContextBase& context);
    ResourceBase(const ResourceBase&) = delete;
    ResourceBase(ResourceBase&&) = delete;

    // Resource interface
    [[nodiscard]] Type                      GetResourceType() const noexcept final     { return m_type; }
    [[nodiscard]] State                     GetState() const noexcept final            { return m_state;  }
    [[nodiscard]] CommandQueue*             GetOwnerQueue() const noexcept final       { return m_owner_queue_ptr; }
    [[nodiscard]] Usage                     GetUsage() const noexcept final            { return m_usage_mask; }
    [[nodiscard]] const Context&            GetContext() const noexcept final;
    [[nodiscard]] const SubResource::Count& GetSubresourceCount() const noexcept final { return m_sub_resource_count; }
    [[nodiscard]] Data::Size                GetSubResourceDataSize(const SubResource::Index& subresource_index = SubResource::Index()) const final;
    [[nodiscard]] SubResource               GetData(const SubResource::Index& sub_resource_index = SubResource::Index(),
                                                    const std::optional<BytesRange>& data_range = {}) override;
    bool SetState(State state, Ptr<Barriers>& out_barriers) final;
    bool SetState(State state) final;
    bool SetOwnerQueue(CommandQueue& owner_queue) final;
    bool SetOwnerQueue(CommandQueue& owner_queue, Ptr<Barriers>& out_barriers) final;
    void SetData(const SubResources& sub_resources, CommandQueue*) override;

    [[nodiscard]] Ptr<Barriers>& GetSetupTransitionBarriers() noexcept { return m_setup_transition_barriers_ptr; }
    [[nodiscard]] static const std::vector<Resource::Usage>& GetPrimaryUsageValues() noexcept;

protected:
    [[nodiscard]] const ContextBase&   GetContextBase() const noexcept                  { return m_context; }
    [[nodiscard]] Data::Size           GetInitializedDataSize() const noexcept          { return m_initialized_data_size; }

    void  SetSubResourceCount(const SubResource::Count& sub_resource_count);
    void  ValidateSubResource(const SubResource& sub_resource) const;
    void  ValidateSubResource(const SubResource::Index& sub_resource_index, const std::optional<BytesRange>& sub_resource_data_range) const;

    [[nodiscard]] virtual Data::Size CalculateSubResourceDataSize(const SubResource::Index& sub_resource_index) const;

private:
    using SubResourceSizes = std::vector<Data::Size>;
    void FillSubresourceSizes();

    const Type         m_type;
    const Usage        m_usage_mask;
    const ContextBase& m_context;
    State              m_state = State::Common;
    Data::Size         m_initialized_data_size       = 0U;
    bool               m_sub_resource_count_constant = false;
    SubResource::Count m_sub_resource_count;
    SubResourceSizes   m_sub_resource_sizes;
    Ptr<Barriers>      m_setup_transition_barriers_ptr;
    CommandQueue*      m_owner_queue_ptr = nullptr;
    TracyLockable(std::mutex, m_state_mutex)
};

} // namespace Methane::Graphics
