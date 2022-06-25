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

FILE: Methane/Graphics/ObjectBase.cpp
Base implementation of the named object interface.

******************************************************************************/

#include "ObjectBase.h"

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <stdexcept>

namespace Methane::Graphics
{

ObjectBase::RegistryBase::NameConflictException::NameConflictException(const std::string& name)
    : std::invalid_argument(fmt::format("Can not add graphics object with name {} to the registry because another object with the same name is already registered.", name))
{
    META_FUNCTION_TASK();
}

void ObjectBase::RegistryBase::AddGraphicsObject(Object& object)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_EMPTY_DESCR(object.GetName(), "Can not add graphics object without name to the objects registry.");

    const auto [name_and_object_it, object_added] = m_object_by_name.try_emplace(object.GetName(), object.GetPtr());
    if (!object_added &&
        !name_and_object_it->second.expired() &&
         name_and_object_it->second.lock().get() != std::addressof(object))
        throw NameConflictException(object.GetName());

    name_and_object_it->second = object.GetPtr();
    object.Connect(*this);
}

void ObjectBase::RegistryBase::RemoveGraphicsObject(Object& object)
{
    META_FUNCTION_TASK();

    const std::string& object_name = object.GetName();
    META_CHECK_ARG_NOT_EMPTY_DESCR(object_name, "Can not remove graphics object without name to the objects registry.");

    if (m_object_by_name.erase(object_name))
    {
        object.Disconnect(*this);
    }
}

Ptr<Object> ObjectBase::RegistryBase::GetGraphicsObject(const std::string& object_name) const noexcept
{
    META_FUNCTION_TASK();
    const auto object_by_name_it = m_object_by_name.find(object_name);
    return object_by_name_it == m_object_by_name.end() ? nullptr : object_by_name_it->second.lock();
}

bool ObjectBase::RegistryBase::HasGraphicsObject(const std::string& object_name) const noexcept
{
    META_FUNCTION_TASK();
    const auto object_by_name_it = m_object_by_name.find(object_name);
    return object_by_name_it != m_object_by_name.end() && !object_by_name_it->second.expired();
}

void ObjectBase::RegistryBase::OnObjectNameChanged(Object& object, const std::string& old_name)
{
    META_FUNCTION_TASK();
    const auto object_by_name_it = m_object_by_name.find(old_name);
    META_CHECK_ARG_TRUE_DESCR(object_by_name_it != m_object_by_name.end(),
                              "renamed object was not found in the objects registry by its old name '{}'", old_name);
    META_CHECK_ARG_TRUE_DESCR(object_by_name_it->second.expired(),
                              "object pointer stored in registry by old name '{}' has expired", old_name);
    META_CHECK_ARG_TRUE_DESCR(std::addressof(*object_by_name_it->second.lock()) == std::addressof(object),
                              "object stored in the registry by old name '{}' differs from the renamed object", old_name);

    const std::string& new_name = object.GetName();
    if (new_name.empty())
    {
        m_object_by_name.erase(object_by_name_it);
        object.Disconnect(*this);
        return;
    }

    auto object_node = m_object_by_name.extract(object_by_name_it);
    object_node.key() = new_name;
    m_object_by_name.insert(std::move(object_node));
}

void ObjectBase::RegistryBase::OnObjectDestroyed(Object& object)
{
    META_FUNCTION_TASK();
    RemoveGraphicsObject(object);
}

ObjectBase::ObjectBase(const std::string& name)
    : m_name(name)
{
    META_FUNCTION_TASK();
}

ObjectBase::~ObjectBase()
{
    META_FUNCTION_TASK();
    try
    {
        Emit(&IObjectCallback::OnObjectDestroyed, *this);
    }
    catch (const std::exception& e)
    {
        META_UNUSED(e);
        META_LOG("WARNING: Unexpected error during object destruction: {}", e.what());
        assert(false);
    }
}

Ptr<Object> ObjectBase::GetPtr()
{
    META_FUNCTION_TASK();
    return std::dynamic_pointer_cast<Object>(shared_from_this());
}

bool ObjectBase::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (m_name == name)
        return false;

    const std::string old_name = m_name;
    m_name = name;

    Emit(&IObjectCallback::OnObjectNameChanged, *this, old_name);
    return true;
}

} // namespace Methane::Graphics