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

FILE: Methane/Graphics/Base/Texture.cpp
Base implementation of the texture interface.

******************************************************************************/

#include <Methane/Graphics/Base/Texture.h>
#include <Methane/Graphics/Base/RenderContext.h>

#include <Methane/Graphics/RHI/TypeFormatters.hpp>
#include <Methane/Graphics/TypeFormatters.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics::Base
{

Texture::Texture(const Context& context, const Settings& settings,
                 State initial_state, Opt<State> auto_transition_source_state_opt)
    : Resource(context, Rhi::IResource::Type::Texture, settings.usage_mask, initial_state, auto_transition_source_state_opt)
    , m_settings(settings)
    , m_sub_resource_count(
        settings.dimensions.GetDepth(),
        settings.array_length,
        settings.mipmapped ? GetRequiredMipLevelsCount(settings.dimensions) : 1U
    )
{
    META_FUNCTION_TASK();
    META_CHECK_NOT_EQUAL_DESCR(m_settings.usage_mask.GetValue(), 0U, "can not create texture with 'Unknown' usage mask");
    META_CHECK_NOT_EQUAL_DESCR(m_settings.pixel_format, PixelFormat::Unknown, "can not create texture with 'Unknown' pixel format");
    META_CHECK_NOT_NULL_DESCR(m_settings.array_length, "array length should be greater than zero");

    ValidateDimensions(m_settings.dimension_type, m_settings.dimensions, m_settings.mipmapped);

    const Data::Size subresource_raw_count = m_sub_resource_count.GetRawCount();
    m_sub_resource_sizes.reserve(subresource_raw_count);
    for (Data::Index subresource_raw_index = 0U; subresource_raw_index < subresource_raw_count; ++subresource_raw_index)
    {
        const SubResource::Index subresource_index(subresource_raw_index, m_sub_resource_count);
        m_sub_resource_sizes.emplace_back(CalculateSubResourceDataSize(subresource_index));
    }
}

void Texture::ValidateDimensions(DimensionType dimension_type, const Dimensions& dimensions, bool mipmapped)
{
    META_FUNCTION_TASK();
    META_UNUSED(mipmapped);
    META_CHECK_NOT_ZERO_DESCR(dimensions, "all dimension sizes should be greater than zero");

    switch (dimension_type)
    {
    case DimensionType::Cube:
    case DimensionType::CubeArray:
        META_CHECK_DESCR(dimensions, dimensions.GetWidth() == dimensions.GetHeight() && dimensions.GetDepth() == 6, "cube texture must have equal width and height dimensions and depth equal to 6");
        [[fallthrough]];
    case DimensionType::Tex3D:
        META_CHECK_DESCR(dimensions.GetDepth(), !mipmapped || !(dimensions.GetDepth() % 2), "all dimensions of the mip-mapped texture should be a power of 2, but depth is not");
        [[fallthrough]];
    case DimensionType::Tex2D:
    case DimensionType::Tex2DArray:
    case DimensionType::Tex2DMultisample:
        META_CHECK_DESCR(dimensions.GetHeight(), !mipmapped || !(dimensions.GetHeight() % 2), "all dimensions of the mip-mapped texture should be a power of 2, but height is not");
        [[fallthrough]];
    case DimensionType::Tex1D:
    case DimensionType::Tex1DArray:
        META_CHECK_DESCR(dimensions.GetWidth(), !mipmapped || !(dimensions.GetWidth() % 2), "all dimensions of the mip-mapped texture should be a power of 2, but width is not");
        break;
    default:
        META_UNEXPECTED(dimension_type);
    }
}

Data::Size Texture::GetRequiredMipLevelsCount(const Dimensions& dimensions)
{
    META_FUNCTION_TASK();
    return 1U + static_cast<uint32_t>(std::floor(std::log2(static_cast<double>(dimensions.GetLongestSide()))));
}

Data::Size Texture::GetDataSize(Data::MemoryState size_type) const noexcept
{
    META_FUNCTION_TASK();
    return size_type == Data::MemoryState::Reserved
            ? m_settings.dimensions.GetPixelsCount() * GetPixelSize(m_settings.pixel_format) * m_settings.array_length
            : GetInitializedDataSize();
}

Data::Size Texture::GetSubResourceDataSize(const SubResource::Index& sub_resource_index) const
{
    META_FUNCTION_TASK();
    META_CHECK_LESS(sub_resource_index, m_sub_resource_count);
    return m_sub_resource_sizes[sub_resource_index.GetRawIndex(m_sub_resource_count)];
}

Rhi::TextureView Texture::GetTextureView(const SubResource::Index& subresource_index,
                                          const SubResource::Count& subresource_count,
                                          Opt<Rhi::TextureDimensionType> texture_dimension_type_opt)
{
    META_FUNCTION_TASK();
    return Rhi::TextureView(*this, subresource_index, subresource_count, texture_dimension_type_opt);
}

void Texture::SetData(Rhi::ICommandQueue&, const SubResources& sub_resources)
{
    META_FUNCTION_TASK();
    META_CHECK_NOT_EMPTY_DESCR(sub_resources, "can not set buffer data from empty sub-resources");

    Data::Size sub_resources_data_size = 0U;
    for(const Rhi::SubResource& sub_resource : sub_resources)
    {
        META_CHECK_NAME_DESCR("sub_resource", !sub_resource.IsEmptyOrNull(), "can not set empty subresource data to buffer");
        sub_resources_data_size += sub_resource.GetDataSize();
        META_CHECK_LESS(sub_resource.GetIndex(), m_sub_resource_count);
    }

    const Data::Size reserved_data_size = GetDataSize(Data::MemoryState::Reserved);
    META_UNUSED(reserved_data_size);

    META_CHECK_LESS_OR_EQUAL_DESCR(sub_resources_data_size, reserved_data_size, "can not set more data than allocated buffer size");
    SetInitializedDataSize(sub_resources_data_size);
}

Data::Size Texture::CalculateSubResourceDataSize(const SubResource::Index& sub_resource_index) const
{
    META_FUNCTION_TASK();
    ValidateSubResource(sub_resource_index, {});

    const Data::Size pixel_size = GetPixelSize(m_settings.pixel_format);
    if (sub_resource_index.GetMipLevel() == 0U)
    {
        return pixel_size * static_cast<const Data::FrameSize&>(m_settings.dimensions).GetPixelsCount();
    }

    const double mip_divider = std::pow(2.0, sub_resource_index.GetMipLevel());
    const Data::FrameSize mip_frame_size(
        static_cast<uint32_t>(std::ceil(static_cast<double>(m_settings.dimensions.GetWidth()) / mip_divider)),
        static_cast<uint32_t>(std::ceil(static_cast<double>(m_settings.dimensions.GetHeight()) / mip_divider))
    );
    return pixel_size * mip_frame_size.GetPixelsCount();
}

void Texture::ValidateSubResource(const Rhi::SubResource& sub_resource) const
{
    META_FUNCTION_TASK();
    ValidateSubResource(sub_resource.GetIndex(), sub_resource.GetDataRangeOptional());

    const Data::Index sub_resource_raw_index = sub_resource.GetIndex().GetRawIndex(m_sub_resource_count);
    const Data::Size sub_resource_data_size = m_sub_resource_sizes[sub_resource_raw_index];
    META_UNUSED(sub_resource_data_size);

    if (sub_resource.HasDataRange())
    {
        META_CHECK_EQUAL_DESCR(sub_resource.GetDataSize(), sub_resource.GetDataRange().GetLength(),
                               "sub-resource {} data size should be equal to the length of data range", sub_resource.GetIndex());
    }
    META_CHECK_LESS_OR_EQUAL_DESCR(sub_resource.GetDataSize(), sub_resource_data_size,
                                   "sub-resource {} data size should be less or equal than full resource size", sub_resource.GetIndex());
}

void Texture::ValidateSubResource(const SubResource::Index& sub_resource_index, const std::optional<BytesRange>& sub_resource_data_range) const
{
    META_FUNCTION_TASK();
    META_CHECK_LESS(sub_resource_index, m_sub_resource_count);
    if (!sub_resource_data_range)
        return;

    META_CHECK_NAME_DESCR("sub_resource_data_range", !sub_resource_data_range->IsEmpty(),
                          "sub-resource {} data range can not be empty", sub_resource_index);
    const Data::Index sub_resource_raw_index = sub_resource_index.GetRawIndex(m_sub_resource_count);
    META_CHECK_LESS(sub_resource_raw_index, m_sub_resource_sizes.size());

    const Data::Size sub_resource_data_size = m_sub_resource_sizes[sub_resource_raw_index];
    META_UNUSED(sub_resource_data_size);
    META_CHECK_LESS_DESCR(sub_resource_data_range->GetEnd(), sub_resource_data_size + 1,
                          "sub-resource index {}", sub_resource_index);
}

} // namespace Methane::Graphics::Base