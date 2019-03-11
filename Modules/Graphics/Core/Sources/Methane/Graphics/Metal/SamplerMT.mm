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

FILE: Methane/Graphics/Metal/SamplerMT.mm
Metal implementation of the sampler interface.

******************************************************************************/

#include "SamplerMT.h"
#include "ContextMT.h"
#include "TypesMT.h"

#include <Methane/Graphics/Instrumentation.h>

#import <Methane/Platform/MacOS/Types.h>

using namespace Methane::Graphics;
using namespace Methane::MacOS;

MTLSamplerAddressMode ConvertAddressModeToMetal(const SamplerBase::Address::Mode& address_mode) noexcept
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

MTLSamplerMinMagFilter ConvertMinMagFilterToMetal(const SamplerBase::Filter::MinMag& min_mag_filter) noexcept
{
    ITT_FUNCTION_TASK();

    using MinMagFilter = SamplerBase::Filter::MinMag;

    switch(min_mag_filter)
    {
        case MinMagFilter::Nearest:             return MTLSamplerMinMagFilterNearest;
        case MinMagFilter::Linear:              return MTLSamplerMinMagFilterLinear;
    }
}

MTLSamplerMipFilter ConvertMipFilterToMetal(const SamplerBase::Filter::Mip& mip_filter) noexcept
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

MTLSamplerBorderColor ConvertBorderColorToMetal(const SamplerBase::BorderColor& border_color) noexcept
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

Sampler::Ptr Sampler::Create(Context& context, const Sampler::Settings& settings, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<SamplerMT>(static_cast<ContextBase&>(context), settings, descriptor_by_usage);
}

SamplerMT::SamplerMT(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
    : SamplerBase(context, settings, descriptor_by_usage)
    , m_mtl_sampler_desc([[MTLSamplerDescriptor alloc] init])
{
    ITT_FUNCTION_TASK();

    InitializeDefaultDescriptors();
    
    m_mtl_sampler_desc.rAddressMode = ConvertAddressModeToMetal(m_settings.address.r);
    m_mtl_sampler_desc.sAddressMode = ConvertAddressModeToMetal(m_settings.address.s);
    m_mtl_sampler_desc.tAddressMode = ConvertAddressModeToMetal(m_settings.address.t);
    
    m_mtl_sampler_desc.minFilter    = ConvertMinMagFilterToMetal(m_settings.filter.min);
    m_mtl_sampler_desc.magFilter    = ConvertMinMagFilterToMetal(m_settings.filter.mag);
    m_mtl_sampler_desc.mipFilter    = ConvertMipFilterToMetal(m_settings.filter.mip);
    
    m_mtl_sampler_desc.lodMinClamp  = m_settings.lod.min;
    m_mtl_sampler_desc.lodMaxClamp  = m_settings.lod.max;
    
    m_mtl_sampler_desc.maxAnisotropy   = m_settings.max_anisotropy;
    m_mtl_sampler_desc.compareFunction = TypeConverterMT::CompareFunctionToMetal(m_settings.compare_function);
    m_mtl_sampler_desc.borderColor     = ConvertBorderColorToMetal(m_settings.border_color);
    
    ResetSampletState();
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

    assert(!!m_mtl_sampler_desc);
    m_mtl_sampler_desc.label = Methane::MacOS::ConvertToNSType<std::string, NSString*>(name);
    
    ResetSampletState();
}

void SamplerMT::ResetSampletState()
{
    ITT_FUNCTION_TASK();

    if (m_mtl_sampler_state)
    {
        [m_mtl_sampler_state release];
    }
    
    assert(m_mtl_sampler_desc);
    m_mtl_sampler_state = [GetContextMT().GetNativeDevice() newSamplerStateWithDescriptor:m_mtl_sampler_desc];
}

ContextMT& SamplerMT::GetContextMT() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<class ContextMT&>(m_context);
}
