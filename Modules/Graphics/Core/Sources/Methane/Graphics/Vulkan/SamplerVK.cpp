/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/SamplerVK.cpp
Vulkan implementation of the sampler interface.

******************************************************************************/

#include "SamplerVK.h"
#include "ContextVK.h"
#include "TypesVK.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>

namespace Methane::Graphics
{

static vk::Filter ConvertMinMagFilterToVulkan(Sampler::Filter::MinMag filter_min_mag)
{
    META_FUNCTION_TASK();
    switch(filter_min_mag)
    {
    case Sampler::Filter::MinMag::Nearest: return vk::Filter::eNearest;
    case Sampler::Filter::MinMag::Linear:  return vk::Filter::eLinear;
    default: META_UNEXPECTED_ARG_RETURN(filter_min_mag, vk::Filter::eNearest);
    }
}

static vk::SamplerMipmapMode ConvertMipmapFilterToVulkan(Sampler::Filter::Mip filter_mip_map)
{
    META_FUNCTION_TASK();
    switch(filter_mip_map)
    {
    case Sampler::Filter::Mip::NotMipmapped:
    case Sampler::Filter::Mip::Nearest:      return vk::SamplerMipmapMode::eNearest;
    case Sampler::Filter::Mip::Linear:       return vk::SamplerMipmapMode::eLinear;
    default: META_UNEXPECTED_ARG_RETURN(filter_mip_map, vk::SamplerMipmapMode::eNearest);
    }
}

static vk::SamplerAddressMode ConvertSamplerAddressModeToVulkan(Sampler::Address::Mode address_mode)
{
    META_FUNCTION_TASK();
    switch(address_mode)
    {
    case Sampler::Address::Mode::ClampToEdge:        return vk::SamplerAddressMode::eClampToEdge;
    case Sampler::Address::Mode::ClampToZero:        return vk::SamplerAddressMode::eClampToBorder;
    case Sampler::Address::Mode::ClampToBorderColor: return vk::SamplerAddressMode::eClampToBorder;
    case Sampler::Address::Mode::Repeat:             return vk::SamplerAddressMode::eRepeat;
    case Sampler::Address::Mode::RepeatMirror:       return vk::SamplerAddressMode::eMirroredRepeat;
    default: META_UNEXPECTED_ARG_RETURN(address_mode, vk::SamplerAddressMode::eClampToEdge);
    }
}

static vk::BorderColor ConvertSamplerBorderColorToVulkan(Sampler::BorderColor border_color)
{
    META_FUNCTION_TASK();
    switch(border_color)
    {
    case Sampler::BorderColor::TransparentBlack: return vk::BorderColor::eFloatTransparentBlack;
    case Sampler::BorderColor::OpaqueBlack:      return vk::BorderColor::eFloatOpaqueBlack;
    case Sampler::BorderColor::OpaqueWhite:      return vk::BorderColor::eFloatOpaqueWhite;
    default: META_UNEXPECTED_ARG_RETURN(border_color, vk::BorderColor::eFloatTransparentBlack);
    }
}

static bool IsAnisotropicFilteringSupported(const Context& context) noexcept
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;
    return magic_enum::flags::enum_contains(context.GetDevice().GetCapabilities().features & Device::Features::AnisotropicFiltering);
}

Ptr<Sampler> Sampler::Create(const Context& context, const Sampler::Settings& settings, const DescriptorByUsage&)
{
    META_FUNCTION_TASK();
    return std::make_shared<SamplerVK>(dynamic_cast<const ContextBase&>(context), settings);
}

SamplerVK::SamplerVK(const ContextBase& context, const Settings& settings)
    : ResourceVK(context, settings, {})
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
            IsAnisotropicFilteringSupported(context), // anisotropy enable
            std::min(static_cast<float>(settings.max_anisotropy),
                     GetContextVK().GetDeviceVK().GetNativePhysicalDevice().getProperties().limits.maxSamplerAnisotropy),
            settings.compare_function != Compare::Never,
            TypeConverterVK::CompareFunctionToVulkan(settings.compare_function),
            settings.lod.min,
            settings.lod.max,
            ConvertSamplerBorderColorToVulkan(settings.border_color),
            false // un-normalized coordinates
        )))
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Graphics
