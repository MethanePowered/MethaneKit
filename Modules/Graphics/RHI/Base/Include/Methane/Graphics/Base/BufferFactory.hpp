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

#include <Methane/Graphics/IBuffer.h>

namespace Methane::Graphics::Base
{

inline IBuffer::StorageMode GetBufferStorageMode(bool is_volatile_data) noexcept
{
    return is_volatile_data ? IBuffer::StorageMode::Managed
                            : IBuffer::StorageMode::Private;
}

template<typename NativeBufferType, typename ...ExtraConstructorArgTypes>
std::enable_if_t<std::is_base_of_v<Buffer, NativeBufferType>, Ptr<NativeBufferType>>
CreateVertexBuffer(const IContext& context, Data::Size size, Data::Size stride, bool is_volatile, ExtraConstructorArgTypes... extra_construct_args)
{
    const IBuffer::Settings settings{
        IBuffer::Type::Vertex,
        IResource::Usage::None,
        size,
        stride,
        PixelFormat::Unknown,
        GetBufferStorageMode(is_volatile)
    };
    return std::make_shared<NativeBufferType>(dynamic_cast<const Context&>(context), settings, extra_construct_args...);
}

template<typename NativeBufferType, typename ...ExtraConstructorArgTypes>
std::enable_if_t<std::is_base_of_v<Buffer, NativeBufferType>, Ptr<NativeBufferType>>
CreateIndexBuffer(const IContext& context, Data::Size size, PixelFormat format, bool is_volatile, ExtraConstructorArgTypes... extra_construct_args)
{
    const IBuffer::Settings settings{
        IBuffer::Type::Index,
        IResource::Usage::None,
        size,
        GetPixelSize(format),
        format,
        GetBufferStorageMode(is_volatile)
    };
    return std::make_shared<NativeBufferType>(dynamic_cast<const Context&>(context), settings, extra_construct_args...);
}

template<typename NativeBufferType, typename ...ExtraConstructorArgTypes>
std::enable_if_t<std::is_base_of_v<Buffer, NativeBufferType>, Ptr<NativeBufferType>>
CreateConstantBuffer(const IContext& context, Data::Size size, bool addressable, bool is_volatile, ExtraConstructorArgTypes... extra_construct_args)
{
    using namespace magic_enum::bitwise_operators;
    const IResource::Usage  usage_mask = IResource::Usage::ShaderRead
                                        | (addressable ? IResource::Usage::Addressable : IResource::Usage::None);
    const IBuffer::Settings settings{
        IBuffer::Type::Constant,
        usage_mask,
        IBuffer::GetAlignedBufferSize(size),
        0U,
        PixelFormat::Unknown,
        GetBufferStorageMode(is_volatile)
    };
    return std::make_shared<NativeBufferType>(dynamic_cast<const Context&>(context), settings, extra_construct_args...);
}

template<typename NativeBufferType, typename ...ExtraConstructorArgTypes>
std::enable_if_t<std::is_base_of_v<Buffer, NativeBufferType>, Ptr<NativeBufferType>>
CreateReadBackBuffer(const IContext& context, Data::Size size, ExtraConstructorArgTypes... extra_construct_args)
{
    META_FUNCTION_TASK();
    const IBuffer::Settings settings{
        IBuffer::Type::ReadBack,
        IResource::Usage::ReadBack,
        size,
        0U,
        PixelFormat::Unknown,
        IBuffer::StorageMode::Managed
    };
    return std::make_shared<NativeBufferType>(dynamic_cast<const Context&>(context), settings, extra_construct_args...);
}

} // namespace Methane::Graphics::Base
