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

FILE: Methane/Data/Types.h
Common Methane primitive data types
u
******************************************************************************/

#pragma once

#include "Point.hpp"
#include "Rect.hpp"

#include <cstdint>
#include <vector>

namespace Methane::Data
{

enum class MemoryState : uint32_t
{
    Reserved = 0U,
    Initialized,
};

using Timestamp = uint64_t;
using TimeDelta = int64_t;
using Frequency = Timestamp;
using Bytes = std::vector<uint8_t>;
using Size = uint32_t;
using Index = Size;
using RawPtr = uint8_t*;
using ConstRawPtr = const uint8_t* const;

} // namespace Methane::Data
