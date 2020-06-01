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
        m_weak_receivers.push_back(receiver.GetSelfPtr());
    }

    void Disconnect(Receiver<EventType>& receiver)
    {
        META_FUNCTION_TASK();
        const auto weak_receiver_it = std::find_if(m_weak_receivers.begin(), m_weak_receivers.end(),
            [&receiver](const auto& cur_weak_receiver_ptr)
            {
                const auto cur_receiver_ptr = cur_weak_receiver_ptr.lock();
                return cur_receiver_ptr && std::addressof(cur_receiver_ptr->get()) == std::addressof(receiver);
            });
        if (weak_receiver_it != m_weak_receivers.end())
        {
            m_weak_receivers.erase(weak_receiver_it);
        }
    }

    template<typename FuncType, typename... ArgTypes>
    void Emit(FuncType&& func_ptr, ArgTypes&&... args)
    {
        META_FUNCTION_TASK();
        for(auto weak_receiver_it = m_weak_receivers.begin(); weak_receiver_it != m_weak_receivers.end();)
        {
            const auto strong_receiver_ptr = weak_receiver_it->lock();
            if (!strong_receiver_ptr)
            {
                weak_receiver_it = m_weak_receivers.erase(weak_receiver_it);
            }

            (strong_receiver_ptr->get().*std::forward<FuncType>(func_ptr))(std::forward<ArgTypes>(args)...);
            ++weak_receiver_it;
        }
    }

private:
    WeakPtrs<Ref<Receiver<EventType>>> m_weak_receivers;
};

} // namespace Methane::Data