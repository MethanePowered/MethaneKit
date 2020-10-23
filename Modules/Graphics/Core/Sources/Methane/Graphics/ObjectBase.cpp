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

#include <stdexcept>

namespace Methane::Graphics
{

void ObjectBase::RegistryBase::AddGraphicsObject(Object& object)
{
    META_FUNCTION_TASK();
    if (object.GetName().empty())
        throw std::logic_error("Can not add graphics object without name to the objects registry.");

    const auto add_result = m_object_by_name.emplace(object.GetName(), object.GetPtr());
    if (!add_result.second && !add_result.first->second.expired() && add_result.first->second.lock().get() != std::addressof(object))
        throw std::logic_error("Can not add graphics object with name " + object.GetName() +
                               " to the objects registry because another object with the same name is already registered.");

    add_result.first->second = object.GetPtr();
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

} // namespace Methane::Graphics