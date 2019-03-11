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

FILE: Methane/Graphics/DirectX12/ContextDX.cpp
DirectX 12 implementation of the context interface.

******************************************************************************/

#include "ContextDX.h"
#include "RenderStateDX.h"
#include "RenderPassDX.h"
#include "CommandQueueDX.h"
#include "TypesDX.h"

#include <Methane/Graphics/Instrumentation.h>
#include <Methane/Graphics/Windows/Helpers.h>

#include <nowide/convert.hpp>
#include <cassert>

using namespace Methane;
using namespace Methane::Graphics;

// Helper function for acquiring the first available hardware adapter that supports Direct3D 12.
// If no such adapter can be found, *ppAdapter will be set to nullptr.
void GetHardwareAdapter(_In_ IDXGIFactory4* p_factory, _Outptr_result_maybenull_ IDXGIAdapter1** pp_adapter)
{
    ITT_FUNCTION_TASK();

    IDXGIAdapter1* p_adapter = nullptr;
    *pp_adapter = nullptr;

    for (UINT adapter_index = 0; DXGI_ERROR_NOT_FOUND != p_factory->EnumAdapters1(adapter_index, &p_adapter); ++adapter_index)
    {
        DXGI_ADAPTER_DESC1 desc;
        p_adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't select the Basic Render Driver adapter.
            // If you want a software adapter, pass in "/warp" on the command line.
            continue;
        }

        // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(p_adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
        {
            break;
        }
    }

    *pp_adapter = p_adapter;
}

Context::Ptr Context::Create(const Platform::AppEnvironment& env, const Data::Provider& data_provider, const Context::Settings& settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ContextDX>(env, data_provider, settings);
}

ContextDX::ContextDX(const Platform::AppEnvironment& env, const Data::Provider& data_provider, const Context::Settings& settings)
    : ContextBase(data_provider, settings)
    , m_present_sync_interval(settings.vsync_enabled ? 1 : 0)
    , m_present_flags(0) // DXGI_PRESENT_DO_NOT_WAIT
{
    ITT_FUNCTION_TASK();

    UINT dxgi_factory_flags = 0;

#ifdef _DEBUG
    // Enable the D3D12 debug layer.
    {
        wrl::ComPtr<ID3D12Debug> cp_debug_controller;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&cp_debug_controller))))
        {
            cp_debug_controller->EnableDebugLayer();
            dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    wrl::ComPtr<IDXGIFactory4> dxgi_factory;
    ThrowIfFailed(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&dxgi_factory)));

    if (env.use_warp_device)
    {
        wrl::ComPtr<IDXGIAdapter> cp_warp_adapter;
        ThrowIfFailed(dxgi_factory->EnumWarpAdapter(IID_PPV_ARGS(&cp_warp_adapter)));
        ThrowIfFailed(D3D12CreateDevice(cp_warp_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_cp_device)));
    }
    else
    {
        wrl::ComPtr<IDXGIAdapter1> cp_hardware_adapter;
        GetHardwareAdapter(dxgi_factory.Get(), &cp_hardware_adapter);
        ThrowIfFailed(D3D12CreateDevice(cp_hardware_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_cp_device)));
    }

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
    swap_chain_desc.BufferCount       = settings.frame_buffers_count;
    swap_chain_desc.BufferDesc.Width  = settings.frame_size.width;
    swap_chain_desc.BufferDesc.Height = settings.frame_size.height;
    swap_chain_desc.BufferDesc.Format = TypeConverterDX::DataFormatToDXGI(settings.color_format);
    swap_chain_desc.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.SwapEffect        = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_chain_desc.OutputWindow      = env.window_handle;
    swap_chain_desc.SampleDesc.Count  = 1;
    swap_chain_desc.Windowed          = TRUE;

    wrl::ComPtr<IDXGISwapChain> cp_swap_chain;
    ThrowIfFailed(dxgi_factory->CreateSwapChain(DefaultCommandQueueDX().GetNativeCommandQueue().Get(), &swap_chain_desc, &cp_swap_chain));
    ThrowIfFailed(cp_swap_chain.As(&m_cp_swap_chain));
    ThrowIfFailed(dxgi_factory->MakeWindowAssociation(env.window_handle, DXGI_MWA_NO_ALT_ENTER));

    // Create synchronization objects to be used for frame sync
    m_frame_buffer_index = m_cp_swap_chain->GetCurrentBackBufferIndex();
    m_fence_values.resize(settings.frame_buffers_count, 0);
    const UINT64 current_fence_value = GetCurrentFenceValue();
    ThrowIfFailed(m_cp_device->CreateFence(current_fence_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_cp_fence)));
    SetCurrentFenceValue(current_fence_value + 1);
    m_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_fence_event)
    {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    m_resource_manager.Initialize({ true });
}

