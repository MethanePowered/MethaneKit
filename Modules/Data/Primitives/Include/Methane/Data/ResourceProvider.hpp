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
Singleton data provider of the specific embedded CMRC resources
from RESOURCES_NAMESPACE or external resource files on disk.

******************************************************************************/

// FIXME: add inclusion guard, depending on RESOURCE_PROVIDER

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

class ResourceProvider : public Methane::Data::FileProvider
{
public:
    static Provider& Get()
    {
        ITT_FUNCTION_TASK();
        static ResourceProvider s_instance;
        return s_instance;
    }

    bool HasData(const std::string& path) const noexcept override
    {
        ITT_FUNCTION_TASK();

        if (m_resource_fs.exists(path))
            return true;

        return FileProvider::HasData(path);
    }

    Methane::Data::Chunk GetData(const std::string& path) const override
    {
        ITT_FUNCTION_TASK();

        if (m_resource_fs.exists(path))
        {
            cmrc::file res_file = m_resource_fs.open(path);
            return Methane::Data::Chunk(reinterpret_cast<Methane::Data::ConstRawPtr>(res_file.cbegin()),
                                        static_cast<Methane::Data::Size>(res_file.cend() - res_file.cbegin()));
        }

        if (FileProvider::HasData(path))
        {
            return FileProvider::GetData(path);
        }
        else
        {
            throw std::invalid_argument("Invalid resource path: " + path);
        }
    }

protected:
    
    ResourceProvider()
        : m_resource_fs(cmrc::RESOURCE_NAMESPACE::get_filesystem())
    {
        ITT_FUNCTION_TASK();
    }

    cmrc::embedded_filesystem m_resource_fs;
};

} // namespace RESOURCE_NAMESPACE