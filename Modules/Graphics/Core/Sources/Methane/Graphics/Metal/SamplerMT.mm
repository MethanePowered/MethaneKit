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

FILE: Methane/Graphics/Metal/SamplerMT.mm
Metal implementation of the sampler interface.

******************************************************************************/

#include "SamplerMT.hh"
#include "ContextMT.h"
#include "DeviceMT.hh"
#include "TypesMT.hh"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Platform/MacOS/Types.hh>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

static MTLSamplerAddressMode ConvertAddressModeToMetal(const SamplerBase::Address::Mode& address_mode) noexcept
{
    ITT_FUNCTION_TASK();

    using AddressMode = SamplerBase::Address::Mode;

    switch(address_mode)
    {
        case AddressMode::ClampToEdge:          return MTLSamplerAddressModeClampToEdge;
        case AddressMode::ClampToZero:          return MTLSamplerAddressModeClampToZero;
        case AddressMode::ClampToBorderColor:   return MTLSamplerAddressModeClampToBorderColor;
        case AddressMode::Repeat:               return MTLSamplerAddressModeRepeat;
        case AddressMode::RepeatMirror:         return MTLSamplerAddressModeMirrorRepeat;
    }
}

static MTLSamplerMinMagFilter ConvertMinMagFilterToMetal(const SamplerBase::Filter::MinMag& min_mag_filter) noexcept
{
    ITT_FUNCTION_TASK();

    using MinMagFilter = SamplerBase::Filter::MinMag;

    switch(min_mag_filter)
    {
        case MinMagFilter::Nearest:             return MTLSamplerMinMagFilterNearest;
        case MinMagFilter::Linear:              return MTLSamplerMinMagFilterLinear;
    }
}

static MTLSamplerMipFilter ConvertMipFilterToMetal(const SamplerBase::Filter::Mip& mip_filter) noexcept
{
    ITT_FUNCTION_TASK();

    using MipFilter = SamplerBase::Filter::Mip;

    switch(mip_filter)
    {
        case MipFilter::NotMipmapped:           return MTLSamplerMipFilterNotMipmapped;
        case MipFilter::Nearest:                return MTLSamplerMipFilterNearest;
        case MipFilter::Linear:                 return MTLSamplerMipFilterLinear;
    }
}

static MTLSamplerBorderColor ConvertBorderColorToMetal(const SamplerBase::BorderColor& border_color) noexcept
{
    ITT_FUNCTION_TASK();

    using BorderColor = SamplerBase::BorderColor;

    switch(border_color)
    {
        case BorderColor::TransparentBlack:     return MTLSamplerBorderColorTransparentBlack;
        case BorderColor::OpaqueBlack:          return MTLSamplerBorderColorOpaqueBlack;
        case BorderColor::OpaqueWhite:          return MTLSamplerBorderColorOpaqueWhite;
    }
}

Ptr<Sampler> Sampler::Create(Context& context, const Sampler::Settings& settings, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<SamplerMT>(dynamic_cast<ContextBase&>(context), settings, descriptor_by_usage);
}

SamplerMT::SamplerMT(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
    : SamplerBase(context, settings, descriptor_by_usage)
    , m_mtl_sampler_desc([[MTLSamplerDescriptor alloc] init])
{
    ITT_FUNCTION_TASK();

    InitializeDefaultDescriptors();
    
    m_mtl_sampler_desc.rAddressMode = ConvertAddressModeToMetal(settings.address.r);
    m_mtl_sampler_desc.sAddressMode = ConvertAddressModeToMetal(settings.address.s);
    m_mtl_sampler_desc.tAddressMode = ConvertAddressModeToMetal(settings.address.t);
    
    m_mtl_sampler_desc.minFilter    = ConvertMinMagFilterToMetal(settings.filter.min);
    m_mtl_sampler_desc.magFilter    = ConvertMinMagFilterToMetal(settings.filter.mag);
    m_mtl_sampler_desc.mipFilter    = ConvertMipFilterToMetal(settings.filter.mip);
    
    m_mtl_sampler_desc.lodMinClamp  = settings.lod.min;
    m_mtl_sampler_desc.lodMaxClamp  = settings.lod.max;
    
    m_mtl_sampler_desc.maxAnisotropy   = settings.max_anisotropy;
    m_mtl_sampler_desc.compareFunction = TypeConverterMT::CompareFunctionToMetal(settings.compare_function);
    m_mtl_sampler_desc.borderColor     = ConvertBorderColorToMetal(settings.border_color);

    ResetSamplerState();
}

SamplerMT::~SamplerMT()
{
    ITT_FUNCTION_TASK();

    [m_mtl_sampler_state release];
    [m_mtl_sampler_desc release];
}

void SamplerMT::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    SamplerBase::SetName(name);

    assert(m_mtl_sampler_desc != nil);
    m_mtl_sampler_desc.label = Methane::MacOS::ConvertToNsType<std::string, NSString*>(name);

    ResetSamplerState();
}

void SamplerMT::ResetSamplerState()
{
    ITT_FUNCTION_TASK();

    if (m_mtl_sampler_state)
    {
        [m_mtl_sampler_state release];
    }
    
    assert(m_mtl_sampler_desc);
    m_mtl_sampler_state = [GetContextMT().GetDeviceMT().GetNativeDevice() newSamplerStateWithDescriptor:m_mtl_sampler_desc];
}

} // namespace Methane::Graphics
