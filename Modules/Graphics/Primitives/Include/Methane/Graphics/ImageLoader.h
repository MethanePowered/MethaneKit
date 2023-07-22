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
#include <Methane/Graphics/RHI/Texture.h>
#include <Methane/Data/IProvider.h>
#include <Methane/Data/EnumMask.hpp>

#include <string>
#include <array>

namespace Methane::Graphics
{

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

enum class ImageOption : uint32_t
{
    Mipmapped,
    SrgbColorSpace,
};

using ImageOptionMask = Data::EnumMask<ImageOption>;

class ImageLoader final
{
public:
    using Option     = ImageOption;
    using OptionMask = ImageOptionMask;

    enum class CubeFace : size_t
    {
        PositiveX = 0U,
        NegativeX,
        PositiveY,
        NegativeY,
        PositiveZ,
        NegativeZ,

        Count
    };

    using CubeFaceResources = std::array<std::string, static_cast<size_t>(CubeFace::Count)>;

    explicit ImageLoader(Data::IProvider& data_provider);

    [[nodiscard]] ImageData    LoadImageData(const std::string& image_path, Data::Size channels_count, bool create_copy) const;
    [[nodiscard]] Rhi::Texture LoadImageToTexture2D(const Rhi::CommandQueue& target_cmd_queue, const std::string& image_path, ImageOptionMask options = {}, const std::string& texture_name = "") const;
    [[nodiscard]] Rhi::Texture LoadImagesToTextureCube(const Rhi::CommandQueue& target_cmd_queue, const CubeFaceResources& image_paths, ImageOptionMask options = {}, const std::string& texture_name = "") const;

private:
    Data::IProvider& m_data_provider;
};

} // namespace Methane::Graphics
