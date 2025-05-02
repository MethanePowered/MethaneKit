/******************************************************************************

Copyright 2025 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/ObjectRegistry.h
Methane PIMPL-based wrapper for ObjectRegistry.

******************************************************************************/

#pragma once

#include <Methane/Pimpl.h>

#include <Methane/Graphics/RHI/IObject.h>

namespace Methane::Graphics::Base
{
class ObjectRegistry;
}

namespace Methane::Graphics::Rhi
{

class RenderContext;
class ComputeContext;

class ObjectRegistry
{
public:
    using NameConflictException = Rhi::NameConflictException;

    META_PIMPL_API explicit ObjectRegistry(IObjectRegistry& interface_ref);

    META_PIMPL_API IObjectRegistry& GetInterface() const META_PIMPL_NOEXCEPT;

    // IObjectRegistry interface methods
    META_PIMPL_API void AddGraphicsObjectInterface(IObject& object);
    META_PIMPL_API void RemoveGraphicsObjectInterface(IObject& object);
    [[nodiscard]] META_PIMPL_API Ptr<IObject> GetGraphicsObject(const std::string& object_name) const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API bool         HasGraphicsObject(const std::string& object_name) const META_PIMPL_NOEXCEPT;

    template<typename ObjectImplType>
    META_PIMPL_API void AddGraphicsObject(const ObjectImplType& object)
    {
        AddGraphicsObjectInterface(object.GetInterface());
    }

    template<typename ObjectImplType>
    META_PIMPL_API void RemoveGraphicsObject(const ObjectImplType& object)
    {
        RemoveGraphicsObjectInterface(object.GetInterface());
    }

    template<typename ObjectImplType>
    ObjectImplType GetGraphicsObject(const std::string& object_name) const META_PIMPL_NOEXCEPT
    {
        return ObjectImplType(std::dynamic_pointer_cast<typename ObjectImplType::Interface>(GetGraphicsObject(object_name)));
    }

private:
    using Impl = Methane::Graphics::Base::ObjectRegistry;

    Impl& m_impl_ref;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_PIMPL_INLINE

#include <Methane/Graphics/RHI/ObjectRegistry.cpp>

#endif // META_PIMPL_INLINE
