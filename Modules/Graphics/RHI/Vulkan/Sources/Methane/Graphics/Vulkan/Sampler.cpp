/******************************************************************************

Copyright 2019-2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/Sampler.cpp
Vulkan implementation of the sampler interface.

******************************************************************************/

#include <Methane/Graphics/Vulkan/Sampler.h>
#include <Methane/Graphics/Vulkan/IContext.h>
#include <Methane/Graphics/Vulkan/Types.h>

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>

namespace Methane::Graphics::Rhi
{

Ptr<ISampler> Rhi::ISampler::Create(const Rhi::IContext& context, const Rhi::ISampler::Settings& settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<Vulkan::Sampler>(dynamic_cast<const Base::Context&>(context), settings);
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Vulkan
{

static vk::Filter ConvertMinMagFilterToVulkan(Rhi::ISampler::Filter::MinMag filter_min_mag)
{
    META_FUNCTION_TASK();
    switch(filter_min_mag)
    {
    case Rhi::ISampler::Filter::MinMag::Nearest: return vk::Filter::eNearest;
    case Rhi::ISampler::Filter::MinMag::Linear:  return vk::Filter::eLinear;
    default: META_UNEXPECTED_ARG_RETURN(filter_min_mag, vk::Filter::eNearest);
    }
}

static vk::SamplerMipmapMode ConvertMipmapFilterToVulkan(Rhi::ISampler::Filter::Mip filter_mip_map)
{
    META_FUNCTION_TASK();
    switch(filter_mip_map)
    {
    case Rhi::ISampler::Filter::Mip::NotMipmapped:
    case Rhi::ISampler::Filter::Mip::Nearest:      return vk::SamplerMipmapMode::eNearest;
    case Rhi::ISampler::Filter::Mip::Linear:       return vk::SamplerMipmapMode::eLinear;
    default: META_UNEXPECTED_ARG_RETURN(filter_mip_map, vk::SamplerMipmapMode::eNearest);
    }
}

static vk::SamplerAddressMode ConvertSamplerAddressModeToVulkan(Rhi::ISampler::Address::Mode address_mode)
{
    META_FUNCTION_TASK();
    switch(address_mode)
    {
    case Rhi::ISampler::Address::Mode::ClampToEdge:        return vk::SamplerAddressMode::eClampToEdge;
    case Rhi::ISampler::Address::Mode::ClampToZero:        return vk::SamplerAddressMode::eClampToBorder;
    case Rhi::ISampler::Address::Mode::ClampToBorderColor: return vk::SamplerAddressMode::eClampToBorder;
    case Rhi::ISampler::Address::Mode::Repeat:             return vk::SamplerAddressMode::eRepeat;
    case Rhi::ISampler::Address::Mode::RepeatMirror:       return vk::SamplerAddressMode::eMirroredRepeat;
    default: META_UNEXPECTED_ARG_RETURN(address_mode, vk::SamplerAddressMode::eClampToEdge);
    }
}

static vk::BorderColor ConvertSamplerBorderColorToVulkan(Rhi::ISampler::BorderColor border_color)
{
    META_FUNCTION_TASK();
    switch(border_color)
    {
    case Rhi::ISampler::BorderColor::TransparentBlack: return vk::BorderColor::eFloatTransparentBlack;
    case Rhi::ISampler::BorderColor::OpaqueBlack:      return vk::BorderColor::eFloatOpaqueBlack;
    case Rhi::ISampler::BorderColor::OpaqueWhite:      return vk::BorderColor::eFloatOpaqueWhite;
    default: META_UNEXPECTED_ARG_RETURN(border_color, vk::BorderColor::eFloatTransparentBlack);
    }
}

Sampler::Sampler(const Base::Context& context, const Settings& settings)
    : Resource(context, settings, {})
    , m_vk_unique_sampler(GetNativeDevice().createSamplerUnique(
        vk::SamplerCreateInfo(
            vk::SamplerCreateFlags{},
            ConvertMinMagFilterToVulkan(settings.filter.mag),
            ConvertMinMagFilterToVulkan(settings.filter.min),
            ConvertMipmapFilterToVulkan(settings.filter.mip),
            ConvertSamplerAddressModeToVulkan(settings.address.s),
            ConvertSamplerAddressModeToVulkan(settings.address.t),
            ConvertSamplerAddressModeToVulkan(settings.address.r),
            settings.lod.bias,
            context.GetDevice().GetCapabilities().features.HasBit(Rhi::DeviceFeatures::Bit::AnisotropicFiltering),
            std::min(static_cast<float>(settings.max_anisotropy),
                     GetVulkanContext().GetVulkanDevice().GetNativePhysicalDevice().getProperties().limits.maxSamplerAnisotropy),
            settings.compare_function != Compare::Never,
            TypeConverter::CompareFunctionToVulkan(settings.compare_function),
            settings.lod.min,
            settings.lod.max,
            ConvertSamplerBorderColorToVulkan(settings.border_color),
            false // un-normalized coordinates
        )))
{
    META_FUNCTION_TASK();
}

Ptr<ResourceView::ViewDescriptorVariant> Sampler::CreateNativeViewDescriptor(const ResourceView::Id&)
{
    META_FUNCTION_TASK();
    ResourceView::ImageViewDescriptor image_view_desc;
    image_view_desc.vk_desc = vk::DescriptorImageInfo(GetNativeSampler());
    return std::make_shared<ResourceView::ViewDescriptorVariant>(std::move(image_view_desc));
}

} // namespace Methane::Graphics::Vulkan
