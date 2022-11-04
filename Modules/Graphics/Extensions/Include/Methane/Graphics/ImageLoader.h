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

FILE: Methane/Graphics/ImageLoader.h
Image Loader creates textures from images loaded via data provider and 
by decoding them from popular image formats.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/ITexture.h>
#include <Methane/Data/IProvider.h>

#include <magic_enum.hpp>
#include <string>
#include <array>

namespace Methane::Graphics
{

class ImageLoader final
{
public:
    enum class Options : uint32_t
    {
        None            = 0U,
        Mipmapped       = 1U << 0U,
        SrgbColorSpace  = 1U << 1U,
        All             = ~0U
    };

    class ImageData // NOSONAR
    {
    public:
        ImageData(const Dimensions& dimensions, uint32_t channels_count, Data::Chunk&& pixels) noexcept;
        ImageData(ImageData&& other) noexcept;
        ImageData(const ImageData& other) noexcept = delete;
        ~ImageData();

        [[nodiscard]] const Dimensions&  GetDimensions() const noexcept    { return m_dimensions; }
        [[nodiscard]] uint32_t           GetChannelsCount() const noexcept { return m_channels_count; }
        [[nodiscard]] const Data::Chunk& GetPixels() const noexcept        { return m_pixels; }

    private:
        Dimensions  m_dimensions;
        uint32_t    m_channels_count;
        Data::Chunk m_pixels;
        const bool  m_pixels_release_required;
    };

    enum class CubeFace : size_t
    {
        PositiveX = 0U,
        NegativeX,
        PositiveY,
        NegativeY,
        PositiveZ,
        NegativeZ
    };

    using CubeFaceResources = std::array<std::string, magic_enum::enum_count<CubeFace>()>;

    explicit ImageLoader(Data::IProvider& data_provider);

    [[nodiscard]] ImageData    LoadImage(const std::string& image_path, Data::Size channels_count, bool create_copy) const;
    [[nodiscard]] Ptr<ITexture> LoadImageToTexture2D(ICommandQueue& target_cmd_queue, const std::string& image_path, Options options = Options::None, const std::string& texture_name = "") const;
    [[nodiscard]] Ptr<ITexture> LoadImagesToTextureCube(ICommandQueue& target_cmd_queue, const CubeFaceResources& image_paths, Options options = Options::None, const std::string& texture_name = "") const;

private:
    Data::IProvider& m_data_provider;
};

} // namespace Methane::Graphics
