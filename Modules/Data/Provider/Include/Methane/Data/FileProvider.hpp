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

FILE: Methane/Graphics/ResourceProvider.hpp
Singleton data provider of files on disk.

******************************************************************************/

#pragma once

#include "IProvider.h"

#include <Methane/Platform/Utils.h>
#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>

#include <string>
#include <fstream>
#include <stdexcept>
#include <regex>

namespace Methane::Data
{

class FileProvider : public IProvider
{
public:
    [[nodiscard]] static IProvider& Get()
    {
        META_FUNCTION_TASK();
        static FileProvider s_instance;
        return s_instance;
    }

    [[nodiscard]] bool HasData(const std::string& path) const noexcept override
    {
        META_FUNCTION_TASK();
        return std::ifstream(GetFullFilePath(path)).good();
    }

    [[nodiscard]] Data::Chunk GetData(const std::string& path) const override
    {
        META_FUNCTION_TASK();

        const std::string file_path = GetFullFilePath(path);
        std::ifstream fs(file_path, std::ios::binary);
        META_CHECK_DESCR(path, fs.good(), "File path does not exist '{}'", file_path);

        fs.seekg(0,std::ios::end);
        Data::Bytes buffer(static_cast<size_t>(fs.tellg()), {});

        fs.seekg(0,std::ios::beg);
        fs.read(reinterpret_cast<char*>(buffer.data()), buffer.size()); // NOSONAR

        return Data::Chunk(std::move(buffer));
    }

    [[nodiscard]] std::vector<std::string> GetFiles(const std::string&) const override
    {
        META_FUNCTION_TASK();
        return { };
    }

protected:
    FileProvider() = default;

    [[nodiscard]] std::string GetFullFilePath(const std::string& path) const
    {
        META_FUNCTION_TASK();
#ifdef _WIN32
        static const std::string path_delimiter = "\\";
        static const std::regex root_path_regex(R"(^[a-zA-Z]\:[\\|/].*)");
#else
        static const std::string path_delimiter = "/";
        const std::regex root_path_regex("^/.*");
#endif
        const bool is_root_path = std::regex_match(path, root_path_regex);
        return is_root_path ? path : m_resources_dir + path_delimiter + path;
    }

    const std::string m_resources_dir = Platform::GetResourceDir();
};

} // namespace Methane::Graphics
