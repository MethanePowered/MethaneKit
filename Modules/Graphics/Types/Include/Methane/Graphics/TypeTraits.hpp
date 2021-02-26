/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/TypeTraits.hpp
Methane graphics type traits available compile-time.

******************************************************************************/

#pragma once

#include "Volume.hpp"
#include "Color.hpp"

#include <Methane/Data/TypeTraits.hpp>

namespace Methane
{

template<typename D>
struct TypeTraits<Graphics::VolumeSize<D>>
{
    using ScalarType = D;

    static constexpr TypeOf type_of           = TypeOf::VolumeSize;
    static constexpr bool   is_floating_point = std::is_floating_point_v<D>;
    static constexpr bool   is_unit_type      = false;
    static constexpr size_t dimensions_count  = 3;
};

template<typename T, typename D>
struct TypeTraits<Graphics::Volume<T, D>>
{
    using ScalarType = T;

    static constexpr TypeOf type_of           = TypeOf::Volume;
    static constexpr bool   is_floating_point = std::is_floating_point_v<T> && std::is_floating_point_v<D>;
    static constexpr bool   is_unit_type      = false;
    static constexpr size_t dimensions_count  = 3;
};

template<typename T, size_t size>
struct TypeTraits<Graphics::Color<T, size>>
{
    using ScalarType = T;

    static constexpr TypeOf type_of           = TypeOf::Color;
    static constexpr bool   is_floating_point = std::is_floating_point_v<T>;
    static constexpr bool   is_unit_type      = false;
    static constexpr size_t dimensions_count  = size;
};

} // namespace Methane::Graphics
