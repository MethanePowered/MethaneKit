/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/Data/Emitter.hpp
Event emitter base template class implementation.

******************************************************************************/

#pragma once

#include "Receiver.hpp"

#include <Methane/Instrumentation.h>

#include <algorithm>

namespace Methane::Data
{

template<typename EventType>
class Emitter
{
    using Receivers = WeakPtrs<Ref<Receiver<EventType>>>;

public:
    void Connect(Receiver<EventType>& receiver)
    {
        META_FUNCTION_TASK();
        m_connected_receiver_weak_ptrs.push_back(receiver.GetSelfPtr());
    }

    void Disconnect(Receiver<EventType>& receiver)
    {
        META_FUNCTION_TASK();
        const auto weak_receiver_it = std::find_if(m_connected_receiver_weak_ptrs.begin(), m_connected_receiver_weak_ptrs.end(),
                                                   [&receiver](const auto& cur_weak_receiver_ptr)
            {
                const auto cur_receiver_ptr = cur_weak_receiver_ptr.lock();
                return cur_receiver_ptr && std::addressof(cur_receiver_ptr->get()) == std::addressof(receiver);
            });
        if (weak_receiver_it != m_connected_receiver_weak_ptrs.end())
        {
            m_connected_receiver_weak_ptrs.erase(weak_receiver_it);
        }
    }

    template<typename EventFuncType, typename... EventArgTypes>
    void Emit(EventFuncType&& event_func, EventArgTypes&&... event_args)
    {
        META_FUNCTION_TASK();
        for(auto connected_receiver_weak_ptr_it = m_connected_receiver_weak_ptrs.begin();
                 connected_receiver_weak_ptr_it != m_connected_receiver_weak_ptrs.end();)
        {
            const auto connected_receiver_ptr = connected_receiver_weak_ptr_it->lock();
            if (!connected_receiver_ptr)
            {
                connected_receiver_weak_ptr_it = m_connected_receiver_weak_ptrs.erase(connected_receiver_weak_ptr_it);
                continue;
            }

            (connected_receiver_ptr->get().*std::forward<EventFuncType>(event_func))(std::forward<EventArgTypes>(event_args)...);
            ++connected_receiver_weak_ptr_it;
        }
    }

    size_t GetConnectedReceiversCount() const { return m_connected_receiver_weak_ptrs.size(); }

private:
    WeakPtrs<Ref<Receiver<EventType>>> m_connected_receiver_weak_ptrs;
};

} // namespace Methane::Data