ContextDX::~ContextDX()
{
    ITT_FUNCTION_TASK();

    CloseHandle(m_fence_event);
}

void ContextDX::OnCommandQueueCompleted(CommandQueue& /*cmd_list*/, uint32_t /*frame_index*/)
{
    ITT_FUNCTION_TASK();
}

void ContextDX::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    ContextBase::SetName(name);

    const std::wstring wname = nowide::widen(name);
    m_cp_device->SetName((wname + L" Device").c_str());
    m_cp_fence->SetName((wname + L" Fence").c_str());
}

void ContextDX::WaitForGpu(WaitFor wait_for)
{
    ITT_FUNCTION_TASK();

    // Schedule a Signal command in the queue.
    const UINT64 current_fence_value = GetCurrentFenceValue();
    CommandQueueDX& dx_command_queue = static_cast<CommandQueueDX&>(wait_for == WaitFor::ResourcesUploaded ? GetUploadCommandQueue() : GetRenderCommandQueue());
    ThrowIfFailed(dx_command_queue.GetNativeCommandQueue()->Signal(m_cp_fence.Get(), current_fence_value));

    const bool switch_to_next_frame = (wait_for == WaitFor::FramePresented);
    if (switch_to_next_frame)
    {
        // Update the frame index.
        m_frame_buffer_index = m_cp_swap_chain->GetCurrentBackBufferIndex();
    }

    // If the next frame is not ready to be rendered yet, wait until it is ready.
    const UINT64 new_fence_value = switch_to_next_frame ? GetCurrentFenceValue() : current_fence_value;
    if (!switch_to_next_frame || m_cp_fence->GetCompletedValue() < new_fence_value)
    {
        ThrowIfFailed(m_cp_fence->SetEventOnCompletion(new_fence_value, m_fence_event));
        WaitForSingleObjectEx(m_fence_event, INFINITE, FALSE);
    }

    // Set the fence value for the next frame.
    SetCurrentFenceValue(current_fence_value + 1);

    ContextBase::WaitForGpu(wait_for);
}

void ContextDX::Resize(const FrameSize& frame_size)
{
    ITT_FUNCTION_TASK();

    WaitForGpu(WaitFor::RenderComplete);

    // Resize the swap chain to the desired dimensions
    DXGI_SWAP_CHAIN_DESC1 desc = {};
    m_cp_swap_chain->GetDesc1(&desc);
    ThrowIfFailed(m_cp_swap_chain->ResizeBuffers(m_settings.frame_buffers_count, frame_size.width, frame_size.height, desc.Format, desc.Flags));

    ContextBase::Resize(frame_size);
}

void ContextDX::Present()
{
    ITT_FUNCTION_TASK();

    ThrowIfFailed(m_cp_swap_chain->Present(m_present_sync_interval, m_present_flags));

    OnPresentComplete();
}

CommandQueueDX& ContextDX::DefaultCommandQueueDX()
{
    ITT_FUNCTION_TASK();
    return static_cast<class CommandQueueDX&>(GetRenderCommandQueue());
}
