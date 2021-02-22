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

FILE: Methane/Data/TypeTraits.hpp
Methane data type traits available compile-time.

******************************************************************************/

#pragma once

#include "Types.h"

namespace Methane
{
    enum class TypeOf
    {
        Undefined,
        HlslVector,
        RawVector,
        Point,
        RectSize,
        Rect,
        VolumeSize,
        Volume,
        Color
    };

    template<typename Type>
    struct TypeTraits
    {
        using ScalarType = void;

        static constexpr TypeOf type_of           = TypeOf::Undefined;
        static constexpr bool   is_floating_point = false;
        static constexpr bool   is_unit_type      = false;
        static constexpr size_t dimensions_count  = 0;
    };

    template<typename T, size_t size>
    struct TypeTraits<Data::Point<T, size>>
    {
        using ScalarType = T;

        static constexpr TypeOf type_of           = TypeOf::Point;
        static constexpr bool   is_floating_point = std::is_floating_point_v<T>;
        static constexpr bool   is_unit_type      = false;
        static constexpr size_t dimensions_count  = size;
    };

    template<typename D>
    struct TypeTraits<Data::RectSize<D>>
    {
        using ScalarType = D;

        static constexpr TypeOf type_of           = TypeOf::RectSize;
        static constexpr bool   is_floating_point = std::is_floating_point_v<D>;
        static constexpr bool   is_unit_type      = false;
        static constexpr size_t dimensions_count  = 2;
    };

    template<typename T, typename D>
    struct TypeTraits<Data::Rect<T, D>>
    {
        using ScalarType = T;

        static constexpr TypeOf type_of           = TypeOf::Rect;
        static constexpr bool   is_floating_point = std::is_floating_point_v<T> && std::is_floating_point_v<D>;
        static constexpr bool   is_unit_type      = false;
        static constexpr size_t dimensions_count  = 2;
    };


    template<typename T, size_t size>
    struct TypeTraits<Data::RawVector<T, size>>
    {
        using ScalarType = T;

        static constexpr TypeOf type_of           = TypeOf::RawVector;
        static constexpr bool   is_floating_point = std::is_floating_point_v<T>;
        static constexpr bool   is_unit_type      = false;
        static constexpr size_t dimensions_count  = size;
    };

    template<>
    struct TypeTraits<hlslpp::int1>
    {
        using ScalarType = int32_t;

        static constexpr TypeOf type_of = TypeOf::HlslVector;
        static constexpr bool   is_floating_point = false;
        static constexpr bool   is_unit_type      = false;
        static constexpr size_t dimensions_count  = 1;
    };

    template<>
    struct TypeTraits<hlslpp::int2>
    {
        using ScalarType = int32_t;

        static constexpr TypeOf type_of           = TypeOf::HlslVector;
        static constexpr bool   is_floating_point = false;
        static constexpr bool   is_unit_type      = false;
        static constexpr size_t dimensions_count  = 2;
    };

    template<>
    struct TypeTraits<hlslpp::int3>
    {
        using ScalarType = int32_t;

        static constexpr TypeOf type_of           = TypeOf::HlslVector;
        static constexpr bool   is_floating_point = false;
        static constexpr bool   is_unit_type      = false;
        static constexpr size_t dimensions_count  = 3;
    };

    template<>
    struct TypeTraits<hlslpp::int4>
    {
        using ScalarType = int32_t;

        static constexpr TypeOf type_of           = TypeOf::HlslVector;
        static constexpr bool   is_floating_point = false;
        static constexpr bool   is_unit_type      = false;
        static constexpr size_t dimensions_count  = 4;
    };

    template<>
    struct TypeTraits<hlslpp::uint1>
    {
        using ScalarType = uint32_t;

        static constexpr TypeOf type_of           = TypeOf::HlslVector;
        static constexpr bool   is_floating_point = false;
        static constexpr bool   is_unit_type      = false;
        static constexpr size_t dimensions_count  = 1;
    };

