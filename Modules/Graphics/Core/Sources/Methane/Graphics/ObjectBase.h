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

FILE: Methane/Graphics/ObjectBase.h
Base implementation of the named object interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Object.h>
#include <Methane/Memory.hpp>
#include <Methane/Data/Emitter.hpp>

#include <map>

namespace Methane::Graphics
{

class ObjectBase // NOSONAR - destructor is required
    : public virtual Object // NOSONAR
    , public std::enable_shared_from_this<ObjectBase>
    , public Data::Emitter<IObjectCallback>
{
public:
    class RegistryBase
        : public Registry
        , private Data::Receiver<IObjectCallback>
    {
    public:
        void        AddGraphicsObject(Object& object) override;
        void        RemoveGraphicsObject(Object& object) override;
        Ptr<Object> GetGraphicsObject(const std::string& object_name) const noexcept override;
        bool        HasGraphicsObject(const std::string& object_name) const noexcept override;

    private:
        // IObjectCallback callback
        void OnObjectNameChanged(Object& object, const std::string& old_name) override;
        void OnObjectDestroyed(Object& object) override;

        std::map<std::string, WeakPtr<Object>, std::less<>> m_object_by_name;
    };

    ObjectBase() = default;
    explicit ObjectBase(const std::string& name);
    ~ObjectBase() override;

    ObjectBase(const ObjectBase&) = default;
    ObjectBase(ObjectBase&&) noexcept = default;

    ObjectBase& operator=(const ObjectBase&) = default;
    ObjectBase& operator=(ObjectBase&&) noexcept = default;

    // Object interface
    bool               SetName(const std::string& name) override;
    const std::string& GetName() const noexcept override  { return m_name; }
    Ptr<Object>        GetPtr() override;

    Ptr<ObjectBase> GetBasePtr() { return shared_from_this(); }

    template<typename T>
    std::enable_if_t<std::is_base_of_v<ObjectBase, T>, Ptr<T>> GetPtr()
    { return std::static_pointer_cast<T>(GetBasePtr()); }

private:
    std::string m_name;
};

} // namespace Methane::Graphics
