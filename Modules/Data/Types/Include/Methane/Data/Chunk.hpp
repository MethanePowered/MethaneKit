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

FILE: Methane/Data/Chunk.h
Data chunk representing owning or non-owning memory container

******************************************************************************/

#pragma once

#include "Types.h"

#include <type_traits>

namespace Methane::Data
{

class Chunk // NOSONAR - rule of zero is not applicable
{
public:
    Chunk() = default;
    Chunk(ConstRawPtr data_ptr, Size size) noexcept
        : m_data_ptr(data_ptr)
        , m_data_size(size)
    { }

    explicit Chunk(Bytes&& data) noexcept
        : m_data_storage(std::move(data))
        , m_data_ptr(m_data_storage.empty() ? nullptr : m_data_storage.data())
        , m_data_size(static_cast<Size>(m_data_storage.size()))
    { }

    template<typename T,
             typename = std::enable_if_t<!std::is_base_of_v<Chunk, std::remove_cv_t<std::remove_reference_t<T>>>>>
    explicit Chunk(T& value)
        : m_data_ptr(GetByteAddress(value))
        , m_data_size(static_cast<Size>(sizeof(T)))
    { }

    template<typename T,
             typename = std::enable_if_t<!std::is_base_of_v<Chunk, std::remove_cv_t<std::remove_reference_t<T>>>>>
    explicit Chunk(T&& value)
        : m_data_storage(GetByteAddress(std::forward<T>(value)),
                         GetByteAddress(std::forward<T>(value)) + sizeof(T))
        , m_data_ptr(m_data_storage.data())
        , m_data_size(static_cast<Size>(m_data_storage.size()))
    { }

    explicit Chunk(const Chunk& other)
        : m_data_storage(other.m_data_storage)
        , m_data_ptr(m_data_storage.empty() ? other.m_data_ptr : m_data_storage.data())
        , m_data_size(m_data_storage.empty() ? other.m_data_size : static_cast<Size>(m_data_storage.size()))
    { }

    explicit Chunk(Chunk&& other) noexcept
        : m_data_storage(std::move(other.m_data_storage))
        , m_data_ptr(m_data_storage.empty() ? other.m_data_ptr : m_data_storage.data())
        , m_data_size(m_data_storage.empty() ? other.m_data_size : static_cast<Size>(m_data_storage.size()))
    { }

    Chunk& operator=(const Chunk& other) noexcept
    {
        m_data_storage = other.m_data_storage;
        m_data_ptr     = m_data_storage.empty() ? other.m_data_ptr : m_data_storage.data();
        m_data_size    = m_data_storage.empty() ? other.m_data_size : static_cast<Size>(m_data_storage.size());
        return *this;
    }

    Chunk& operator=(Chunk&& other) noexcept
    {
        m_data_storage = std::move(other.m_data_storage);
        m_data_ptr     = m_data_storage.empty() ? other.m_data_ptr : m_data_storage.data();
        m_data_size    = m_data_storage.empty() ? other.m_data_size : static_cast<Size>(m_data_storage.size());
        return *this;
    }

    friend bool operator==(const Chunk& left, const Chunk& right) noexcept
    {
        return left.m_data_size == right.m_data_size &&
               (left.m_data_ptr == right.m_data_ptr ||
                memcmp(left.m_data_ptr, right.m_data_ptr, left.m_data_size) == 0);
    }

    friend bool operator!=(const Chunk& left, const Chunk& right) noexcept
    {
        return !(left == right);
    }

    explicit operator bool() const noexcept
    {
        return !IsEmptyOrNull();
    }

    [[nodiscard]] bool IsEmptyOrNull() const noexcept { return !m_data_ptr || !m_data_size; }
    [[nodiscard]] bool IsDataStored() const noexcept  { return !m_data_storage.empty(); }

    static Chunk StoreFrom(const Chunk& other)
    {
        return Chunk(Bytes(other.GetDataPtr(), other.GetDataEndPtr()));
    }

    template<typename T = Byte>
    [[nodiscard]] Size GetDataSize() const noexcept
    {
        if constexpr (std::is_same_v<T, Byte>)
            return m_data_size;
        else
            return m_data_size / sizeof(T);
    }

    template<typename T = Byte>
    [[nodiscard]] const T* GetDataPtr() const noexcept
    {
        if constexpr (std::is_same_v<T, Byte>)
            return m_data_ptr;
        else
            return reinterpret_cast<const T*>(m_data_ptr); // NOSONAR
    }

    template<typename T = Byte>
    [[nodiscard]] const T* GetDataEndPtr() const noexcept
    {
        return GetDataPtr<T>() + GetDataSize<T>();
    }

private:
    template<typename T>
    static const std::byte* GetByteAddress(const T& value)
    {
        return reinterpret_cast<const std::byte*>(std::addressof(value)); // NOSONAR
    }

    // Data storage is used only when m_data_storage is not managed by m_data_storage provider and
    // returned with chunk (when m_data_storage is loaded from file, for example)
    Bytes       m_data_storage;
    ConstRawPtr m_data_ptr  = nullptr;
    Size        m_data_size = 0U;
};

} // namespace Methane::Data
