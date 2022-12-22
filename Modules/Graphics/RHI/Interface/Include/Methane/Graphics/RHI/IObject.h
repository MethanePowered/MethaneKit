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

FILE: Methane/Graphics/RHI/IObject.h
Methane object interface: represents any named object.

******************************************************************************/

#pragma once

#include <Methane/Memory.hpp>
#include <Methane/Data/IEmitter.h>

#include <string>
#include <stdexcept>

namespace Methane::Graphics::Rhi
{

struct IObject;

class NameConflictException : public std::invalid_argument
{
public:
    explicit NameConflictException(std::string_view name);
};

struct IObjectRegistry
{
    using NameConflictException = Rhi::NameConflictException;

    virtual void AddGraphicsObject(IObject& object) = 0;
    virtual void RemoveGraphicsObject(IObject& object) = 0;
    [[nodiscard]] virtual Ptr<IObject> GetGraphicsObject(const std::string& object_name) const noexcept = 0;
    [[nodiscard]] virtual bool         HasGraphicsObject(const std::string& object_name) const noexcept = 0;

    virtual ~IObjectRegistry() = default;
};

struct IObjectCallback
{
    virtual void OnObjectNameChanged(IObject&, const std::string& /*old_name*/) { /* does nothing by default */ }
    virtual void OnObjectDestroyed(IObject&)                                    { /* does nothing by default */ }

    virtual ~IObjectCallback() = default;
};

struct IObject
    : virtual Data::IEmitter<IObjectCallback> // NOSONAR
{
    using IRegistry = IObjectRegistry;

    virtual bool SetName(std::string_view name) = 0;
    [[nodiscard]] virtual std::string_view GetName() const noexcept = 0;
    [[nodiscard]] virtual Ptr<IObject>     GetPtr() = 0;

    template<typename T>
    std::enable_if_t<std::is_base_of_v<IObject, T>, Ptr<T>> GetDerivedPtr()
    { return std::dynamic_pointer_cast<T>(GetPtr()); }
};

} // namespace Methane::Graphics::Rhi
