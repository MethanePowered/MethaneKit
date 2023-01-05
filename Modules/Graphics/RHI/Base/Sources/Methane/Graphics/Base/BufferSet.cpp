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

FILE: Methane/Graphics/Base/BufferSet.cpp
Base implementation of the buffer-set interface.

******************************************************************************/

#include <Methane/Graphics/Base/BufferSet.h>

#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>
#include <sstream>

namespace Methane::Graphics::Base
{

BufferSet::BufferSet(Rhi::BufferType buffers_type, const Refs<Rhi::IBuffer>& buffer_refs)
    : m_buffers_type(buffers_type)
    , m_refs(buffer_refs)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_EMPTY_DESCR(buffer_refs, "empty buffers set is not allowed");

    m_ptrs.reserve(m_refs.size());
    m_raw_ptrs.reserve(m_refs.size());
    for(const Ref<Rhi::IBuffer>& buffer_ref : m_refs)
    {
        META_CHECK_ARG_EQUAL_DESCR(buffer_ref.get().GetSettings().type, m_buffers_type,
                                   "All buffers must be of the same type '{}'", magic_enum::enum_name(m_buffers_type));
        auto& buffer_base = static_cast<Buffer&>(buffer_ref.get());
        m_ptrs.emplace_back(buffer_base.GetPtr<Buffer>());
        m_raw_ptrs.emplace_back(std::addressof(buffer_base));
    }
}

std::string BufferSet::GetNames() const noexcept
{
    META_FUNCTION_TASK();
    std::stringstream ss;
    bool is_empty = true;
    for (const Ref<Rhi::IBuffer>& buffer_ref : m_refs)
    {
        if (!is_empty)
            ss << ", ";
        ss << "'" << buffer_ref.get().GetName() << "'";
        is_empty = false;
    }
    return ss.str();
}

Rhi::IBuffer& BufferSet::operator[](Data::Index index) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS(index, m_refs.size());

    return m_refs[index].get();
}

bool BufferSet::SetState(Rhi::ResourceState state)
{
    META_FUNCTION_TASK();
    bool state_changed = false;
    for(const Ref<Rhi::IBuffer>& buffer_ref : m_refs)
    {
        state_changed |= static_cast<Buffer&>(buffer_ref.get()).SetState(state, m_setup_transition_barriers);
    }
    return state_changed;
}

} // namespace Methane::Graphics::Base