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

FILE: Methane/Graphics/RHI/ImplWrapper.hpp
Methane wrapper of the shared pointer to the private implementation of public interface
used in implementation of PImpl classes to optimize calls of virtual methods by
calling them through the pointer of final implementation (final classes or final method).

******************************************************************************/

#pragma once

#include <Methane/Memory.hpp>
#include <Methane/Checks.hpp>

#include <type_traits>

namespace Methane::Graphics::Rhi
{

template<typename PublicInterfaceType, typename PrivateImplType>
class ImplWrapper
{
    static_assert(std::is_base_of_v<PublicInterfaceType, PrivateImplType>, "Implementation type should be based on Interface type");

public:
    ImplWrapper(const Ptr<PublicInterfaceType>& interface_ptr)
        : m_impl_ptr(std::dynamic_pointer_cast<PrivateImplType>(interface_ptr))
        , m_interface(*interface_ptr)
    {
        META_CHECK_ARG_NOT_NULL_DESCR(m_impl_ptr, "Implementation pointer can not be null.");
    }

    PrivateImplType&       Get() noexcept       { return *m_impl_ptr; }
    const PrivateImplType& Get() const noexcept { return *m_impl_ptr; }

    const Ptr<PrivateImplType>& GetPtr() const noexcept { return m_impl_ptr; }
    PublicInterfaceType& GetInterface() const noexcept  { return m_interface; }

private:
    // Hold reference to public interface type along with shared pointer to private implementation,
    // so that it won't be necessary to do dynamic_cast to get interface.
    const Ptr<PrivateImplType> m_impl_ptr;
    PublicInterfaceType&       m_interface;
};

} // namespace Methane::Graphics::Rhi
