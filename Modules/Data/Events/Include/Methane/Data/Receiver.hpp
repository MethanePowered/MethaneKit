/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/Data/Receiver.hpp
Event receiver base template class implementation.

******************************************************************************/

#pragma once

#include "IEmitter.h"

#include <Methane/Memory.hpp>
#include <Methane/Instrumentation.h>

#include <functional>
#include <algorithm>

namespace Methane::Data
{

template<class EventType>
class Receiver : public EventType
{
public:
    Receiver() = default;
    Receiver(const Receiver& other)
        : m_connected_emitter_refs(other.m_connected_emitter_refs)
    {
        META_FUNCTION_TASK();
        for(const Ref<IEmitter<EventType>>& connected_emitter_ref : m_connected_emitter_refs)
        {
            connected_emitter_ref.get().Connect(*this);
        }
    }

    ~Receiver() override
    {
        META_FUNCTION_TASK();
        const auto connected_emitter_refs = m_connected_emitter_refs;
        m_connected_emitter_refs.clear(); // no need to be processed in OnDisconnected callbacks from emitter

        // Disconnect all connected emitters on receiver destruction
        for(const Ref<IEmitter<EventType>>& connected_emitter_ref : connected_emitter_refs)
        {
            connected_emitter_ref.get().Disconnect(*this);
        }
    }

protected:
    template<class>
    friend class Emitter;

    void OnConnected(IEmitter<EventType>& emitter)
    {
        META_FUNCTION_TASK();
        const auto connected_emitter_ref_it = FindConnectedEmitter(emitter);
        if (connected_emitter_ref_it != m_connected_emitter_refs.end())
            return;

        m_connected_emitter_refs.emplace_back(emitter);
    }

    void OnDisconnected(IEmitter<EventType>& emitter)
    {
        META_FUNCTION_TASK();
        const auto connected_emitter_ref_it = FindConnectedEmitter(emitter);
        if (connected_emitter_ref_it == m_connected_emitter_refs.end())
            return;

        m_connected_emitter_refs.erase(connected_emitter_ref_it);
    }

    size_t GetConnectedEmittersCount() const noexcept { return m_connected_emitter_refs.size(); }

private:
    decltype(auto) FindConnectedEmitter(IEmitter<EventType>& emitter)
    {
        META_FUNCTION_TASK();
        return std::find_if(m_connected_emitter_refs.begin(), m_connected_emitter_refs.end(),
                            [&emitter](const Ref<IEmitter<EventType>>& connected_emitter_ref)
                            {
                                return std::addressof(connected_emitter_ref.get()) == std::addressof(emitter);
                            });
    }

    Refs<IEmitter<EventType>> m_connected_emitter_refs;
};

} // namespace Methane::Data