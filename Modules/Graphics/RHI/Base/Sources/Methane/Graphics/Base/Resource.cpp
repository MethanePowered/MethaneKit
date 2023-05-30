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

FILE: Methane/Graphics/Base/Resource.cpp
Base implementation of the resource interface.

******************************************************************************/

#include <Methane/Graphics/Base/Resource.h>
#include <Methane/Graphics/Base/Texture.h>
#include <Methane/Graphics/Base/Context.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <utility>
#include <algorithm>

namespace Methane::Graphics::Base
{

Resource::Resource(const Context& context, Type type, UsageMask usage_mask,
                   State initial_state, Opt<State> auto_transition_source_state_opt)
    : m_context(context)
    , m_type(type)
    , m_usage_mask(usage_mask)
    , m_state(initial_state)
    , m_auto_transition_source_state_opt(auto_transition_source_state_opt)
{ }

const Rhi::IContext& Resource::GetContext() const noexcept
{
    META_FUNCTION_TASK();
    return m_context;
}

bool Resource::SetState(State state, Ptr<IBarriers>& out_barriers)
{
    META_FUNCTION_TASK();
    if (!m_is_state_change_updates_barriers)
        Resource::SetState(state);

    std::scoped_lock lock_guard(m_state_mutex);
    if (m_state == state)
    {
        if (out_barriers)
            out_barriers->RemoveStateTransition(*this);

        return false;
    }

    META_LOG("{} resource '{}' state changed from {} to {} with barrier update",
             magic_enum::enum_name(GetResourceType()), GetName(),
             magic_enum::enum_name(m_state), magic_enum::enum_name(state));

    if (m_state != m_auto_transition_source_state_opt)
    {
        if (!out_barriers)
            out_barriers = Rhi::IResourceBarriers::Create();

        out_barriers->AddStateTransition(*this, m_state, state);
    }

    m_state = state;
    return true;
}

bool Resource::SetState(State state)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_state_mutex);
    if (m_state == state)
        return false;

    META_LOG("{} resource '{}' state changed from {} to {}",
             magic_enum::enum_name(GetResourceType()), GetName(),
             magic_enum::enum_name(m_state), magic_enum::enum_name(state));

    m_state = state;
    return true;
}

bool Resource::SetOwnerQueueFamily(uint32_t family_index, Ptr<IBarriers>& out_barriers)
{
    META_FUNCTION_TASK();
    if (m_owner_queue_family_index_opt == family_index)
    {
        if (out_barriers)
            out_barriers->RemoveOwnerTransition(*this);
        return false;
    }

    META_LOG("{} resource '{}' owner queue changed from {} to {} queue family {} barrier update",
             magic_enum::enum_name(GetResourceType()), GetName(),
             m_owner_queue_family_index_opt ? std::to_string(*m_owner_queue_family_index_opt) : "n/a",
             family_index,
             m_owner_queue_family_index_opt ? "with" : "without");

    if (m_owner_queue_family_index_opt)
    {
        if (!out_barriers)
            out_barriers = Rhi::IResourceBarriers::Create();

        out_barriers->AddOwnerTransition(*this, *m_owner_queue_family_index_opt, family_index);
    }

    m_owner_queue_family_index_opt = family_index;
    return true;
}

bool Resource::SetOwnerQueueFamily(uint32_t family_index)
{
    META_FUNCTION_TASK();
    if (m_owner_queue_family_index_opt == family_index)
        return false;

    META_LOG("{} resource '{}' owner queue changed from {} to {} queue family",
             magic_enum::enum_name(GetResourceType()), GetName(),
             m_owner_queue_family_index_opt ? std::to_string(*m_owner_queue_family_index_opt) : "n/a",
             family_index);

    m_owner_queue_family_index_opt = family_index;
    return true;
}

} // namespace Methane::Graphics::Base
