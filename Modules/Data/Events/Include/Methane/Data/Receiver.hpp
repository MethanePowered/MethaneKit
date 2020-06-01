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

FILE: Methane/Data/Receiver.hpp
Event receiver base template class implementation.

******************************************************************************/

#pragma once

#include <Methane/Memory.hpp>

#include <functional>

namespace Methane::Data
{

template<class EventType>
class Receiver : public EventType
{
    using RefType = Ref<Receiver<EventType>>;

public:
    Receiver() : m_self_ptr(std::make_shared<RefType>(*this)) { }

    virtual ~Receiver() = default;

protected:
    template<class> friend class Emitter;

    const Ptr<RefType>& GetSelfPtr() const noexcept { return m_self_ptr; }

private:
    Ptr<RefType> m_self_ptr;
};

} // namespace Methane::Data