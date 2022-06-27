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
#include <mutex>

namespace Methane::Data
{

template<class EventType>
class Receiver : public EventType
{
public:
    Receiver() = default;
    Receiver(const Receiver& other) noexcept
        : m_connected_emitter_refs(other.m_connected_emitter_refs)
    {
        META_FUNCTION_TASK();
        std::lock_guard lock(m_connected_emitter_refs_mutex);
        ConnectEmitters();
    }

    Receiver(Receiver&& other) noexcept
        : m_connected_emitter_refs(other.DisconnectEmitters())
    {
        META_FUNCTION_TASK();
        std::lock_guard lock(m_connected_emitter_refs_mutex);
        ConnectEmitters();
    }

    ~Receiver() override // NOSONAR
    {
        META_FUNCTION_TASK();
        std::lock_guard lock(m_connected_emitter_refs_mutex);
        DisconnectEmitters();
    }

    Receiver& operator=(const Receiver& other) noexcept
    {
        META_FUNCTION_TASK();
        if (this == std::addressof(other))
            return *this;

        std::lock_guard lock(m_connected_emitter_refs_mutex);
        DisconnectEmitters();
        m_connected_emitter_refs = other.m_connected_emitter_refs;
        ConnectEmitters();
        return *this;
    }

    Receiver& operator=(Receiver&& other) noexcept
    {
        META_FUNCTION_TASK();
        if (this == std::addressof(other))
            return *this;

        std::lock_guard lock(m_connected_emitter_refs_mutex);
        DisconnectEmitters();
        m_connected_emitter_refs = std::move(other.m_connected_emitter_refs);
        ConnectEmitters();
        return *this;
    }

protected:
    template<class>
    friend class Emitter;

    void OnConnected(IEmitter<EventType>& emitter) noexcept
    {
        META_FUNCTION_TASK();
        std::lock_guard lock(m_connected_emitter_refs_mutex);
        if (FindConnectedEmitter(emitter) != m_connected_emitter_refs.end())
            return;

        m_connected_emitter_refs.emplace_back(emitter);
    }

    void OnDisconnected(IEmitter<EventType>& emitter) noexcept
    {
        META_FUNCTION_TASK();
        std::lock_guard lock(m_connected_emitter_refs_mutex);
        const auto connected_emitter_ref_it = FindConnectedEmitter(emitter);
        if (connected_emitter_ref_it == m_connected_emitter_refs.end())
            return;

        m_connected_emitter_refs.erase(connected_emitter_ref_it);
    }

    [[nodiscard]] size_t GetConnectedEmittersCount() const noexcept { return m_connected_emitter_refs.size(); }

private:
    [[nodiscard]]
    inline decltype(auto) FindConnectedEmitter(IEmitter<EventType>& emitter) noexcept
    {
        return std::find_if(m_connected_emitter_refs.begin(), m_connected_emitter_refs.end(),
            [&emitter](const Ref<IEmitter<EventType>>& connected_emitter_ref)
            {
                return std::addressof(connected_emitter_ref.get()) == std::addressof(emitter);
            }
        );
    }

    inline void ConnectEmitters() noexcept
    {
        for(const Ref<IEmitter<EventType>>& connected_emitter_ref : m_connected_emitter_refs)
        {
            connected_emitter_ref.get().Connect(*this);
        }
    }

    inline auto DisconnectEmitters() noexcept
    {
        // Move connected emitters so that OnDisconnected callbacks are not processed (m_connected_emitter_refs would be empty)
        const auto connected_emitter_refs = std::move(m_connected_emitter_refs);
        for(const Ref<IEmitter<EventType>>& connected_emitter_ref : connected_emitter_refs)
        {
            connected_emitter_ref.get().Disconnect(*this);
        }
        return connected_emitter_refs;
    }

    Refs<IEmitter<EventType>> m_connected_emitter_refs;
    TracyLockable(std::recursive_mutex, m_connected_emitter_refs_mutex);
};

} // namespace Methane::Data