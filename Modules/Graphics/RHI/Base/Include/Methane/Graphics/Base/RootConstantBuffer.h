/******************************************************************************

Copyright 2024 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Base/RootConstantBuffer.h
Root constant buffer used for sub-allocations of small constants buffer views,
bound to Program using ProgramArgumentBinging as RootConstant.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RHI/RootConstant.h>

#include <Methane/Memory.hpp>
#include <Methane/Data/Types.h>
#include <Methane/Data/RangeSet.hpp>

namespace Methane::Graphics::Rhi
{

struct IBuffer;

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Base
{

class RootConstantBuffer;

class RootConstantAccessor
{
public:
    using Range = Data::Range<Data::Index>;

    RootConstantAccessor(RootConstantBuffer& buffer, const Range& buffer_range);
    ~RootConstantAccessor();

    const Range& GetBufferRange() const noexcept { return m_buffer_range; }
    Rhi::RootConstant GetRootConstant() const;
    void SetRootConstant(const Rhi::RootConstant& root_constant);

private:
    Ref<RootConstantBuffer> m_buffer;
    Range                   m_buffer_range;
};

class Context;

class RootConstantBuffer
{
public:
    using Accessor = RootConstantAccessor;

    explicit RootConstantBuffer(const Context& context);
    ~RootConstantBuffer();

    UniquePtr<Accessor> ReserveRootConstant(Data::Size root_constant_size);
    void ReleaseRootConstant(const Accessor& reservation);
    void SetRootConstant(const Accessor& accessor, const Rhi::RootConstant& root_constant);

    Data::Bytes&  GetData();
    Rhi::IBuffer& GetBuffer();

private:
    using RangeSet = Data::RangeSet<Data::Index>;

    const Context&    m_context;
    Data::Size        m_deferred_size = 0U;
    Data::Bytes       m_buffer_data;
    Ptr<Rhi::IBuffer> m_buffer_ptr;
    RangeSet          m_free_ranges;
};

} // namespace Methane::Graphics::Base
