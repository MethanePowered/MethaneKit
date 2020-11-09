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

FILE: Methane/Graphics/Point.hpp
3D Point type based on cml::vector

******************************************************************************/

#pragma once

#include <Methane/Data/Point.hpp>

namespace Methane::Graphics
{

template<typename T>
using Point2T = Data::PointT<T, 2>;

using Point2i = Point2T<int32_t>;
using Point2u = Point2T<uint32_t>;
using Point2f = Point2T<float>;
using Point2d = Point2T<double>;

template<typename T>
using Point3T = Data::PointT<T, 3>;

using Point3i = Point3T<int32_t>;
using Point3u = Point3T<uint32_t>;
using Point3f = Point3T<float>;
using Point3d = Point3T<double>;

} // namespace Methane::Graphics