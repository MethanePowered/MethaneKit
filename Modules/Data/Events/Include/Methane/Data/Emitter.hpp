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

FILE: Methane/Data/Emitter.hpp
Event emitter base template class implementation.

******************************************************************************/

#pragma once

#include "Receiver.hpp"

#include <Methane/Instrumentation.h>

#include <ranges>

namespace Methane::Data
{

template<typename EventType>
class Emitter // NOSONAR - custom destructor is required, rule of zero is not applicable
    : public virtual IEmitter<EventType> // NOSONAR - virtual inheritance is required
{
    using ReceiverAndPriority = std::pair<Receiver<EventType>*, int32_t>;
    static bool CompareReceiverAndPriority(const ReceiverAndPriority& left, const ReceiverAndPriority& right)
    {
        return left.second > right.second;
    }

public:
    Emitter() = default;
    Emitter(const Emitter& other) noexcept
        : m_connected_receivers(other.m_connected_receivers)
    {
        META_FUNCTION_TASK();
        ConnectReceivers();
    }

    Emitter(Emitter&& other) noexcept
        : m_connected_receivers(other.DisconnectReceivers())
    {
        META_FUNCTION_TASK();
        ConnectReceivers();
    }

    ~Emitter() override
    {
        META_FUNCTION_TASK();
        DisconnectReceivers();
    }

    Emitter& operator=(const Emitter& other) noexcept
    {
        META_FUNCTION_TASK();
        if (this == std::addressof(other))
            return *this;

        DisconnectReceivers();
        m_connected_receivers = other.m_connected_receivers;
        ConnectReceivers();
        return *this;
    }

    Emitter& operator=(Emitter&& other) noexcept
    {
        META_FUNCTION_TASK();
        if (this == std::addressof(other))
            return *this;

        DisconnectReceivers();
        m_connected_receivers = std::move(other.m_connected_receivers);
        ConnectReceivers();
        return *this;
    }

    void Connect(Receiver<EventType>& receiver, int32_t priority = 0) noexcept final
    {
        META_FUNCTION_TASK();
        std::lock_guard lock(m_connected_receivers_mutex);
        if (FindConnectedReceiver(receiver) != m_connected_receivers.end())
            return;

        // Modification of connected receivers collection is prohibited during emit cycle, so we add them to separate collection and merge later
        auto& connected_receivers = m_is_emitting ? m_additional_connected_receivers : m_connected_receivers;
        const auto receiver_and_priority = std::make_pair(&receiver, priority);
        connected_receivers.insert(
           std::ranges::upper_bound(connected_receivers, receiver_and_priority, CompareReceiverAndPriority),
           receiver_and_priority
        );

        receiver.OnConnected(*this);
    }

    void Disconnect(Receiver<EventType>& receiver) noexcept final
    {
        META_FUNCTION_TASK();
        std::lock_guard lock(m_connected_receivers_mutex);

        const auto connected_receiver_it = FindConnectedReceiver(receiver);
        if (connected_receiver_it == m_connected_receivers.end())
        {
            if (m_is_emitting)
            {
                const auto receiver_it = std::ranges::find_if(m_additional_connected_receivers,
                    [&receiver](const ReceiverAndPriority& receiver_and_priority)
                    { return &receiver == receiver_and_priority.first; }
                );
                m_additional_connected_receivers.erase(receiver_it);
            }
            return;
        }

        if (m_is_emitting)
        {
            // Modification of connected receivers collection is prohibited during emit cycle, so we just clear the reference instead of erasing from collection
            connected_receiver_it->first = nullptr;
        }
        else
        {
            m_connected_receivers.erase(connected_receiver_it);
        }
        receiver.OnDisconnected(*this);
    }

protected:
    template<typename FuncType, typename... ArgTypes>
    void Emit(FuncType&& func_ptr, ArgTypes&&... args)
    {
        META_FUNCTION_TASK();
        std::lock_guard lock(m_connected_receivers_mutex);

        // Additional receivers may be non-empty before emitting connected receiver calls
        // only in case when current emit is called during another emitted callback for the same emitter
        if (m_is_emitting && !m_additional_connected_receivers.empty())
        {
            // Create copy of the additional connected receivers to iterate over,
            // because original set of additional receivers may change during emitted calls
            auto additional_connected_receivers = m_additional_connected_receivers;
            EmitFuncOfReceivers(additional_connected_receivers, std::forward<FuncType>(func_ptr), std::forward<ArgTypes>(args)...);
        }

        // Emit function of connected receivers
        bool was_emitting = m_is_emitting;
        m_is_emitting = true;
        if (EmitFuncOfReceivers(m_connected_receivers, std::forward<FuncType>(func_ptr), std::forward<ArgTypes>(args)...))
        {
            CleanupConnectedReceivers();
        }
        m_is_emitting = was_emitting;

        // Add additional receivers connected during emit cycle to the connected receivers
        if (!was_emitting && !m_additional_connected_receivers.empty())
        {
            m_connected_receivers.insert(m_connected_receivers.end(), m_additional_connected_receivers.begin(), m_additional_connected_receivers.end());
            std::ranges::sort(m_connected_receivers, CompareReceiverAndPriority);
            m_additional_connected_receivers.clear();
        }
    }

    size_t GetConnectedReceiversCount() const noexcept
    {
        return m_connected_receivers.size() + m_additional_connected_receivers.size();
    }

private:
    [[nodiscard]]
    inline decltype(auto) FindConnectedReceiver(Receiver<EventType>& receiver) noexcept
    {
        return std::ranges::find_if(m_connected_receivers,
            [&receiver](const ReceiverAndPriority& receiver_and_priority)
            {
                return receiver_and_priority.first && receiver_and_priority.first == std::addressof(receiver);
            }
        );
    }

    template<typename ReceiversContainerType, typename FuncType, typename... ArgTypes>
    bool EmitFuncOfReceivers(ReceiversContainerType& receivers, FuncType&& func_ptr, ArgTypes&&... args)
    {
        bool is_cleanup_required = false;
        for(const ReceiverAndPriority& receiver_and_priority : receivers)
        {
            if (!receiver_and_priority.first)
            {
                is_cleanup_required = true;
                continue;
            }

            // Call the emitted event function in receiver
            (receiver_and_priority.first->*std::forward<FuncType>(func_ptr))(std::forward<ArgTypes>(args)...);

            if (!receiver_and_priority.first)
            {
                // Receiver may be disconnected or destroyed during emitted event, so it will be cleaned up after full emit cycle
                is_cleanup_required = true;
            }
        }
        return is_cleanup_required;
    }

    inline void CleanupConnectedReceivers() noexcept
    {
        // Erase receivers disconnected during emit cycle from the connected receivers
        std::lock_guard lock(m_connected_receivers_mutex);
        for(auto connected_receiver_it  = m_connected_receivers.begin(); connected_receiver_it != m_connected_receivers.end();)
        {
            if (connected_receiver_it->first)
                connected_receiver_it++;
            else
                connected_receiver_it = m_connected_receivers.erase(connected_receiver_it);
        }
    }

    inline void ConnectReceivers() noexcept
    {
        std::lock_guard lock(m_connected_receivers_mutex);
        for(const ReceiverAndPriority& receiver_and_priority : m_connected_receivers)
        {
            if (receiver_and_priority.first)
                receiver_and_priority.first->OnConnected(*this);
        }
    }

    inline auto DisconnectReceivers() noexcept
    {
        // Move connected receivers so that OnDisconnected callbacks are not processed (m_connected_receivers would be empty)
        std::lock_guard lock(m_connected_receivers_mutex);
        const auto connected_receivers = std::move(m_connected_receivers);
        for(const ReceiverAndPriority& receiver_and_priority : connected_receivers)
        {
            if (!receiver_and_priority.first)
                continue;

            receiver_and_priority.first->OnDisconnected(*this);
        }
        return connected_receivers;
    }

    bool                                m_is_emitting = false;
    std::vector<ReceiverAndPriority>    m_connected_receivers;
    std::vector<ReceiverAndPriority>    m_additional_connected_receivers;
#if defined(__GNUG__) && !defined(__clang__)
    // GCC fails with internal compiler error: Segmentation fault
    std::recursive_mutex                m_connected_receivers_mutex;
#else
    TracyLockable(std::recursive_mutex, m_connected_receivers_mutex);
#endif
};

} // namespace Methane::Data