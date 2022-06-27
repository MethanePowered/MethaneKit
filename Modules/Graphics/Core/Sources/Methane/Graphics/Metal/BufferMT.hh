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

FILE: Methane/Graphics/Metal/BufferMT.hh
Metal implementation of the buffer interface.

******************************************************************************/

#pragma once

#include "ResourceMT.hh"

#include <Methane/Graphics/BufferBase.h>

#import <Metal/Metal.h>

namespace Methane::Graphics
{

class BufferMT final : public ResourceMT<BufferBase>
{
public:
    BufferMT(const ContextBase& context, const Settings& settings);

    // Resource interface
    void SetData(const SubResources& sub_resources, CommandQueue& target_cmd_queue) override;

    // Object interface
    bool SetName(const std::string& name) override;
    
    const id<MTLBuffer>& GetNativeBuffer() const noexcept { return m_mtl_buffer; }
    MTLIndexType         GetNativeIndexType() const noexcept;

private:
    void SetDataToManagedBuffer(const SubResources& sub_resources);
    void SetDataToPrivateBuffer(const SubResources& sub_resources);

    id<MTLBuffer> m_mtl_buffer;
};

class BufferSetMT final : public BufferSetBase
{
public:
    BufferSetMT(Buffer::Type buffers_type, const Refs<Buffer>& buffer_refs);

    const std::vector<id<MTLBuffer>>& GetNativeBuffers() const noexcept { return m_mtl_buffers; }
    const std::vector<NSUInteger>&    GetNativeOffsets() const noexcept { return m_mtl_buffer_offsets; }

private:
    std::vector<id<MTLBuffer>>  m_mtl_buffers;
    std::vector<NSUInteger>     m_mtl_buffer_offsets;
};

} // namespace Methane::Graphics
