/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Data/Provider.h
Data provider interface used for loading application resources and resource files

******************************************************************************/

#pragma once

#include "Types.h"

namespace Methane::Data
{

struct Chunk
{
    // NOTE: Data storage is used only when data is not managed by data provider and returned with chunk (when data is loaded from file, for example)
    const Bytes data;
    ConstRawPtr p_data = nullptr;
    const Size  size   = 0;

    Chunk() = default;
    Chunk(ConstRawPtr in_p_data, Size in_size) : p_data(in_p_data), size(in_size) { }
    Chunk(const Bytes&& in_data) : data(std::move(in_data)), p_data(static_cast<ConstRawPtr>(data.data())), size(static_cast<Size>(data.size())) { }
    Chunk(const Chunk&& other)   : data(std::move(other.data)), p_data(data.empty() ? other.p_data : data.data()), size(data.empty() ? other.size : data.size()) { }
};

struct Provider
{
    enum class Type
    {
        Shader,
        Texture,
        Other,
    };

    virtual bool  HasData(Type data_type, const std::string& path) const noexcept = 0;
    virtual Chunk GetData(Type data_type, const std::string& path) const = 0;

    virtual ~Provider() = default;
};

} // namespace Methane::Data
