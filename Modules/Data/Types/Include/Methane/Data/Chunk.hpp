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

namespace Methane::Data
{

class Chunk
{
public:
    Chunk() noexcept = default;
    Chunk(ConstRawPtr in_p_data, Size in_size) noexcept
        : m_data_ptr(in_p_data)
        , m_data_size(in_size)
    { }

    explicit Chunk(Bytes&& in_data) noexcept
        : m_data_storage(std::move(in_data))
        , m_data_ptr(m_data_storage.empty() ? nullptr : m_data_storage.data())
        , m_data_size(static_cast<Size>(m_data_storage.size()))
    { }

    explicit Chunk(Chunk&& other) noexcept
        : m_data_storage(std::move(other.m_data_storage))
        , m_data_ptr(m_data_storage.empty() ? other.m_data_ptr : m_data_storage.data())
        , m_data_size(m_data_storage.empty() ? other.m_data_size : static_cast<Size>(m_data_storage.size()))
    { }

    ~Chunk() = default;

    ConstRawPtr GetDataPtr() const noexcept    { return m_data_ptr; }
    ConstRawPtr GetDataEndPtr() const noexcept { return m_data_ptr + m_data_size; }
    Size        GetDataSize() const noexcept   { return m_data_size; }
    bool        IsEmptyOrNull() const noexcept { return !m_data_ptr || !m_data_size; }
    bool        IsDataStored() const noexcept  { return !m_data_storage.empty(); }

protected:
    explicit Chunk(const Chunk& other) noexcept
        : m_data_storage(other.m_data_storage)
        , m_data_ptr(m_data_storage.empty() ? other.m_data_ptr : m_data_storage.data())
        , m_data_size(m_data_storage.empty() ? other.m_data_size : static_cast<Size>(m_data_storage.size()))
    { }

private:
    // Data storage is used only when m_data_storage is not managed by m_data_storage provider and
    // returned with chunk (when m_data_storage is loaded from file, for example)
    Bytes             m_data_storage;
    const ConstRawPtr m_data_ptr  = nullptr;
    const Size        m_data_size = 0U;
};

} // namespace Methane::Data
