/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Data/Transmitter.h
Event transmitter implementation, which passes connected receivers to other emitter

******************************************************************************/

#pragma once

#include "IEmitter.h"
#include "Receiver.hpp"

#include <Methane/Instrumentation.h>

namespace Methane::Data
{

template<typename EventType>
class Transmitter
    : public IEmitter<EventType>
{
public:
    Transmitter() = default;
    Transmitter(IEmitter<EventType>& target_emitter)
        : m_target_emitter_ptr(&target_emitter)
    { }

    void Connect(Receiver<EventType>& receiver) final
    {
        if (m_target_emitter_ptr)
            m_target_emitter_ptr->Connect(receiver);
    }

    void Disconnect(Receiver<EventType>& receiver) final
    {
        if (m_target_emitter_ptr)
            m_target_emitter_ptr->Disconnect(receiver);
    }

    void Reset(IEmitter<EventType>* target_emitter_ptr = nullptr)
    {
        m_target_emitter_ptr = target_emitter_ptr;
    }

private:
    IEmitter<EventType>* m_target_emitter_ptr = nullptr;
};

} // namespace Methane::Data
