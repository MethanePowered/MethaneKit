/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Metal/BufferMT.h
Metal implementation of the buffer interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/BufferBase.h>

#import <Metal/Metal.h>

namespace Methane
{
namespace Graphics
{

class BufferMT : public BufferBase
{
public:
    using Ptr = std::shared_ptr<BufferMT>;

    BufferMT(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage = DescriptorByUsage());
    BufferMT(ContextBase& context, const Settings& settings, Data::Size stride, PixelFormat format, const DescriptorByUsage& descriptor_by_usage = DescriptorByUsage());
    ~BufferMT() override;

    // Resource interface
    void SetData(Data::ConstRawPtr p_data, Data::Size data_size) override;
    
    // Buffer interface
    uint32_t GetFormattedItemsCount() const override;

    // Object interface
    void SetName(const std::string& name) override;
    
    const id<MTLBuffer>& GetNativeBuffer() const noexcept { return m_mtl_buffer; }
    MTLIndexType         GetNativeIndexType() const noexcept;

protected:
    id<MTLBuffer> m_mtl_buffer;
    Data::Size    m_stride = 0;
    PixelFormat   m_format = PixelFormat::Unknown;
};

} // namespace Graphics
} // namespace Methane