    template<>
    struct TypeTraits<hlslpp::uint2>
    {
        using ScalarType = uint32_t;

        static constexpr TypeOf type_of           = TypeOf::HlslVector;
        static constexpr bool   is_floating_point = false;
        static constexpr bool   is_unit_type      = false;
        static constexpr size_t dimensions_count  = 2;
    };

    template<>
    struct TypeTraits<hlslpp::uint3>
    {
        using ScalarType = uint32_t;

        static constexpr TypeOf type_of           = TypeOf::HlslVector;
        static constexpr bool   is_floating_point = false;
        static constexpr bool   is_unit_type      = false;
        static constexpr size_t dimensions_count  = 3;
    };

    template<>
    struct TypeTraits<hlslpp::uint4>
    {
        using ScalarType = uint32_t;

        static constexpr TypeOf type_of           = TypeOf::HlslVector;
        static constexpr bool   is_floating_point = false;
        static constexpr bool   is_unit_type      = false;
        static constexpr size_t dimensions_count  = 4;
    };

    template<>
    struct TypeTraits<hlslpp::float1>
    {
        using ScalarType = float;

        static constexpr TypeOf type_of           = TypeOf::HlslVector;
        static constexpr bool   is_floating_point = true;
        static constexpr bool   is_unit_type      = false;
        static constexpr size_t dimensions_count  = 1;
    };

    template<>
    struct TypeTraits<hlslpp::float2>
    {
        using ScalarType = float;

        static constexpr TypeOf type_of           = TypeOf::HlslVector;
        static constexpr bool   is_floating_point = true;
        static constexpr bool   is_unit_type      = false;
        static constexpr size_t dimensions_count  = 2;
    };

    template<>
    struct TypeTraits<hlslpp::float3>
    {
        using ScalarType = float;

        static constexpr TypeOf type_of           = TypeOf::HlslVector;
        static constexpr bool   is_floating_point = true;
        static constexpr bool   is_unit_type      = false;
        static constexpr size_t dimensions_count  = 3;
    };

    template<>
    struct TypeTraits<hlslpp::float4>
    {
        using ScalarType = float;

        static constexpr TypeOf type_of           = TypeOf::HlslVector;
        static constexpr bool   is_floating_point = true;
        static constexpr bool   is_unit_type      = false;
        static constexpr size_t dimensions_count  = 4;
    };

#ifdef HLSLPP_DOUBLE
    template<>
    struct TypeTraits<hlslpp::double1>
    {
        using ScalarType = double;

        static constexpr TypeOf type_of           = TypeOf::HlslVector;
        static constexpr bool   is_floating_point = true;
        static constexpr bool   is_unit_type      = false;
        static constexpr size_t dimensions_count  = 1;
    };

    template<>
    struct TypeTraits<hlslpp::double2>
    {
        using ScalarType = double;

        static constexpr TypeOf type_of           = TypeOf::HlslVector;
        static constexpr bool   is_floating_point = true;
        static constexpr bool   is_unit_type      = false;
        static constexpr size_t dimensions_count  = 2;
    };

    template<>
    struct TypeTraits<hlslpp::double3>
    {
        using ScalarType = double;

        static constexpr TypeOf type_of           = TypeOf::HlslVector;
        static constexpr bool   is_floating_point = true;
        static constexpr bool   is_unit_type      = false;
        static constexpr size_t dimensions_count  = 3;
    };

    template<>
    struct TypeTraits<hlslpp::double4>
    {
        using ScalarType = double;

        static constexpr TypeOf type_of           = TypeOf::HlslVector;
        static constexpr bool   is_floating_point = true;
        static constexpr bool   is_unit_type      = false;
        static constexpr size_t dimensions_count  = 4;
    };
#endif // defined(HLSLPP_DOUBLE)

} // namespace Methane::Data
