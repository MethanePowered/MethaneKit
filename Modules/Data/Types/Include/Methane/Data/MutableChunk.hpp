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

FILE: Methane/Data/MutableChunk.h
Data chunk representing owning or non-owning memory container

******************************************************************************/

#pragma once

#include "Chunk.hpp"

#include <Methane/Exceptions.hpp>

namespace Methane::Data
{

class MutableChunk // NOSONAR - custom copy and move constructors are required here.
{
public:
    MutableChunk(ConstRawPtr data_ptr, Size size) noexcept
        : m_data(data_ptr, data_ptr + size)
        , m_chunk(m_data.data(), static_cast<Size>(m_data.size()))
    { }

    explicit MutableChunk(Bytes&& data) noexcept
        : m_data(std::move(data))
        , m_chunk(m_data.data(), static_cast<Size>(m_data.size()))
    { }

    explicit MutableChunk(const Chunk& chunk) noexcept
        : m_data(chunk.GetDataPtr(), chunk.GetDataEndPtr())
        , m_chunk(m_data.data(), static_cast<Size>(m_data.size()))
    { }

    explicit MutableChunk(MutableChunk&& other) noexcept
        : m_data(std::move(other.m_data))
        , m_chunk(m_data.data(), static_cast<Size>(m_data.size()))
    { }

    MutableChunk(const MutableChunk& other) noexcept
        : m_data(other.m_data)
        , m_chunk(m_data.data(), static_cast<Size>(m_data.size()))
    { }

    ~MutableChunk() = default;

    MutableChunk& operator=(const MutableChunk&) noexcept = delete;
    MutableChunk& operator=(MutableChunk&&) noexcept = delete;

    [[nodiscard]] const Chunk& AsConstChunk() const noexcept
    {
        return m_chunk;
    }

    template<typename T = Byte>
    [[nodiscard]] Size GetDataSize() const noexcept
    {
        if constexpr (sizeof(T) == 1)
            return static_cast<Size>(m_data.size());
        else
            return static_cast<Size>(m_data.size() / sizeof(T));
    }

    template<typename T = Byte>
    [[nodiscard]] T* GetDataPtr() noexcept
    {
        if constexpr (std::is_same_v<T, Byte>)
            return m_data.data();
        else
            return reinterpret_cast<T*>(m_data.data()); // NOSONAR
    }

    template<typename T = Byte>
    [[nodiscard]] T* GetDataEndPtr() const noexcept
    {
        return GetDataPtr<T>() + GetDataSize<T>();
    }

    template<typename T = char, typename V = char>
    void PatchData(T offset, V value)
    {
        const T data_size = GetDataSize<T>();
        META_CHECK_ARG_LESS_DESCR(offset, data_size, "can not patch data with offset outside of bounds");

        if constexpr (std::is_same_v<T, V>)
            *(GetDataPtr<T>() + offset) = value;
        else
            *reinterpret_cast<V*>(GetDataPtr<T>() + offset) = value; // NOSONAR
    }

private:
    Bytes m_data;
    Chunk m_chunk;
};

} // namespace Methane::Data
