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

FILE: Methane/Graphics/DirectX/BufferSet.cpp
DirectX 12 implementation of the buffer-set interface.

******************************************************************************/

#include <Methane/Graphics/DirectX/BufferSet.h>
#include <Methane/Graphics/DirectX/Buffer.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>

namespace Methane::Graphics::Rhi
{

Ptr<IBufferSet> IBufferSet::Create(BufferType buffers_type, const Refs<IBuffer>& buffer_refs)
{
    META_FUNCTION_TASK();
    return std::make_shared<DirectX::BufferSet>(buffers_type, buffer_refs);
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::DirectX
{

static std::vector<D3D12_VERTEX_BUFFER_VIEW> GetNativeVertexBufferViews(const Refs<Rhi::IBuffer>& buffer_refs)
{
    META_FUNCTION_TASK();
    std::vector<D3D12_VERTEX_BUFFER_VIEW> vertex_buffer_views;
    std::transform(buffer_refs.begin(), buffer_refs.end(), std::back_inserter(vertex_buffer_views),
        [](const Ref<Rhi::IBuffer>& buffer_ref)
        {
           const auto& buffer = static_cast<const Buffer&>(buffer_ref.get());
           return buffer.GetNativeVertexBufferView();
        }
    );
    return vertex_buffer_views;
}

BufferSet::BufferSet(Rhi::BufferType buffers_type, const Refs<Rhi::IBuffer>& buffer_refs)
    : Base::BufferSet(buffers_type, buffer_refs)
{
    META_FUNCTION_TASK();
    if (buffers_type == Rhi::BufferType::Vertex)
    {
        m_vertex_buffer_views = DirectX::GetNativeVertexBufferViews(GetRefs());
    }
}

const std::vector<D3D12_VERTEX_BUFFER_VIEW>& BufferSet::GetNativeVertexBufferViews() const
{
    META_FUNCTION_TASK();
    const Rhi::BufferType buffers_type = GetType();
    META_CHECK_EQUAL_DESCR(buffers_type, Rhi::BufferType::Vertex,
                               "unable to get vertex buffer views from buffer of {} type", magic_enum::enum_name(buffers_type));
    return m_vertex_buffer_views;
}

} // namespace Methane::Graphics
