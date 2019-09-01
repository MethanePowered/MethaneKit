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

FILE: Methane/Graphics/AppDataProvider.hpp
Singleton data provider implementing acess to the embedded application resources
and external resource files on disk.

******************************************************************************/

#pragma once

#include <Methane/Data/Provider.h>
#include <Methane/Platform/Utils.h>
#include <Methane/Instrumentation.h>

#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <cassert>

#if defined(ENABLE_SHADER_RESOURCES) || defined(ENABLE_TEXTURE_RESOURCES)
    #include <cmrc/cmrc.hpp>
    #define ENABLE_DATA_RESOURCES
#endif

#ifdef ENABLE_SHADER_RESOURCES
    CMRC_DECLARE(Shaders);
#endif

#ifdef ENABLE_TEXTURE_RESOURCES
    CMRC_DECLARE(Textures);
#endif

namespace Methane
{
namespace Graphics
{

class AppDataProvider : public Data::Provider
{
public:
    static Provider& Get()
    {
        ITT_FUNCTION_TASK();
        static AppDataProvider s_instance;
        return s_instance;
    }

    bool HasData(Type data_type, const std::string& path) const noexcept override
    {
        ITT_FUNCTION_TASK();
        switch(data_type)
        {
#ifdef ENABLE_SHADER_RESOURCES
        case Type::Shader:  return HasResourceData(m_shaders_fs, path);
#endif
#ifdef ENABLE_TEXTURE_RESOURCES
        case Type::Texture: return HasResourceData(m_textures_fs, path);
#endif
        default: return HasFileData(path);
        }
    }

    Data::Chunk GetData(Type data_type, const std::string& path) const override
    {
        ITT_FUNCTION_TASK();
        switch(data_type)
        {
#ifdef ENABLE_SHADER_RESOURCES
        case Type::Shader:  return GetResourceData(m_shaders_fs, path);
#endif
#ifdef ENABLE_TEXTURE_RESOURCES
        case Type::Texture: return GetResourceData(m_textures_fs, path);
#endif
        default: return GetFileData(path);
        }
    }

private:

#ifdef ENABLE_DATA_RESOURCES
    bool HasResourceData(const cmrc::embedded_filesystem& fs, const std::string& path) const noexcept
    {
        ITT_FUNCTION_TASK();
        return fs.exists(path);
    }

    Data::Chunk GetResourceData(const cmrc::embedded_filesystem& fs, const std::string& path) const
    {
        ITT_FUNCTION_TASK();
        if (!fs.exists(path))
            throw std::invalid_argument("Invalid resource path: " + path);

        cmrc::file res_file = fs.open(path);
        return Data::Chunk(static_cast<Data::ConstRawPtr>(res_file.cbegin()), static_cast<Data::Size>(res_file.cend() - res_file.cbegin()));
    }
#endif

    static std::string GetDataFilePath(const std::string& path)
    {
        ITT_FUNCTION_TASK();
        return Platform::GetResourceDir() + "/" + path;
    }

    bool HasFileData(const std::string& path) const noexcept
    {
        ITT_FUNCTION_TASK();
        return std::ifstream(GetDataFilePath(path)).good();
    }

    Data::Chunk GetFileData(const std::string& path) const
    {
        ITT_FUNCTION_TASK();
        const std::string file_path = GetDataFilePath(path);
        std::ifstream fs(file_path, std::ios::binary);
        if (!fs.good())
        {
            throw std::invalid_argument("Resource file path does not exist: " + file_path);
        }
        return Data::Chunk(Data::Bytes(std::istreambuf_iterator<Data::Bytes::value_type>(fs), {}));
    }
    
    AppDataProvider()
        : m_initialized(true) // NOTE: used to be the first in constructor's initializer list
#ifdef ENABLE_SHADER_RESOURCES
        , m_shaders_fs(cmrc::Shaders::get_filesystem())
#endif
#ifdef ENABLE_TEXTURE_RESOURCES
        , m_textures_fs(cmrc::Textures::get_filesystem())
#endif
    {
        ITT_FUNCTION_TASK();
        (void) m_initialized; // NOTE: silence unused variable warning
    }

    const bool m_initialized; // NOTE: this member variable is required to be the first in constructor's list of initialization
#ifdef ENABLE_SHADER_RESOURCES
    cmrc::embedded_filesystem m_shaders_fs;
#endif
#ifdef ENABLE_TEXTURE_RESOURCES
    cmrc::embedded_filesystem m_textures_fs;
#endif
};

} // namespace Graphics
} // namespace Methane
