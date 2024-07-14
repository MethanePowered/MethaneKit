/******************************************************************************

Copyright 2024 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/RootConstant.h
Methane root constant value, used to set program argument binding value directly

******************************************************************************/

#pragma once

#include <Methane/Data/Chunk.hpp>

namespace Methane::Graphics::Rhi
{

class RootConstant
    : public Data::Chunk
{
public:
    RootConstant() = default;

    RootConstant(Data::ConstRawPtr data_ptr, Data::Size size) noexcept
        : Data::Chunk(data_ptr, size)
    { }

    template<typename T>
    explicit RootConstant(T&& value)
        : Data::Chunk(std::forward<T>(value))
    { }

    template<typename T>
    const T& GetValue() const
    {
        META_CHECK_ARG_EQUAL_DESCR(sizeof(T), Data::Chunk::GetDataSize(),
                                   "size of value type does not match with root constant data size");
        return reinterpret_cast<const T&>(Data::Chunk::GetDataPtr()); // NOSONAR
    }
};

} // namespace Methane::Graphics::Rhi
