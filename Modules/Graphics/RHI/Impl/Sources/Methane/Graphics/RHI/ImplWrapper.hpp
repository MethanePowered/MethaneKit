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

#include <Methane/Graphics/RHI/Pimpl.h>

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
    using InterfaceType = PublicInterfaceType;
    using ImplType = PrivateImplType;

    ImplWrapper(const Ptr<PublicInterfaceType>& interface_ptr)
        : m_impl_ptr(std::dynamic_pointer_cast<PrivateImplType>(interface_ptr))
        , m_interface(*interface_ptr)
    {
        META_CHECK_ARG_NOT_NULL_DESCR(m_impl_ptr, "Implementation pointer can not be null.");
    }

    PrivateImplType&       Get() META_PIMPL_NOEXCEPT       { return *m_impl_ptr; }
    const PrivateImplType& Get() const META_PIMPL_NOEXCEPT { return *m_impl_ptr; }

    const Ptr<PrivateImplType>& GetPtr() const META_PIMPL_NOEXCEPT { return m_impl_ptr; }
    PublicInterfaceType&  GetInterface() const META_PIMPL_NOEXCEPT { return m_interface; }

private:
    // Hold reference to public interface type along with shared pointer to private implementation,
    // so that it won't be necessary to do dynamic_cast to get interface.
    const Ptr<PrivateImplType> m_impl_ptr;
    PublicInterfaceType&       m_interface;
};

template<typename ImplWrapperType>
typename ImplWrapperType::ImplType& GetPrivateImpl(const UniquePtr<ImplWrapperType>& impl_ptr) META_PIMPL_NOEXCEPT
{
#ifdef PIMPL_NULL_CHECK_ENABLED
    META_CHECK_ARG_NOT_NULL_DESCR(impl_ptr, "{} PIMPL is not initialized", typeid(typename ImplWrapperType::InterfaceType).name());
#endif
    return impl_ptr->Get();
}

template<typename ImplWrapperType>
typename ImplWrapperType::InterfaceType& GetPublicInterface(const UniquePtr<ImplWrapperType>& impl_ptr) META_PIMPL_NOEXCEPT
{
#ifdef PIMPL_NULL_CHECK_ENABLED
    META_CHECK_ARG_NOT_NULL_DESCR(impl_ptr, "{} PIMPL is not initialized", typeid(typename ImplWrapperType::InterfaceType).name());
#endif
    return impl_ptr->GetInterface();
}

} // namespace Methane::Graphics::Rhi
