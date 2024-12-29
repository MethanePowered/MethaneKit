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

FILE: Methane/Graphics/DirectX/Context.hpp
DirectX 12 base template implementation of the context interface.

******************************************************************************/

#pragma once

#include "Fence.h"
#include "Device.h"
#include "CommandQueue.h"
#include "Shader.h"
#include "Program.h"
#include "ComputeState.h"
#include "Buffer.h"
#include "Texture.h"
#include "Sampler.h"
#include "System.h"
#include "IContext.h"
#include "DescriptorManager.h"
#include "ErrorHandling.h"

#include <Methane/Graphics/RHI/ICommandKit.h>
#include <Methane/Graphics/Base/Context.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <wrl.h>
#include <directx/d3d12.h>

#include <array>

namespace Methane::Graphics::DirectX
{

namespace wrl = Microsoft::WRL;

template<class ContextBaseT, typename = std::enable_if_t<std::is_base_of_v<Base::Context, ContextBaseT>>>
class Context
    : public ContextBaseT
    , public IContext
{
public:
    Context(Base::Device& device, tf::Executor& parallel_executor, const typename ContextBaseT::Settings& settings)
        : ContextBaseT(device, std::make_unique<DescriptorManager>(*this), parallel_executor, settings)
    { }

    // Base::Context interface

    void Initialize(Base::Device& device, bool is_callback_emitted) override
    {
        META_FUNCTION_TASK();
        ContextBaseT::Initialize(device, false);
        GetDirectDescriptorManager().Initialize(m_descriptor_manager_init_settings);
        if (is_callback_emitted)
        {
            Data::Emitter<Rhi::IContextCallback>::Emit(&Rhi::IContextCallback::OnContextInitialized, *this);
        }
    }

    void Release() override
    {
        META_FUNCTION_TASK();

        const DescriptorManager& descriptor_manager = GetDirectDescriptorManager();
        m_descriptor_manager_init_settings.default_heap_sizes        = descriptor_manager.GetDescriptorHeapSizes(true, false);
        m_descriptor_manager_init_settings.shader_visible_heap_sizes = descriptor_manager.GetDescriptorHeapSizes(true, true);

        for(wrl::ComPtr<ID3D12QueryHeap>& query_heap_cptr : m_query_heaps)
        {
            query_heap_cptr.Reset();
        }
        GetDirectMutableDevice().ReleaseNativeDevice();

        ContextBaseT::Release();

        // DirectX descriptor heaps are released after destroying all resources
        // to check that all descriptor ranges have been properly released by resources
        GetDescriptorManager().Release();

        static_cast<System&>(Rhi::ISystem::Get()).ReportLiveObjects();
    }

    // IContext overrides

    [[nodiscard]] Ptr<Rhi::ICommandQueue> CreateCommandQueue(Rhi::CommandListType type) const final
    {
        META_FUNCTION_TASK();
        return std::make_shared<CommandQueue>(*this, type);
    }

    [[nodiscard]] Ptr<Rhi::IShader> CreateShader(Rhi::ShaderType type, const Rhi::ShaderSettings& settings) const final
    {
        META_FUNCTION_TASK();
        return std::make_shared<Shader>(type, *this, settings);
    }

    [[nodiscard]] Ptr<Rhi::IProgram> CreateProgram(const Rhi::ProgramSettings& settings) final
    {
        META_FUNCTION_TASK();
        return std::make_shared<Program>(*this, settings);
    }

    [[nodiscard]] Ptr<Rhi::IComputeState> CreateComputeState(const Rhi::ComputeStateSettings& settings) const final
    {
        META_FUNCTION_TASK();
        return std::make_shared<ComputeState>(*this, settings);
    }

    [[nodiscard]] Ptr<Rhi::IBuffer> CreateBuffer(const Rhi::BufferSettings& settings) const final
    {
        META_FUNCTION_TASK();
        return std::make_shared<Buffer>(*this, settings);
    }

    [[nodiscard]] Ptr<Rhi::ITexture> CreateTexture(const Rhi::TextureSettings& settings) const override
    {
        META_FUNCTION_TASK();
        return std::make_shared<Texture>(*this, settings);
    }

    [[nodiscard]] Ptr<Rhi::ISampler> CreateSampler(const Rhi::SamplerSettings& settings) const final
    {
        META_FUNCTION_TASK();
        return std::make_shared<Sampler>(*this, settings);
    }

    const Device& GetDirectDevice() const noexcept final
    {
        return static_cast<const Device&>(Base::Context::GetBaseDevice());
    }

    CommandQueue& GetDirectDefaultCommandQueue(Rhi::CommandListType type) final
    {
        META_FUNCTION_TASK();
        return static_cast<CommandQueue&>(Base::Context::GetDefaultCommandKit(type).GetQueue());
    }

    DescriptorManager& GetDirectDescriptorManager() const noexcept final
    {
        return static_cast<DescriptorManager&>(Base::Context::GetDescriptorManager());
    }

    ID3D12QueryHeap& GetNativeQueryHeap(D3D12_QUERY_HEAP_TYPE type, uint32_t max_query_count = 1U << 15U) const final
    {
        META_FUNCTION_TASK();
        META_CHECK_LESS(static_cast<size_t>(type), m_query_heaps.size());
        wrl::ComPtr<ID3D12QueryHeap>& query_heap_cptr = m_query_heaps[type];
        if (!query_heap_cptr)
        {
            D3D12_QUERY_HEAP_DESC query_heap_desc{};
            query_heap_desc.Count = max_query_count;
            query_heap_desc.Type  = type;
            ThrowIfFailed(GetDirectDevice().GetNativeDevice()->CreateQueryHeap(&query_heap_desc, IID_PPV_ARGS(&query_heap_cptr)),
                          GetDirectDevice().GetNativeDevice().Get());
        }
        META_CHECK_NOT_NULL(query_heap_cptr);
        return *query_heap_cptr.Get();
    }

protected:
    Device& GetDirectMutableDevice() noexcept
    {
        return static_cast<Device&>(GetBaseDevice());
    }

private:
    using NativeQueryHeaps = std::array<wrl::ComPtr<ID3D12QueryHeap>, D3D12_QUERY_HEAP_TYPE_COPY_QUEUE_TIMESTAMP + 1>;

    DescriptorManager::Settings m_descriptor_manager_init_settings{ true, {}, {} };
    mutable NativeQueryHeaps m_query_heaps;
};

} // namespace Methane::Graphics::DirectX
