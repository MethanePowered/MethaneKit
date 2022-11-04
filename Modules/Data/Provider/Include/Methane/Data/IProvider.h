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

FILE: Methane/Data/IProvider.h
Data provider interface used for loading application resources and resource files

******************************************************************************/

#pragma once

#include <Methane/Data/Chunk.hpp>

#include <string>
#include <vector>

namespace Methane::Data
{

struct IProvider
{
    virtual bool  HasData(const std::string& path) const noexcept = 0;
    virtual Chunk GetData(const std::string& path) const = 0;
    virtual std::vector<std::string> GetFiles(const std::string& directory) const = 0;

    virtual ~IProvider() = default;
};

} // namespace Methane::Data
