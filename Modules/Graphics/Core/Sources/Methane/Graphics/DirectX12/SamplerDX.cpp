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

FILE: Methane/Graphics/DirectX12/SamplerDX.cpp
DirectX 12 implementation of the sampler interface.

******************************************************************************/

#include "SamplerDX.h"
#include "DeviceDX.h"
#include "TypesDX.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics
{

static D3D12_FILTER ConvertFilterToDX(const SamplerBase::Filter& filter) noexcept
{
    ITT_FUNCTION_TASK();

    using FilterMinMag = SamplerBase::Filter::MinMag;
    using FilterMip = SamplerBase::Filter::Mip;
    
    switch (filter.min)
    {
    case FilterMinMag::Nearest:
    {
        switch(filter.mag)
        {
        case FilterMinMag::Nearest:
        {
            switch (filter.mip)
            {
            case FilterMip::NotMipmapped:
            case FilterMip::Nearest:      return D3D12_FILTER_MIN_MAG_MIP_POINT;
            case FilterMip::Linear:       return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
            }
        } break;
        case FilterMinMag::Linear:
        {
            switch (filter.mip)
            {
            case FilterMip::NotMipmapped:
            case FilterMip::Nearest:      return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
            case FilterMip::Linear:       return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
            }
        } break;
        }
    } break;

    case FilterMinMag::Linear:
    {
        switch (filter.mag)
        {
        case FilterMinMag::Nearest:
        {
            switch (filter.mip)
            {
            case FilterMip::NotMipmapped:
            case FilterMip::Nearest:      return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
            case FilterMip::Linear:       return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
            }
        } break;
        case FilterMinMag::Linear:
        {
            switch (filter.mip)
            {
            case FilterMip::NotMipmapped:
            case FilterMip::Nearest:      return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
            case FilterMip::Linear:       return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            }
        } break;
        }
    } break;
    }
    return D3D12_FILTER_MIN_MAG_MIP_POINT;

    // TODO: unsupported filtering types
    // D3D12_FILTER_ANISOTROPIC
    // D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT
    // D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR
    // D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT
    // D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR
    // D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT
    // D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR
    // D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT
    // D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR
    // D3D12_FILTER_COMPARISON_ANISOTROPIC
    // D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT
    // D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR
    // D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT
    // D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR
    // D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT
    // D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR
    // D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT
    // D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR
    // D3D12_FILTER_MINIMUM_ANISOTROPIC
    // D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT
    // D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR
    // D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT
    // D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR
    // D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT
    // D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR
    // D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT
    // D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR
    // D3D12_FILTER_MAXIMUM_ANISOTROPIC
}

static D3D12_TEXTURE_ADDRESS_MODE ConvertAddressModeToDX(SamplerBase::Address::Mode address_mode) noexcept
{
    ITT_FUNCTION_TASK();

    using AddressMode = SamplerBase::Address::Mode;
    
    switch(address_mode)
    {
    case AddressMode::ClampToEdge:          return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    case AddressMode::ClampToZero:          return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    case AddressMode::ClampToBorderColor:   return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    case AddressMode::Repeat:               return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    case AddressMode::RepeatMirror:         return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    }
    return D3D12_TEXTURE_ADDRESS_MODE_WRAP;

    // TODO: unsupported filtering types
    // D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE
}

static void SetColor(const Color4f& in_color, FLOAT* p_out_color) noexcept
{
    ITT_FUNCTION_TASK();

    for (int i = 0; i < in_color.array_size; ++i)
    {
        p_out_color[i] = in_color[i];
    }
}

static void ConvertBorderColorToDXColor(SamplerBase::BorderColor border_color, FLOAT* p_out_color) noexcept
{
    ITT_FUNCTION_TASK();
    assert(!!p_out_color);

    using BorderColor = SamplerBase::BorderColor;
    
    switch (border_color)
    {
    case BorderColor::TransparentBlack: SetColor({ 0.f, 0.f, 0.f, 0.f }, p_out_color); break;
    case BorderColor::OpaqueBlack:      SetColor({ 0.f, 0.f, 0.f, 1.f }, p_out_color); break;
    case BorderColor::OpaqueWhite:      SetColor({ 1.f, 1.f, 1.f, 1.f }, p_out_color); break;
    default:                            SetColor({ 1.f, 0.f, 0.f, 0.f }, p_out_color);
    }
}

Ptr<Sampler> Sampler::Create(Context& context, const Sampler::Settings& settings, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<SamplerDX>(dynamic_cast<ContextBase&>(context), settings, descriptor_by_usage);
}

SamplerDX::SamplerDX(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
    : SamplerBase(context, settings, descriptor_by_usage)
{
    ITT_FUNCTION_TASK();

    InitializeDefaultDescriptors();

    D3D12_SAMPLER_DESC dx_sampler_desc = {};
    dx_sampler_desc.Filter             = ConvertFilterToDX(settings.filter);
    dx_sampler_desc.AddressU           = ConvertAddressModeToDX(settings.address.r);
    dx_sampler_desc.AddressV           = ConvertAddressModeToDX(settings.address.s);
    dx_sampler_desc.AddressW           = ConvertAddressModeToDX(settings.address.t);
    dx_sampler_desc.MinLOD             = settings.lod.min;
    dx_sampler_desc.MaxLOD             = settings.lod.max;
    dx_sampler_desc.MipLODBias         = settings.lod.bias;
    dx_sampler_desc.MaxAnisotropy      = 0;
    dx_sampler_desc.ComparisonFunc     = TypeConverterDX::CompareFunctionToDX(settings.compare_function);
    ConvertBorderColorToDXColor(settings.border_color, &dx_sampler_desc.BorderColor[0]);

    GetContextDX().GetDeviceDX().GetNativeDevice()->CreateSampler(&dx_sampler_desc, GetNativeCpuDescriptorHandle(Usage::ShaderRead));
}

} // namespace Methane::Graphics
