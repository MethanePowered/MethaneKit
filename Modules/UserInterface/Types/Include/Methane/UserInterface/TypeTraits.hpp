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

FILE: Methane/UserInterface/TypeTraits.hpp
Methane user interface type traits available in compile time.

******************************************************************************/

#pragma once

#include "Types.hpp"

#include <Methane/Graphics/TypeTraits.hpp>

namespace Methane
{

template<typename BaseType>
struct TypeTraits<UserInterface::UnitType<BaseType>>
{
    using ScalarType = typename TypeTraits<BaseType>::ScalarType;

    static constexpr TypeOf type_of           = TypeTraits<BaseType>::type_of;
    static constexpr bool   is_floating_point = TypeTraits<BaseType>::is_floating_point;
    static constexpr bool   is_unit_type      = true;
    static constexpr size_t dimensions_count  = TypeTraits<BaseType>::dimensions_count;
};

} // namespace Methane::Graphics
