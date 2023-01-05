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

FILE: Methane/Graphics/Metal/BufferSet.hh
Metal implementation of the buffer-set interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/BufferSet.h>

#import <Metal/Metal.h>

namespace Methane::Graphics::Metal
{

class BufferSet final
    : public Base::BufferSet
{
public:
    BufferSet(Rhi::BufferType buffers_type, const Refs<Rhi::IBuffer>& buffer_refs);

    const std::vector<id<MTLBuffer>>& GetNativeBuffers() const noexcept { return m_mtl_buffers; }
    const std::vector<NSUInteger>&    GetNativeOffsets() const noexcept { return m_mtl_buffer_offsets; }

private:
    std::vector<id<MTLBuffer>>  m_mtl_buffers;
    std::vector<NSUInteger>     m_mtl_buffer_offsets;
};

} // namespace Methane::Graphics::Metal
