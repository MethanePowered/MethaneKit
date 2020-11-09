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

FILE: Methane/Data/IEmitter.h
Event emitter abstract interface

******************************************************************************/

#pragma once

namespace Methane::Data
{

template<class>
class Receiver;

template<class EventType>
struct IEmitter
{
    virtual void Connect(Receiver<EventType>& receiver) = 0;
    virtual void Disconnect(Receiver<EventType>& receiver) = 0;

    virtual ~IEmitter() = default;
};

}