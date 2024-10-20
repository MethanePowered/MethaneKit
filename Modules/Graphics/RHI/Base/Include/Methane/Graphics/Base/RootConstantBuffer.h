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
#include <Methane/Instrumentation.h>

#include <mutex>
#include <atomic>

namespace Methane::Graphics::Rhi
{

struct IBuffer;

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Base
{

class RootConstantStorage;

class RootConstantAccessor
{
public:
    using Range = Data::Range<Data::Index>;

    RootConstantAccessor(RootConstantStorage& storage, const Range& buffer_range, Data::Size data_size);
    ~RootConstantAccessor();

    [[nodiscard]] Rhi::RootConstant GetRootConstant() const;
    bool SetRootConstant(const Rhi::RootConstant& root_constant) const;

    bool                    IsInitialized() const noexcept  { return m_is_initialized; }
    const Range&            GetBufferRange() const noexcept { return m_buffer_range; }
    Data::Size              GetDataSize() const noexcept    { return m_data_size; }
    Rhi::ResourceView       GetResourceView() const;
    RootConstantStorage&    GetRootConstantBuffer() const   { return m_storage_ref.get(); }

private:
    Ref<RootConstantStorage> m_storage_ref;  // storage reference
    Range                    m_buffer_range; // aligned memory range
    Data::Size               m_data_size;    // unaligned original size
    mutable bool             m_is_initialized = false;
};

class RootConstantStorage
{
public:
    using Accessor = RootConstantAccessor;

    RootConstantStorage() = default;
    virtual ~RootConstantStorage();

    // RootConstantStorage virtual methods
    virtual [[nodiscard]] UniquePtr<Accessor> ReserveRootConstant(Data::Size root_constant_size);
    virtual void ReleaseRootConstant(const Accessor& accessor);
    virtual void SetRootConstant(const Accessor& accessor, const Rhi::RootConstant& root_constant);

    Data::Size GetDataSize() const noexcept { return m_deferred_size; }
    Data::Bytes& GetData();

protected:
#ifdef TRACY_ENABLE
    using Mutex = tracy::Lockable<std::mutex>;
#else
    using Mutex = std::mutex;
#endif

    std::lock_guard<Mutex> GetLockGuard();
    bool IsDataResizeRequired() const noexcept { return m_data_resize_required.load(); }

private:
    using RangeSet = Data::RangeSet<Data::Index>;

    Data::Size        m_deferred_size = 0U;
    Data::Bytes       m_buffer_data;
    std::atomic<bool> m_data_resize_required{ false };
    RangeSet          m_free_ranges;

    TracyLockable(std::mutex, m_mutex);
};

class Context;
class RootConstantBuffer;

struct IRootConstantBufferCallback
{
    virtual void OnRootConstantBufferChanged(RootConstantBuffer& root_constant_buffer,
                                             const Ptr<Rhi::IBuffer>& old_buffer_ptr) = 0;
    
    virtual ~IRootConstantBufferCallback() = default;
};

class RootConstantBuffer final
    : public RootConstantStorage
    , public Data::Emitter<IRootConstantBufferCallback>
    , private Data::Receiver<Rhi::IContextCallback> //NOSONAR
{
public:
    using ICallback = IRootConstantBufferCallback;

    explicit RootConstantBuffer(Context& context, std::string_view buffer_name);

    // RootConstantStorage overrides
    [[nodiscard]] UniquePtr<Accessor> ReserveRootConstant(Data::Size root_constant_size) override;
    void SetRootConstant(const Accessor& accessor, const Rhi::RootConstant& root_constant) override;

    Rhi::IBuffer& GetBuffer();
    const Ptr<Rhi::IBuffer>& GetBufferPtr() const { return m_buffer_ptr; }

    [[nodiscard]] Rhi::ResourceView GetResourceView(Data::Size offset, Data::Size size);

    void SetBufferName(std::string_view buffer_name);
    std::string_view GetBufferName() { return m_buffer_name; }

private:
    using RangeSet = Data::RangeSet<Data::Index>;

    void UpdateGpuBuffer(Rhi::ICommandQueue& target_cmd_queue);

    // Rhi::IContextCallback overrides
    void OnContextUploadingResources(Rhi::IContext& context) final;
    void OnContextReleased(Rhi::IContext&) final    { /* event not handled */ }
    void OnContextInitialized(Rhi::IContext&) final { /* event not handled */ }

    Context&          m_context;
    std::string       m_buffer_name;
    std::atomic<bool> m_buffer_resize_required{ false };
    std::atomic<bool> m_buffer_data_changed{ false };
    Ptr<Rhi::IBuffer> m_buffer_ptr;
};

} // namespace Methane::Graphics::Base
