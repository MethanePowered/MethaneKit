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

#include <map>

namespace Methane::Graphics
{

class ObjectBase
    : public virtual Object
    , public std::enable_shared_from_this<ObjectBase>
{
public:
    class RegistryBase : public Registry
    {
    public:
        void        AddGraphicsObject(Object& object) override;
        Ptr<Object> GetGraphicsObject(const std::string& object_name) const noexcept override;
        bool        HasGraphicsObject(const std::string& object_name) const noexcept override;

    private:
        std::map<std::string, WeakPtr<Object>> m_object_by_name;
    };

    ObjectBase() = default;
    explicit ObjectBase(const std::string& name) : m_name(name) { }

    // Object interface
    void               SetName(const std::string& name) override { m_name = name; }
    const std::string& GetName() const noexcept override         { return m_name; }
    Ptr<Object>        GetPtr() override                         { return std::dynamic_pointer_cast<Object>(shared_from_this()); }

    Ptr<ObjectBase>    GetBasePtr()                              { return shared_from_this(); }

private:
    std::string m_name;
};

} // namespace Methane::Graphics
