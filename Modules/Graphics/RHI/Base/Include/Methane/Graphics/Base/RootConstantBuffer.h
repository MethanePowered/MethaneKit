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
#include <Methane/Graphics/RHI/ResourceView.h>
#include <Methane/Graphics/RHI/IContext.h>

#include <Methane/Memory.hpp>
#include <Methane/Data/Types.h>
#include <Methane/Data/RangeSet.hpp>
#include <Methane/Data/Emitter.hpp>

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

    [[nodiscard]] Rhi::RootConstant GetRootConstant() const;
    bool SetRootConstant(const Rhi::RootConstant& root_constant) const;

    const Range& GetBufferRange() const noexcept { return m_buffer_range; }
    const Rhi::ResourceView GetResourceView() const;
    RootConstantBuffer& GetRootConstantBuffer() const { return m_buffer.get(); }

private:
    Ref<RootConstantBuffer> m_buffer;
    Range                   m_buffer_range;
};

class Context;

struct IRootConstantBufferCallback
{
    virtual void OnRootConstantBufferChanged(RootConstantBuffer& root_constant_buffer) = 0;
    virtual ~IRootConstantBufferCallback() = default;
};

class RootConstantBuffer
    : public Data::Emitter<IRootConstantBufferCallback>
    , private Data::Receiver<Rhi::IContextCallback> //NOSONAR
{
public:
    using ICallback = IRootConstantBufferCallback;
    using Accessor = RootConstantAccessor;

    explicit RootConstantBuffer(Context& context, std::string_view buffer_name);
    ~RootConstantBuffer() override;

    [[nodiscard]] UniquePtr<Accessor> ReserveRootConstant(Data::Size root_constant_size);
    void ReleaseRootConstant(const Accessor& accessor);
    void SetRootConstant(const Accessor& accessor, const Rhi::RootConstant& root_constant);

    Data::Bytes&  GetData();
    Rhi::IBuffer& GetBuffer();

    void SetBufferName(std::string_view buffer_name);
    std::string_view GetBufferName() { return m_buffer_name; }

private:
    using RangeSet = Data::RangeSet<Data::Index>;

    void UpdateGpuBuffer(Rhi::ICommandQueue& target_cmd_queue);

    // Rhi::IContextCallback overrides
    void OnContextCompletingInitialization(Rhi::IContext& context) final;
    void OnContextReleased(Rhi::IContext&) final;
    void OnContextInitialized(Rhi::IContext&) final;

    Context&          m_context;
    std::string       m_buffer_name;
    Data::Size        m_deferred_size = 0U;
    Data::Bytes       m_buffer_data;
    bool              m_buffer_data_changed = false;
    Ptr<Rhi::IBuffer> m_buffer_ptr;
    RangeSet          m_free_ranges;
};

} // namespace Methane::Graphics::Base
