/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/ResourceProvider.hpp
Singleton data provider of files on disk.

******************************************************************************/

#pragma once

#include "Provider.h"

#include <Methane/Platform/Utils.h>
#include <Methane/Instrumentation.h>

#include <string>
#include <fstream>
#include <stdexcept>

namespace Methane::Data
{

class FileProvider : public Provider
{
public:
    static Provider& Get()
    {
        ITT_FUNCTION_TASK();
        static FileProvider s_instance;
        return s_instance;
    }

    bool HasData(const std::string& path) const noexcept override
    {
        ITT_FUNCTION_TASK();
        return std::ifstream(GetDataFilePath(path)).good();
    }

    Data::Chunk GetData(const std::string& path) const override
    {
        ITT_FUNCTION_TASK();

        const std::string file_path = GetDataFilePath(path);
        std::ifstream fs(file_path, std::ios::binary);
        if (!fs.good())
        {
            throw std::invalid_argument("File path does not exist: " + file_path);
        }
        return Data::Chunk(Data::Bytes(std::istreambuf_iterator<char>(fs), {}));
    }

protected:

    std::string GetDataFilePath(const std::string& path) const
    {
        ITT_FUNCTION_TASK();
        return m_resources_dir + "/" + path;
    }
    
    FileProvider()
        : m_resources_dir(Platform::GetResourceDir())
    {
        ITT_FUNCTION_TASK();
    }

    const std::string m_resources_dir;
};

} // namespace Methane::Graphics
