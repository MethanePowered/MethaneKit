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
Singleton data provider of the specific embedded CMRC resources
from RESOURCES_NAMESPACE or external resource files on disk.

******************************************************************************/

// Inclusion guard is missing intentionally because is included multiple times with different value of RESOURCE_PROVIDER

#include "FileProvider.hpp"

#include <Methane/Instrumentation.h>

#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>

#include <cmrc/cmrc.hpp>
CMRC_DECLARE(RESOURCE_NAMESPACE);

namespace RESOURCE_NAMESPACE
{

class ResourceProvider final : public Methane::Data::FileProvider
{
public:
    [[nodiscard]] static Provider& Get()
    {
        META_FUNCTION_TASK();
        static ResourceProvider s_instance;
        return s_instance;
    }

    [[nodiscard]] bool HasData(const std::string& path) const noexcept override
    {
        META_FUNCTION_TASK();
        if (m_resource_fs.exists(path))
            return true;

        return FileProvider::HasData(path);
    }

    [[nodiscard]] Methane::Data::Chunk GetData(const std::string& path) const override
    {
        META_FUNCTION_TASK();
        if (m_resource_fs.exists(path))
        {
            cmrc::file res_file = m_resource_fs.open(path);
            return Methane::Data::Chunk(reinterpret_cast<Methane::Data::ConstRawPtr>(res_file.cbegin()), // NOSONAR
                                        static_cast<Methane::Data::Size>(std::distance(res_file.cbegin(), res_file.cend())));
        }

        META_CHECK_ARG_DESCR(path, FileProvider::HasData(path), "invalid resource path '{}'", path);
        return FileProvider::GetData(path);
    }

    [[nodiscard]] std::vector<std::string> GetFiles(const std::string& directory_path) const override
    {
        META_FUNCTION_TASK();
        std::vector<std::string> file_paths;
        AddFilesInDirectory(directory_path, file_paths);
        return file_paths;
    }

private:
    ResourceProvider() = default;

    void AddFilesInDirectory(const std::string& directory_path, std::vector<std::string>& file_paths) const
    {
        META_FUNCTION_TASK();
        for(cmrc::directory_iterator dir_it = m_resource_fs.iterate_directory(directory_path); dir_it != dir_it.end(); dir_it++)
        {
            cmrc::directory_entry entry = *dir_it;
            std::string entry_path = directory_path + "/" + entry.filename();
            if (entry.is_directory())
                AddFilesInDirectory(entry_path, file_paths);
            else
                file_paths.emplace_back(std::move(entry_path));
        }
    }

    cmrc::embedded_filesystem m_resource_fs = cmrc::RESOURCE_NAMESPACE::get_filesystem();
};

} // namespace RESOURCE_NAMESPACE