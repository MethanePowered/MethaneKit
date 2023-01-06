/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Null/Buffer.cpp
Null implementation of the buffer interface.

******************************************************************************/

#include <Methane/Graphics/Null/Buffer.h>

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Graphics/Base/BufferFactory.hpp>

#include <iterator>

namespace Methane::Graphics::Rhi
{

Ptr<IBuffer> IBuffer::CreateVertexBuffer(const IContext& context, Data::Size size, Data::Size stride, bool is_volatile)
{
    META_FUNCTION_TASK();
    return Base::CreateVertexBuffer<Null::Buffer>(context, size, stride, is_volatile);
}

Ptr<IBuffer> IBuffer::CreateIndexBuffer(const IContext& context, Data::Size size, PixelFormat format, bool is_volatile)
{
    META_FUNCTION_TASK();
    return Base::CreateIndexBuffer<Null::Buffer>(context, size, format, is_volatile);
}

Ptr<IBuffer> IBuffer::CreateConstantBuffer(const IContext& context, Data::Size size, bool addressable, bool is_volatile)
{
    META_FUNCTION_TASK();
    return Base::CreateConstantBuffer<Null::Buffer>(context, size, addressable, is_volatile);
}

Ptr<IBuffer> IBuffer::CreateReadBackBuffer(const IContext& context, Data::Size size)
{
    META_FUNCTION_TASK();
    return Base::CreateReadBackBuffer<Null::Buffer>(context, size);
}

Data::Size IBuffer::GetAlignedBufferSize(Data::Size size) noexcept
{
    META_FUNCTION_TASK();
    return size;
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Null
{

Buffer::Buffer(const Base::Context& context, const Settings& settings)
    : Resource(context, settings)
{
}

} // namespace Methane::Graphics::Null
