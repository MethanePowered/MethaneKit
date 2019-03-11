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

FILE: Methane/Graphics/ImageLoader.cpp
Image Loader creates textures from images loaded via data provider and
by decoding them from popular image formats.

******************************************************************************/

#include <Methane/Graphics/ImageLoader.h>
#include <Methane/Platform/Utils.h>

#ifdef USE_OPEN_IMAGE_IO

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/filesystem.h>

#else

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include <stb_image.h>

#endif

using namespace Methane;
using namespace Methane::Graphics;

ImageLoader::ImageLoader(Data::Provider& data_provider)
    : m_data_provider(data_provider)
{
}

Texture::Ptr ImageLoader::CreateImageTexture(Context& context, const std::string& image_path)
{
    const size_t texture_channels_count = 4;

#ifdef USE_OPEN_IMAGE_IO

#if 0
    Data::Chunk image_data = m_data_provider.GetData(Data::Provider::Type::Texture, image_path);
    OIIO::Filesystem::IOMemReader image_reader(const_cast<char*>(image_data.p_data), image_data.size);
    OIIO::ImageSpec init_spec;
    init_spec.attribute("oiio:ioproxy", OIIO::TypeDesc::PTR, &image_reader);
    OIIO::ImageBuf image_buf(init_spec);
#else
    const std::string image_file_path = Platform::GetResourceDir() + "/" + image_path;
    OIIO::ImageBuf image_buf(image_file_path.c_str());
#endif

    // Read image format with general information
    const OIIO::ImageSpec& image_spec = image_buf.spec();
    if (image_spec.undefined() || !image_buf.read())
    {
        throw std::runtime_error("Failed to read image from file \"" + image_path + "\", error: " + image_buf.geterror());
    }

    // Convert image pixels data to the target texture format RGBA8 Unorm
    OIIO::ROI image_roi = OIIO::get_roi(image_spec);
    std::vector<uint8_t> texture_data(texture_channels_count * image_roi.npixels(), 255);
    const OIIO::TypeDesc texture_format(OIIO::TypeDesc::BASETYPE::UCHAR);
    if (!image_buf.get_pixels(image_roi, texture_format, texture_data.data(), texture_channels_count * sizeof(texture_data[0])))
    {
        throw std::runtime_error("Failed to decode image from file \"" + image_path + "\", error: " + image_buf.geterror());
    }

    // Create texture for loaded image and set image data (with uploading it to the GPU)
    const Dimensions image_dimensions(static_cast<uint32_t>(image_spec.width), static_cast<uint32_t>(image_spec.height), 1);
    Texture::Ptr sp_texture = Texture::CreateImage(context, image_dimensions, PixelFormat::RGBA8Unorm, false);
    sp_texture->SetData(reinterpret_cast<Data::ConstRawPtr>(texture_data.data()), static_cast<Data::Size>(texture_data.size()));

    return sp_texture;

#else

    Data::Chunk image_chunk = m_data_provider.GetData(Data::Provider::Type::Texture, image_path);
    int image_width = 0, image_height = 0, image_channels_count = 0;
    stbi_uc* p_image_data = stbi_load_from_memory(reinterpret_cast<stbi_uc const*>(image_chunk.p_data),
                                                    static_cast<int>(image_chunk.size),
                                                    &image_width, &image_height, &image_channels_count,
                                                    static_cast<int>(texture_channels_count));
    const Data::Size image_data_size = static_cast<Data::Size>(image_width * image_height) * texture_channels_count * sizeof(stbi_uc);

    if (!p_image_data || !image_width || !image_height || !image_channels_count)
    {
        throw std::runtime_error("Failed to decode image from memory file \"" + image_path + "\".");
    }

    // Create texture for loaded image and set image data (with uploading it to the GPU)
    const Dimensions image_dimensions(static_cast<uint32_t>(image_width), static_cast<uint32_t>(image_height), 1);
    Texture::Ptr sp_texture = Texture::CreateImage(context, image_dimensions, PixelFormat::RGBA8Unorm, false);
    sp_texture->SetData(reinterpret_cast<Data::ConstRawPtr>(p_image_data), image_data_size);

    stbi_image_free(p_image_data);
    return sp_texture;

#endif

}
