/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/BufferFactories.hpp
Buffer factory functions template implementations.

******************************************************************************/

#pragma once

#include "Context.h"

#include <Methane/Graphics/RHI/IBuffer.h>

namespace Methane::Graphics::Base
{

inline Rhi::BufferStorageMode GetBufferStorageMode(bool is_volatile_data) noexcept
{
    return is_volatile_data ? Rhi::BufferStorageMode::Managed
                            : Rhi::BufferStorageMode::Private;
}

template<typename NativeBufferType, typename ...ExtraConstructorArgTypes>
std::enable_if_t<std::is_base_of_v<Rhi::IBuffer, NativeBufferType>, Ptr<NativeBufferType>>
CreateVertexBuffer(const Rhi::IContext& context, Data::Size size, Data::Size stride, bool is_volatile, ExtraConstructorArgTypes... extra_construct_args)
{
    const Rhi::BufferSettings settings{
        Rhi::BufferType::Vertex,
        Rhi::ResourceUsage::None,
        size,
        stride,
        PixelFormat::Unknown,
        GetBufferStorageMode(is_volatile)
    };
    return std::make_shared<NativeBufferType>(dynamic_cast<const Context&>(context), settings, extra_construct_args...);
}

template<typename NativeBufferType, typename ...ExtraConstructorArgTypes>
std::enable_if_t<std::is_base_of_v<Rhi::IBuffer, NativeBufferType>, Ptr<NativeBufferType>>
CreateIndexBuffer(const Rhi::IContext& context, Data::Size size, PixelFormat format, bool is_volatile, ExtraConstructorArgTypes... extra_construct_args)
{
    const Rhi::BufferSettings settings{
        Rhi::BufferType::Index,
        Rhi::ResourceUsage::None,
        size,
        GetPixelSize(format),
        format,
        GetBufferStorageMode(is_volatile)
    };
    return std::make_shared<NativeBufferType>(dynamic_cast<const Context&>(context), settings, extra_construct_args...);
}

template<typename NativeBufferType, typename ...ExtraConstructorArgTypes>
std::enable_if_t<std::is_base_of_v<Rhi::IBuffer, NativeBufferType>, Ptr<NativeBufferType>>
CreateConstantBuffer(const Rhi::IContext& context, Data::Size size, bool addressable, bool is_volatile, ExtraConstructorArgTypes... extra_construct_args)
{
    using namespace magic_enum::bitwise_operators;
    const Rhi::ResourceUsage  usage_mask = Rhi::ResourceUsage::ShaderRead
                                        | (addressable ? Rhi::ResourceUsage::Addressable : Rhi::ResourceUsage::None);
    const Rhi::BufferSettings settings{
        Rhi::BufferType::Constant,
        usage_mask,
        Rhi::IBuffer::GetAlignedBufferSize(size),
        0U,
        PixelFormat::Unknown,
        GetBufferStorageMode(is_volatile)
    };
    return std::make_shared<NativeBufferType>(dynamic_cast<const Context&>(context), settings, extra_construct_args...);
}

template<typename NativeBufferType, typename ...ExtraConstructorArgTypes>
std::enable_if_t<std::is_base_of_v<Rhi::IBuffer, NativeBufferType>, Ptr<NativeBufferType>>
CreateReadBackBuffer(const Rhi::IContext& context, Data::Size size, ExtraConstructorArgTypes... extra_construct_args)
{
    META_FUNCTION_TASK();
    const Rhi::BufferSettings settings{
        Rhi::BufferType::ReadBack,
        Rhi::ResourceUsage::ReadBack,
        size,
        0U,
        PixelFormat::Unknown,
        Rhi::BufferStorageMode::Managed
    };
    return std::make_shared<NativeBufferType>(dynamic_cast<const Context&>(context), settings, extra_construct_args...);
}

} // namespace Methane::Graphics::Base