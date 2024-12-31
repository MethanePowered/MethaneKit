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

FILE: Methane/Graphics/DirectX/Sampler.cpp
DirectX 12 implementation of the sampler interface.

******************************************************************************/

#include <Methane/Graphics/DirectX/Sampler.h>
#include <Methane/Graphics/DirectX/Device.h>
#include <Methane/Graphics/DirectX/Types.h>

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Graphics/Color.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics::DirectX
{

using FilterMinMag = Rhi::ISampler::Filter::MinMag;
using FilterMip = Rhi::ISampler::Filter::Mip;

static D3D12_FILTER ConvertFilterMinNearestMagNearestToDirectX(const Rhi::ISampler::Filter& filter)
{
    META_FUNCTION_TASK();
    META_CHECK_EQUAL(filter.min, FilterMinMag::Nearest);
    META_CHECK_EQUAL(filter.mag, FilterMinMag::Nearest);

    switch (filter.mip)
    {
    case FilterMip::NotMipmapped:
    case FilterMip::Nearest:      return D3D12_FILTER_MIN_MAG_MIP_POINT;
    case FilterMip::Linear:       return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
    default:                      META_UNEXPECTED_RETURN(filter.mip, D3D12_FILTER_MIN_MAG_MIP_POINT);
    }
}

static D3D12_FILTER ConvertFilterMinNearestMagLinearToDirectX(const Rhi::ISampler::Filter& filter)
{
    META_FUNCTION_TASK();
    META_CHECK_EQUAL(filter.min, FilterMinMag::Nearest);
    META_CHECK_EQUAL(filter.mag, FilterMinMag::Linear);

    switch (filter.mip)
    {
    case FilterMip::NotMipmapped:
    case FilterMip::Nearest:      return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
    case FilterMip::Linear:       return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
    default:                      META_UNEXPECTED_RETURN(filter.mip, D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT);
    }
}

static D3D12_FILTER ConvertFilterMinNearestToDirectX(const Rhi::ISampler::Filter& filter)
{
    META_FUNCTION_TASK();
    META_CHECK_EQUAL(filter.min, FilterMinMag::Nearest);

    switch(filter.mag)
    {
    case FilterMinMag::Nearest: return ConvertFilterMinNearestMagNearestToDirectX(filter);
    case FilterMinMag::Linear:  return ConvertFilterMinNearestMagLinearToDirectX(filter);
    default:                    META_UNEXPECTED_RETURN(filter.mag, D3D12_FILTER_MIN_MAG_MIP_POINT);
    }
}

static D3D12_FILTER ConvertFilterMinLinearMagNearestToDirectX(const Rhi::ISampler::Filter& filter)
{
    META_FUNCTION_TASK();
    META_CHECK_EQUAL(filter.min, FilterMinMag::Linear);
    META_CHECK_EQUAL(filter.mag, FilterMinMag::Nearest);

    switch (filter.mip)
    {
    case FilterMip::NotMipmapped:
    case FilterMip::Nearest:      return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
    case FilterMip::Linear:       return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
    default:                      META_UNEXPECTED_RETURN(filter.mip, D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT);
    }
}

static D3D12_FILTER ConvertFilterMinLinearMagLinearToDirectX(const Rhi::ISampler::Filter& filter)
{
    META_FUNCTION_TASK();
    META_CHECK_EQUAL(filter.min, FilterMinMag::Linear);
    META_CHECK_EQUAL(filter.mag, FilterMinMag::Linear);

    switch (filter.mip)
    {
    case FilterMip::NotMipmapped:
    case FilterMip::Nearest:      return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    case FilterMip::Linear:       return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    default:                      META_UNEXPECTED_RETURN(filter.mip, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);
    }
}

static D3D12_FILTER ConvertFilterMinLinearToDirectX(const Rhi::ISampler::Filter& filter)
{
    META_FUNCTION_TASK();
    META_CHECK_EQUAL(filter.min, FilterMinMag::Linear);

    switch (filter.mag)
    {
    case FilterMinMag::Nearest: return ConvertFilterMinLinearMagNearestToDirectX(filter);
    case FilterMinMag::Linear:  return ConvertFilterMinLinearMagLinearToDirectX(filter);
    default:                    META_UNEXPECTED_RETURN(filter.mag, D3D12_FILTER_MIN_MAG_MIP_POINT);
    }
}

static D3D12_FILTER ConvertFilterToDirectX(const Rhi::ISampler::Filter& filter)
{
    META_FUNCTION_TASK();

    switch (filter.min)
    {
    case FilterMinMag::Nearest: return ConvertFilterMinNearestToDirectX(filter);
    case FilterMinMag::Linear:  return ConvertFilterMinLinearToDirectX(filter);
    default:                    META_UNEXPECTED_RETURN(filter.min, D3D12_FILTER_MIN_MAG_MIP_POINT);
    }

    // Other DX texture filtering types, which are currently not supported:
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

static D3D12_TEXTURE_ADDRESS_MODE ConvertAddressModeToDirectX(Rhi::ISampler::Address::Mode address_mode)
{
    META_FUNCTION_TASK();
    using AddressMode = Rhi::ISampler::Address::Mode;
    
    switch(address_mode)
    {
    case AddressMode::ClampToEdge:          return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    case AddressMode::ClampToZero:          return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    case AddressMode::ClampToBorderColor:   return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    case AddressMode::Repeat:               return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    case AddressMode::RepeatMirror:         return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    default: META_UNEXPECTED_RETURN(address_mode, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
    }
}

static void SetColor(const Color4F& in_color, FLOAT* out_color_ptr)
{
    META_FUNCTION_TASK();
    for (Data::Size i = 0; i < Color4F::Size; ++i)
    {
        out_color_ptr[i] = in_color[i];
    }
}

static void ConvertBorderColorToDXColor(Rhi::ISampler::BorderColor border_color, FLOAT* out_color_ptr)
{
    META_FUNCTION_TASK();
    META_CHECK_NOT_NULL(out_color_ptr);
    using BorderColor = Rhi::ISampler::BorderColor;
    
    switch (border_color)
    {
    case BorderColor::TransparentBlack: SetColor({ 0.F, 0.F, 0.F, 0.F }, out_color_ptr); break;
    case BorderColor::OpaqueBlack:      SetColor({ 0.F, 0.F, 0.F, 1.F }, out_color_ptr); break;
    case BorderColor::OpaqueWhite:      SetColor({ 1.F, 1.F, 1.F, 1.F }, out_color_ptr); break;
    default:                            META_UNEXPECTED(border_color);
    }
}

Sampler::Sampler(const Base::Context& context, const Settings& settings)
    : Resource(context, settings)
{ }

Opt<ResourceDescriptor> Sampler::InitializeNativeViewDescriptor(const View::Id& view_id)
{
    META_FUNCTION_TASK();
    const Rhi::IResource::Descriptor& descriptor = GetDescriptorByViewId(view_id);
    const D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor_handle = GetNativeCpuDescriptorHandle(descriptor);
    const Settings& settings = GetSettings();

    D3D12_SAMPLER_DESC dx_sampler_desc{};
    dx_sampler_desc.Filter             = ConvertFilterToDirectX(settings.filter);
    dx_sampler_desc.AddressU           = ConvertAddressModeToDirectX(settings.address.r);
    dx_sampler_desc.AddressV           = ConvertAddressModeToDirectX(settings.address.s);
    dx_sampler_desc.AddressW           = ConvertAddressModeToDirectX(settings.address.t);
    dx_sampler_desc.MinLOD             = settings.lod.min;
    dx_sampler_desc.MaxLOD             = settings.lod.max;
    dx_sampler_desc.MipLODBias         = settings.lod.bias;
    dx_sampler_desc.MaxAnisotropy      = 0;
    dx_sampler_desc.ComparisonFunc     = TypeConverter::CompareFunctionToD3D(settings.compare_function);
    ConvertBorderColorToDXColor(settings.border_color, &dx_sampler_desc.BorderColor[0]);

    GetDirectContext().GetDirectDevice().GetNativeDevice()->CreateSampler(&dx_sampler_desc, cpu_descriptor_handle);
    return descriptor;
}

} // namespace Methane::Graphics::DirectX
