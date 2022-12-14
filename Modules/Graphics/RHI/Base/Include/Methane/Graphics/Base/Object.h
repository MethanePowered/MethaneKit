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

FILE: Methane/Graphics/Base/Object.h
Base implementation of the named object interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RHI/IObject.h>
#include <Methane/Memory.hpp>
#include <Methane/Data/Emitter.hpp>

#include <map>

namespace Methane::Graphics::Base
{

class ObjectRegistry final
    : public Rhi::IObjectRegistry
    , private Data::Receiver<Rhi::IObjectCallback>
{
public:
    void                 AddGraphicsObject(Rhi::IObject& object) override;
    void                 RemoveGraphicsObject(Rhi::IObject& object) override;
    Ptr<Rhi::IObject>    GetGraphicsObject(const std::string& object_name) const noexcept override;
    bool                 HasGraphicsObject(const std::string& object_name) const noexcept override;

private:
    // IObjectCallback callback
    void OnObjectNameChanged(Rhi::IObject& object, const std::string& old_name) override;
    void OnObjectDestroyed(Rhi::IObject& object) override;

    std::map<std::string, WeakPtr<Rhi::IObject>, std::less<>> m_object_by_name;
};

class Object // NOSONAR - destructor is required
    : public virtual Rhi::IObject // NOSONAR
    , public std::enable_shared_from_this<Object>
    , public Data::Emitter<Rhi::IObjectCallback>
{
public:
    using Registry = ObjectRegistry;

    Object() = default;
    explicit Object(std::string_view name);
    ~Object() override;

    Object(const Object&) = default;
    Object(Object&&) noexcept = default;

    Object& operator=(const Object&) = default;
    Object& operator=(Object&&) noexcept = default;

    // IObject interface
    bool               SetName(std::string_view name) override;
    std::string_view   GetName() const noexcept override { return m_name; }
    Ptr<Rhi::IObject>  GetPtr() override;

    Ptr<Object>        GetBasePtr()                { return shared_from_this(); }
    const std::string& GetNameRef() const noexcept { return m_name; }

    template<typename T>
    std::enable_if_t<std::is_base_of_v<Object, T>, Ptr<T>> GetPtr()
    { return std::static_pointer_cast<T>(GetBasePtr()); }

private:
    std::string m_name;
};

} // namespace Methane::Graphics::Base
