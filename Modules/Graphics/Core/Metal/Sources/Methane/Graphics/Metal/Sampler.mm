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

FILE: Methane/Graphics/Metal/Sampler.mm
Metal implementation of the sampler interface.

******************************************************************************/

#include <Methane/Graphics/Metal/Sampler.hh>
#include <Methane/Graphics/Metal/Context.h>
#include <Methane/Graphics/Metal/Device.hh>
#include <Methane/Graphics/Metal/Types.hh>

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Platform/Apple/Types.hh>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

Ptr<ISampler> ISampler::Create(const IContext& context, const ISampler::Settings& settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<Metal::Sampler>(dynamic_cast<const Base::Context&>(context), settings);
}

} // namespace Methane::Graphics

namespace Methane::Graphics::Metal
{

static MTLSamplerAddressMode ConvertAddressModeToMetal(const Base::Sampler::Address::Mode& address_mode)
{
    META_FUNCTION_TASK();
    using AddressMode = Base::Sampler::Address::Mode;

    switch(address_mode)
    {
        case AddressMode::ClampToEdge:          return MTLSamplerAddressModeClampToEdge;
        case AddressMode::ClampToZero:          return MTLSamplerAddressModeClampToZero;
#ifndef APPLE_TVOS
        case AddressMode::ClampToBorderColor:   return MTLSamplerAddressModeClampToBorderColor;
#endif
        case AddressMode::Repeat:               return MTLSamplerAddressModeRepeat;
        case AddressMode::RepeatMirror:         return MTLSamplerAddressModeMirrorRepeat;
        default:                                META_UNEXPECTED_ARG_RETURN(address_mode, MTLSamplerAddressModeClampToEdge);
    }
}

static MTLSamplerMinMagFilter ConvertMinMagFilterToMetal(const Base::Sampler::Filter::MinMag& min_mag_filter)
{
    META_FUNCTION_TASK();
    using MinMagFilter = Base::Sampler::Filter::MinMag;

    switch(min_mag_filter)
    {
        case MinMagFilter::Nearest:             return MTLSamplerMinMagFilterNearest;
        case MinMagFilter::Linear:              return MTLSamplerMinMagFilterLinear;
        default:                                META_UNEXPECTED_ARG_RETURN(min_mag_filter, MTLSamplerMinMagFilterNearest);
    }
}

static MTLSamplerMipFilter ConvertMipFilterToMetal(const Base::Sampler::Filter::Mip& mip_filter)
{
    META_FUNCTION_TASK();
    using MipFilter = Base::Sampler::Filter::Mip;

    switch(mip_filter)
    {
        case MipFilter::NotMipmapped:           return MTLSamplerMipFilterNotMipmapped;
        case MipFilter::Nearest:                return MTLSamplerMipFilterNearest;
        case MipFilter::Linear:                 return MTLSamplerMipFilterLinear;
        default:                                META_UNEXPECTED_ARG_RETURN(mip_filter, MTLSamplerMipFilterNotMipmapped);
    }
}

#ifndef APPLE_TVOS // MTLSamplerBorderColor is not supported on tvOS
static MTLSamplerBorderColor ConvertBorderColorToMetal(const Base::Sampler::BorderColor& border_color)
{
    META_FUNCTION_TASK();
    using BorderColor = Base::Sampler::BorderColor;

    switch(border_color)
    {
        case BorderColor::TransparentBlack:     return MTLSamplerBorderColorTransparentBlack;
        case BorderColor::OpaqueBlack:          return MTLSamplerBorderColorOpaqueBlack;
        case BorderColor::OpaqueWhite:          return MTLSamplerBorderColorOpaqueWhite;
        default:                                META_UNEXPECTED_ARG_RETURN(border_color, MTLSamplerBorderColorTransparentBlack);
    }
}
#endif

Sampler::Sampler(const Base::Context& context, const Settings& settings)
    : Resource(context, settings)
    , m_mtl_sampler_desc([[MTLSamplerDescriptor alloc] init])
{
    META_FUNCTION_TASK();

    m_mtl_sampler_desc.rAddressMode = ConvertAddressModeToMetal(settings.address.r);
    m_mtl_sampler_desc.sAddressMode = ConvertAddressModeToMetal(settings.address.s);
    m_mtl_sampler_desc.tAddressMode = ConvertAddressModeToMetal(settings.address.t);
    
    m_mtl_sampler_desc.minFilter    = ConvertMinMagFilterToMetal(settings.filter.min);
    m_mtl_sampler_desc.magFilter    = ConvertMinMagFilterToMetal(settings.filter.mag);
    m_mtl_sampler_desc.mipFilter    = ConvertMipFilterToMetal(settings.filter.mip);
    
    m_mtl_sampler_desc.lodMinClamp  = settings.lod.min;
    m_mtl_sampler_desc.lodMaxClamp  = settings.lod.max;
    
    m_mtl_sampler_desc.maxAnisotropy   = settings.max_anisotropy;
    m_mtl_sampler_desc.compareFunction = TypeConverter::CompareFunctionToMetal(settings.compare_function);
    
#ifndef APPLE_TVOS
    m_mtl_sampler_desc.borderColor     = ConvertBorderColorToMetal(settings.border_color);
#endif
    
    ResetSamplerState();
}

bool Sampler::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (!Resource::SetName(name))
        return false;

    META_CHECK_ARG_NOT_NULL(m_mtl_sampler_desc);
    m_mtl_sampler_desc.label = Methane::MacOS::ConvertToNsType<std::string, NSString*>(name);

    ResetSamplerState();
    return true;
}

void Sampler::ResetSamplerState()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_mtl_sampler_desc);
    m_mtl_sampler_state = [GetMetalContext().GetMetalDevice().GetNativeDevice() newSamplerStateWithDescriptor:m_mtl_sampler_desc];
}

} // namespace Methane::Graphics::Metal
