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

FILE: Methane/Graphics/RHI/ObjectRegistry.cpp
Methane PIMPL-based wrapper for ObjectRegistry.

******************************************************************************/

#include <Methane/Graphics/RHI/ObjectRegistry.h>
#include <Methane/Graphics/RHI/RenderContext.h>
#include <Methane/Graphics/RHI/ComputeContext.h>

#include <Methane/Pimpl.hpp>

#include <Methane/Graphics/Base/Object.h>

namespace Methane::Graphics::Rhi
{

ObjectRegistry::ObjectRegistry(IObjectRegistry& interface_ref)
    : m_impl_ref(dynamic_cast<Impl&>(interface_ref))
{
}

IObjectRegistry& ObjectRegistry::GetInterface() const META_PIMPL_NOEXCEPT
{
    return m_impl_ref;
}

void ObjectRegistry::AddGraphicsObjectInterface(IObject& object)
{
    m_impl_ref.AddGraphicsObject(object);
}

void ObjectRegistry::RemoveGraphicsObjectInterface(IObject& object)
{
    m_impl_ref.RemoveGraphicsObject(object);
}

Ptr<IObject> ObjectRegistry::GetGraphicsObject(const std::string& object_name) const META_PIMPL_NOEXCEPT
{
    return m_impl_ref.GetGraphicsObject(object_name);
}

bool ObjectRegistry::HasGraphicsObject(const std::string& object_name) const META_PIMPL_NOEXCEPT
{
    return m_impl_ref.HasGraphicsObject(object_name);
}

} // namespace Methane::Graphics::Rhi
