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
#include <Methane/Instrumentation.h>

#ifdef USE_OPEN_IMAGE_IO

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/filesystem.h>

#else

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include <stb_image.h>

#endif

namespace Methane::Graphics
{

ImageLoader::ImageData::ImageData(const Dimensions& in_dimensions, uint32_t in_channels_count, const Data::Chunk&& in_pixels)
    : dimensions(in_dimensions)
    , channels_count(in_channels_count)
    , pixels(std::move(in_pixels))
{
    ITT_FUNCTION_TASK();
}

ImageLoader::ImageData::ImageData(const ImageData&& other)
    : dimensions(std::move(other.dimensions))
    , channels_count(other.channels_count)
    , pixels(std::move(other.pixels))
{
    ITT_FUNCTION_TASK();
}

ImageLoader::ImageData::~ImageData()
{
    ITT_FUNCTION_TASK();

#ifndef USE_OPEN_IMAGE_IO
    if (pixels.data.empty() && pixels.p_data)
    {
        // We assume that image data was loaded with STB load call and was not copied to container, so it must be freed
        stbi_image_free(const_cast<Data::RawPtr>(pixels.p_data));
    }
#endif
}

ImageLoader::ImageLoader(Data::Provider& data_provider)
    : m_data_provider(data_provider)
{
    ITT_FUNCTION_TASK();
}

ImageLoader::ImageData ImageLoader::LoadImage(const std::string& image_path, size_t channels_count, bool create_copy)
{
    ITT_FUNCTION_TASK();

    Data::Chunk raw_image_data = m_data_provider.GetData(image_path);

#ifdef USE_OPEN_IMAGE_IO

#if 0
    OIIO::Filesystem::IOMemReader image_reader(const_cast<char*>(raw_image_data.p_data), raw_image_data.size);
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
    Data::Bytes texture_data(channels_count * image_roi.npixels(), 255);
    const OIIO::TypeDesc texture_format(OIIO::TypeDesc::BASETYPE::UCHAR);
    if (!image_buf.get_pixels(image_roi, texture_format, texture_data.data(), channels_count * sizeof(texture_data[0])))
    {
        throw std::runtime_error("Failed to decode image from file \"" + image_path + "\", error: " + image_buf.geterror());
    }

    return ImageData(Dimensions(static_cast<uint32_t>(image_spec.width), static_cast<uint32_t>(image_spec.height)),
                                static_cast<uint32_t>(channels_count),
                                Data::Chunk(std::move(texture_data)));

#else
    int image_width = 0, image_height = 0, image_channels_count = 0;
    stbi_uc* p_image_data = stbi_load_from_memory(reinterpret_cast<stbi_uc const*>(raw_image_data.p_data),
                                                  static_cast<int>(raw_image_data.size),
                                                  &image_width, &image_height, &image_channels_count,
                                                  static_cast<int>(channels_count));

    if (!p_image_data || image_width <= 0 || image_height <= 0 || image_channels_count <= 0)
    {
        throw std::runtime_error("Failed to decode image from memory file \"" + image_path + "\".");
    }

    const Dimensions image_dimensions(static_cast<uint32_t>(image_width), static_cast<uint32_t>(image_height));
    const Data::Size image_data_size = static_cast<Data::Size>(image_width * image_height) * channels_count * sizeof(stbi_uc);

    if (create_copy)
    {
        Data::RawPtr p_image_raw_data = reinterpret_cast<Data::RawPtr>(p_image_data);
        Data::Bytes image_data_copy(p_image_raw_data, p_image_raw_data + image_data_size);
        ImageData image_data(image_dimensions, static_cast<uint32_t>(image_channels_count), Data::Chunk(std::move(image_data_copy)));
        stbi_image_free(p_image_data);
        return image_data;
    }
    else
    {
        return ImageData(image_dimensions, static_cast<uint32_t>(image_channels_count),
                         Data::Chunk(reinterpret_cast<Data::ConstRawPtr>(p_image_data), image_data_size));
    }

#endif
}

Texture::Ptr ImageLoader::LoadImageToTexture2D(Context& context, const std::string& image_path)
{
    ITT_FUNCTION_TASK();

    const ImageData image_data = LoadImage(image_path, 4, false);
    Texture::Ptr sp_texture = Texture::CreateImage(context, image_data.dimensions, 1, PixelFormat::RGBA8Unorm, false);
    sp_texture->SetData({ { image_data.pixels.p_data, image_data.pixels.size } });

    return sp_texture;
}

Texture::Ptr ImageLoader::LoadImagesToTextureCube(Context& context, const CubeFaceResources& image_paths)
{
    ITT_FUNCTION_TASK();

    std::vector<ImageData> face_resources_data;
    face_resources_data.reserve(image_paths.size());

    Resource::SubResources face_resources;
    face_resources.reserve(image_paths.size());

    const uint32_t desired_channels_count = 4;
    uint32_t       face_channels_count = 0;
    Dimensions     face_dimensions;
    uint32_t       face_slice = 0;

    for (const std::string& image_path : image_paths)
    {
        // NOTE:
        //  we create a copy of the loaded image data (via 3-rd argument of LoadImage)
        //  to resolve a problem of STB image loader which requires an image data to be freed before next image is loaded
        face_resources_data.emplace_back(LoadImage(image_path, desired_channels_count, true));
        const ImageData& image_data = face_resources_data.back();

        if (face_slice == 0)
        {
            face_dimensions     = image_data.dimensions;
            face_channels_count = image_data.channels_count;
            if (face_dimensions.width != face_dimensions.height)
            {
                throw std::runtime_error("All images of cube texture faces must have equal width and height.");
            }
        }
        else if (face_dimensions     != image_data.dimensions ||
                 face_channels_count != image_data.channels_count)
        {
            throw std::runtime_error("All face image of cube texture must have equal dimensions and channels count.");
        }

        face_resources.emplace_back(image_data.pixels.p_data, image_data.pixels.size, face_slice);
        face_slice++;
    }

    Texture::Ptr sp_texture = Texture::CreateCube(context, face_dimensions.width, 1, PixelFormat::RGBA8Unorm, false);
    sp_texture->SetData(face_resources);

    return sp_texture;
}

} // namespace Methane::Graphics
