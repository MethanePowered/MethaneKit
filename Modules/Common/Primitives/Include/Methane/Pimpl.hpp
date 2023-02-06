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

FILE: Methane/Pimpl.hpp
Methane PIMPL implementation helper functions.

******************************************************************************/

#pragma once

#include <Methane/Pimpl.h>
#include <Methane/Memory.hpp>

#ifdef META_PIMPL_NULL_CHECK_ENABLED
#include <Methane/Checks.hpp>
#endif

namespace Methane
{

template<typename ImplType>
ImplType& GetImpl(const Ptr<ImplType>& impl_ptr) META_PIMPL_NOEXCEPT
{
#ifdef META_PIMPL_NULL_CHECK_ENABLED
    META_CHECK_ARG_NOT_NULL_DESCR(impl_ptr, "{} PIMPL is not initialized", typeid(ImplType).name());
#endif
    return *impl_ptr;
}

} // namespace Methane
