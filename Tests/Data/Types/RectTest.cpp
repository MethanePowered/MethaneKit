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

FILE: Tests/Data/Types/RectTest.cpp
Unit-tests of the Rect data type

******************************************************************************/

#include "TestHelpers.hpp"

#include <Methane/Data/Rect.hpp>

#include <catch2/catch.hpp>
#include <sstream>

using namespace Methane::Data;

#define RECT_TYPES_MATRIX \
    ((typename T, typename S), T, S), \
    (int32_t,  int32_t),  (int32_t,  uint32_t),  (int32_t,  float),  (int32_t,  double),  \
    (uint32_t,  int32_t), (uint32_t,  uint32_t), (uint32_t,  float), (uint32_t,  double), \
    (float,  int32_t),    (float,  uint32_t),    (float,  float),    (float,  double),    \
    (double,  int32_t),   (double,  uint32_t),   (double,  float),   (double,  double)    \

