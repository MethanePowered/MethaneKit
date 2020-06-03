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

namespace Methane::Data
{

template<typename EventType>
class Emitter : public IEmitter<EventType>
{
public:
    ~Emitter() override
    {
        META_FUNCTION_TASK();
        const auto connected_receiver_refs = m_connected_receiver_opt_refs;
        m_connected_receiver_opt_refs.clear(); // no need to be processed in Disconnect callbacks from receiver

        for(const auto& receiver_opt_ref : connected_receiver_refs)
        {
            if (receiver_opt_ref)
                continue;

            receiver_opt_ref->get().OnDisconnected(*this);
        }
    }

    void Connect(Receiver<EventType>& receiver) override
    {
        META_FUNCTION_TASK();
        const auto connected_receiver_it = FindConnectedReceiver(receiver);
        if (connected_receiver_it != m_connected_receiver_opt_refs.end())
            return;

        if (m_is_emitting)
        {
            // Modification of connected receivers collection is prohibited during emit cycle, so we add them to separate collection
            m_additional_connected_receiver_opt_refs.emplace_back(receiver);
        }
        else
        {
            m_connected_receiver_opt_refs.emplace_back(receiver);
        }
        receiver.OnConnected(*this);
    }

    void Disconnect(Receiver<EventType>& receiver) override
    {
        META_FUNCTION_TASK();
        const auto connected_receiver_it = FindConnectedReceiver(receiver);
        if (connected_receiver_it == m_connected_receiver_opt_refs.end())
            return;

        if (!connected_receiver_it->has_value())
            throw std::logic_error("Something went really wrong with connected receivers at this point.");

        if (m_is_emitting)
        {
            // Modification of connected receivers collection is prohibited during emit cycle, so we just clear the reference instead of erasing from collection
            connected_receiver_it->reset();
        }
        else
        {
            m_connected_receiver_opt_refs.erase(connected_receiver_it);
        }
        receiver.OnDisconnected(*this);
    }

    template<typename FuncType, typename... ArgTypes>
    void Emit(FuncType&& func_ptr, ArgTypes&&... args)
    {
        META_FUNCTION_TASK();
        bool is_cleanup_required = false;

        m_is_emitting = true;
        for(const auto& receiver_opt_ref : m_connected_receiver_opt_refs)
        {
            if (!receiver_opt_ref)
            {
                is_cleanup_required = true;
                continue;
            }

            // Call the emitted event function in receiver
            (receiver_opt_ref->get().*std::forward<FuncType>(func_ptr))(std::forward<ArgTypes>(args)...);

            if (!receiver_opt_ref)
            {
                // Receiver may be disconnected or destroyed during emitted event and it will be cleaned up after full emit cycle
                is_cleanup_required = true;
            }
        }
        m_is_emitting = false;

        if (is_cleanup_required)
        {
            // Erase receivers disconnected during emit cycle from the connected receivers
            for(auto connected_receiver_opt_ref_it  = m_connected_receiver_opt_refs.begin();
                     connected_receiver_opt_ref_it != m_connected_receiver_opt_refs.end();)
            {
                if (connected_receiver_opt_ref_it->has_value())
                    connected_receiver_opt_ref_it++;
                else
                    connected_receiver_opt_ref_it = m_connected_receiver_opt_refs.erase(connected_receiver_opt_ref_it);
            }
        }

        // Add receivers connected during emit cycle to the connected receivers
        if (!m_additional_connected_receiver_opt_refs.empty())
        {
            m_connected_receiver_opt_refs.insert(m_connected_receiver_opt_refs.end(), m_additional_connected_receiver_opt_refs.begin(), m_additional_connected_receiver_opt_refs.end());
            m_additional_connected_receiver_opt_refs.clear();
        }
    }

    size_t GetConnectedReceiversCount() const noexcept { return m_connected_receiver_opt_refs.size() + m_additional_connected_receiver_opt_refs.size(); }

private:
    decltype(auto) FindConnectedReceiver(Receiver<EventType>& receiver)
    {
        META_FUNCTION_TASK();
        return std::find_if(m_connected_receiver_opt_refs.begin(), m_connected_receiver_opt_refs.end(),
            [&receiver](auto& connected_receiver_opt_ref)
            {
                return connected_receiver_opt_ref && std::addressof(connected_receiver_opt_ref->get()) == std::addressof(receiver);
            }
        );
    }

    bool m_is_emitting = false;
    Opts<Ref<Receiver<EventType>>> m_connected_receiver_opt_refs;
    Opts<Ref<Receiver<EventType>>> m_additional_connected_receiver_opt_refs;
};

} // namespace Methane::Data