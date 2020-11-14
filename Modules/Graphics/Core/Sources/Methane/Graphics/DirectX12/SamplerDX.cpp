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

FILE: Methane/Graphics/DirectX12/SamplerDX.cpp
DirectX 12 implementation of the sampler interface.

******************************************************************************/

#include "SamplerDX.h"
#include "DeviceDX.h"
#include "TypesDX.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/Color.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics
{

using FilterMinMag = Sampler::Filter::MinMag;
using FilterMip = Sampler::Filter::Mip;

static D3D12_FILTER ConvertFilterMinNearesetMagNearestToDX(const Sampler::Filter& filter)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL(filter.min, FilterMinMag::Nearest);
    META_CHECK_ARG_EQUAL(filter.mag, FilterMinMag::Nearest);

    switch (filter.mip)
    {
    case FilterMip::NotMipmapped:
    case FilterMip::Nearest:      return D3D12_FILTER_MIN_MAG_MIP_POINT;
    case FilterMip::Linear:       return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
    default:                      META_UNEXPECTED_ENUM_ARG_RETURN(filter.mip, D3D12_FILTER_MIN_MAG_MIP_POINT);
    }
}

static D3D12_FILTER ConvertFilterMinNearesetMagLinearToDX(const Sampler::Filter& filter)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL(filter.min, FilterMinMag::Nearest);
    META_CHECK_ARG_EQUAL(filter.mag, FilterMinMag::Linear);

    switch (filter.mip)
    {
    case FilterMip::NotMipmapped:
    case FilterMip::Nearest:      return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
    case FilterMip::Linear:       return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
    default:                      META_UNEXPECTED_ENUM_ARG_RETURN(filter.mip, D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT);
    }
}

static D3D12_FILTER ConvertFilterMinNearesetToDX(const Sampler::Filter& filter)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL(filter.min, FilterMinMag::Nearest);

    switch(filter.mag)
    {
    case FilterMinMag::Nearest: return ConvertFilterMinNearesetMagNearestToDX(filter);
    case FilterMinMag::Linear:  return ConvertFilterMinNearesetMagLinearToDX(filter);
    default:                    META_UNEXPECTED_ENUM_ARG_RETURN(filter.mag, D3D12_FILTER_MIN_MAG_MIP_POINT);
    }
}

static D3D12_FILTER ConvertFilterMinLinearMagNearestToDX(const Sampler::Filter& filter)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL(filter.min, FilterMinMag::Linear);
    META_CHECK_ARG_EQUAL(filter.mag, FilterMinMag::Nearest);

    switch (filter.mip)
    {
    case FilterMip::NotMipmapped:
    case FilterMip::Nearest:      return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
    case FilterMip::Linear:       return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
    default:                      META_UNEXPECTED_ENUM_ARG_RETURN(filter.mip, D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT);
    }
}

static D3D12_FILTER ConvertFilterMinLinearMagLinearToDX(const Sampler::Filter& filter)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL(filter.min, FilterMinMag::Linear);
    META_CHECK_ARG_EQUAL(filter.mag, FilterMinMag::Linear);

    switch (filter.mip)
    {
    case FilterMip::NotMipmapped:
    case FilterMip::Nearest:      return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    case FilterMip::Linear:       return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    default:                      META_UNEXPECTED_ENUM_ARG_RETURN(filter.mip, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);
    }
}

static D3D12_FILTER ConvertFilterMinLinearToDX(const Sampler::Filter& filter)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL(filter.min, FilterMinMag::Linear);

    switch (filter.mag)
    {
    case FilterMinMag::Nearest: return ConvertFilterMinLinearMagNearestToDX(filter); break;
    case FilterMinMag::Linear:  return ConvertFilterMinLinearMagLinearToDX(filter); break;
    default:                    META_UNEXPECTED_ENUM_ARG_RETURN(filter.mag, D3D12_FILTER_MIN_MAG_MIP_POINT);
    }
}

static D3D12_FILTER ConvertFilterToDX(const Sampler::Filter& filter)
{
    META_FUNCTION_TASK();

    switch (filter.min)
    {
    case FilterMinMag::Nearest: return ConvertFilterMinNearesetToDX(filter);
    case FilterMinMag::Linear:  return ConvertFilterMinLinearToDX(filter);
    default:                    META_UNEXPECTED_ENUM_ARG_RETURN(filter.min, D3D12_FILTER_MIN_MAG_MIP_POINT);
    }

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

static D3D12_TEXTURE_ADDRESS_MODE ConvertAddressModeToDX(Sampler::Address::Mode address_mode)
{
    META_FUNCTION_TASK();
    using AddressMode = Sampler::Address::Mode;
    
    switch(address_mode)
    {
    case AddressMode::ClampToEdge:          return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    case AddressMode::ClampToZero:          return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    case AddressMode::ClampToBorderColor:   return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    case AddressMode::Repeat:               return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    case AddressMode::RepeatMirror:         return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    default: META_UNEXPECTED_ENUM_ARG_RETURN(address_mode, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
    }
}

static void SetColor(const Color4f& in_color, FLOAT* p_out_color) noexcept
{
    META_FUNCTION_TASK();
    for (int i = 0; i < Color4f::array_size; ++i)
    {
        p_out_color[i] = in_color[i];
    }
}

static void ConvertBorderColorToDXColor(Sampler::BorderColor border_color, FLOAT* p_out_color)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(p_out_color);
    using BorderColor = Sampler::BorderColor;
    
    switch (border_color)
    {
    case BorderColor::TransparentBlack: SetColor({ 0.F, 0.F, 0.F, 0.F }, p_out_color); break;
    case BorderColor::OpaqueBlack:      SetColor({ 0.F, 0.F, 0.F, 1.F }, p_out_color); break;
    case BorderColor::OpaqueWhite:      SetColor({ 1.F, 1.F, 1.F, 1.F }, p_out_color); break;
    default:                            META_UNEXPECTED_ENUM_ARG(border_color);
    }
}

Ptr<Sampler> Sampler::Create(Context& context, const Sampler::Settings& settings, const DescriptorByUsage& descriptor_by_usage)
{
    META_FUNCTION_TASK();
    return std::make_shared<SamplerDX>(dynamic_cast<ContextBase&>(context), settings, descriptor_by_usage);
}

SamplerDX::SamplerDX(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
    : ResourceDX<SamplerBase>(context, settings, descriptor_by_usage)
{
    META_FUNCTION_TASK();
    InitializeDefaultDescriptors();

    D3D12_SAMPLER_DESC dx_sampler_desc{};
    dx_sampler_desc.Filter             = ConvertFilterToDX(settings.filter);
    dx_sampler_desc.AddressU           = ConvertAddressModeToDX(settings.address.r);
    dx_sampler_desc.AddressV           = ConvertAddressModeToDX(settings.address.s);
    dx_sampler_desc.AddressW           = ConvertAddressModeToDX(settings.address.t);
    dx_sampler_desc.MinLOD             = settings.lod.min;
    dx_sampler_desc.MaxLOD             = settings.lod.max;
    dx_sampler_desc.MipLODBias         = settings.lod.bias;
    dx_sampler_desc.MaxAnisotropy      = 0;
    dx_sampler_desc.ComparisonFunc     = TypeConverterDX::CompareFunctionToD3D(settings.compare_function);
    ConvertBorderColorToDXColor(settings.border_color, &dx_sampler_desc.BorderColor[0]);

    GetContextDX().GetDeviceDX().GetNativeDevice()->CreateSampler(&dx_sampler_desc, GetNativeCpuDescriptorHandle(Usage::ShaderRead));
}

} // namespace Methane::Graphics
