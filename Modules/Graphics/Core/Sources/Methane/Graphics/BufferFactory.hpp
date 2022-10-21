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

#include <Methane/Graphics/Buffer.h>

#include "ContextBase.h"

namespace Methane::Graphics
{

inline Buffer::StorageMode GetBufferStorageMode(bool is_volatile_data) noexcept
{
    return is_volatile_data ? Buffer::StorageMode::Managed
                            : Buffer::StorageMode::Private;
}

template<typename NativeBufferType, typename ...ExtraConstructorArgTypes>
std::enable_if_t<std::is_base_of_v<BufferBase, NativeBufferType>, Ptr<NativeBufferType>>
CreateVertexBuffer(const IContext& context, Data::Size size, Data::Size stride, bool is_volatile, ExtraConstructorArgTypes... extra_construct_args)
{
    const Buffer::Settings settings{
        Buffer::Type::Vertex,
        Resource::Usage::None,
        size,
        stride,
        PixelFormat::Unknown,
        GetBufferStorageMode(is_volatile)
    };
    return std::make_shared<NativeBufferType>(dynamic_cast<const ContextBase&>(context), settings, extra_construct_args...);
}

template<typename NativeBufferType, typename ...ExtraConstructorArgTypes>
std::enable_if_t<std::is_base_of_v<BufferBase, NativeBufferType>, Ptr<NativeBufferType>>
CreateIndexBuffer(const IContext& context, Data::Size size, PixelFormat format, bool is_volatile, ExtraConstructorArgTypes... extra_construct_args)
{
    const Buffer::Settings settings{
        Buffer::Type::Index,
        Resource::Usage::None,
        size,
        GetPixelSize(format),
        format,
        GetBufferStorageMode(is_volatile)
    };
    return std::make_shared<NativeBufferType>(dynamic_cast<const ContextBase&>(context), settings, extra_construct_args...);
}

template<typename NativeBufferType, typename ...ExtraConstructorArgTypes>
std::enable_if_t<std::is_base_of_v<BufferBase, NativeBufferType>, Ptr<NativeBufferType>>
CreateConstantBuffer(const IContext& context, Data::Size size, bool addressable, bool is_volatile, ExtraConstructorArgTypes... extra_construct_args)
{
    using namespace magic_enum::bitwise_operators;
    const Resource::Usage  usage_mask = Resource::Usage::ShaderRead
                                      | (addressable ? Resource::Usage::Addressable : Resource::Usage::None);
    const Buffer::Settings settings{
        Buffer::Type::Constant,
        usage_mask,
        Buffer::GetAlignedBufferSize(size),
        0U,
        PixelFormat::Unknown,
        GetBufferStorageMode(is_volatile)
    };
    return std::make_shared<NativeBufferType>(dynamic_cast<const ContextBase&>(context), settings, extra_construct_args...);
}

template<typename NativeBufferType, typename ...ExtraConstructorArgTypes>
std::enable_if_t<std::is_base_of_v<BufferBase, NativeBufferType>, Ptr<NativeBufferType>>
CreateReadBackBuffer(const IContext& context, Data::Size size, ExtraConstructorArgTypes... extra_construct_args)
{
    META_FUNCTION_TASK();
    const Buffer::Settings settings{
        Buffer::Type::ReadBack,
        Resource::Usage::ReadBack,
        size,
        0U,
        PixelFormat::Unknown,
        Buffer::StorageMode::Managed
    };
    return std::make_shared<NativeBufferType>(dynamic_cast<const ContextBase&>(context), settings, extra_construct_args...);
}

} // namespace Methane::Graphics
