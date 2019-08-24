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
#include "DeviceDX.h"
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

Context::Ptr Context::Create(const Platform::AppEnvironment& env, const Data::Provider& data_provider, Device& device, const Context::Settings& settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ContextDX>(env, data_provider, static_cast<DeviceBase&>(device), settings);
}

ContextDX::ContextDX(const Platform::AppEnvironment& env, const Data::Provider& data_provider, DeviceBase& device, const Context::Settings& settings)
    : ContextBase(data_provider, device, settings)
    , m_platform_env(env)
{
    ITT_FUNCTION_TASK();

    Initialize(GetDeviceDX());

    m_resource_manager.Initialize({ true });
}

ContextDX::~ContextDX()
{
    ITT_FUNCTION_TASK();

    CloseHandle(m_fence_event);
}

void ContextDX::Initialize(const DeviceDX& device)
{
    ITT_FUNCTION_TASK();

    // Initialize swap-chain

    DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
    swap_chain_desc.BufferCount          = m_settings.frame_buffers_count;
    swap_chain_desc.BufferDesc.Width     = m_settings.frame_size.width;
    swap_chain_desc.BufferDesc.Height    = m_settings.frame_size.height;
    swap_chain_desc.BufferDesc.Format    = TypeConverterDX::DataFormatToDXGI(m_settings.color_format);
    swap_chain_desc.BufferUsage          = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.SwapEffect           = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_chain_desc.OutputWindow         = m_platform_env.window_handle;
    swap_chain_desc.SampleDesc.Count     = 1;
    swap_chain_desc.Windowed             = TRUE;

    const wrl::ComPtr<IDXGIFactory4>& cp_dxgi_factory = SystemDX::Get().GetNativeFactory();
    assert(!!cp_dxgi_factory);

    wrl::ComPtr<ID3D12CommandQueue>& cp_command_queue = DefaultCommandQueueDX().GetNativeCommandQueue();
    assert(!!cp_command_queue);

    wrl::ComPtr<IDXGISwapChain>  cp_swap_chain;
    ThrowIfFailed(cp_dxgi_factory->CreateSwapChain(cp_command_queue.Get(), &swap_chain_desc, &cp_swap_chain));
    assert(!!cp_swap_chain);

    ThrowIfFailed(cp_swap_chain.As(&m_cp_swap_chain));
    ThrowIfFailed(cp_dxgi_factory->MakeWindowAssociation(m_platform_env.window_handle, DXGI_MWA_NO_ALT_ENTER));

    // Initialize frame fences

    assert(!!m_cp_swap_chain);
    m_frame_buffer_index = m_cp_swap_chain->GetCurrentBackBufferIndex();
    m_frame_fences.resize(m_settings.frame_buffers_count);

    const wrl::ComPtr<ID3D12Device>& cp_device = device.GetNativeDevice();
    assert(!!cp_device);

    uint32_t frame_index = 0;
    for (FrameFence& frame_fence : m_frame_fences)
    {
        if (frame_fence.cp_fence)
        {
            SafeRelease(frame_fence.cp_fence);
        }
        frame_fence.value = 0;
        frame_fence.frame = frame_index++;
        ThrowIfFailed(cp_device->CreateFence(frame_fence.value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frame_fence.cp_fence)));
    }

    ThrowIfFailed(cp_device->CreateFence(m_upload_fence.value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_upload_fence.cp_fence)));

    if (m_fence_event)
    {
        CloseHandle(m_fence_event);
    }

    m_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_fence_event)
    {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }
}

void ContextDX::OnCommandQueueCompleted(CommandQueue& /*cmd_queue*/, uint32_t /*frame_index*/)
{
    ITT_FUNCTION_TASK();
}

void ContextDX::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();
    ContextBase::SetName(name);

    GetDevice().SetName(name + " Device");

    const std::wstring wname = nowide::widen(name);
    for (FrameFence& frame_fence : m_frame_fences)
    {
        assert(!!frame_fence.cp_fence);
        if (!frame_fence.cp_fence) continue;
        frame_fence.cp_fence->SetName((wname + L" Fence " + std::to_wstring(frame_fence.frame)).c_str());
    }
}

const DeviceDX& ContextDX::GetDeviceDX() const
{
    ITT_FUNCTION_TASK();
    return static_cast<const DeviceDX&>(GetDeviceBase());
}

void ContextDX::WaitForGpu(WaitFor wait_for)
{
    ITT_FUNCTION_TASK();

    switch (wait_for)
    {
    case WaitFor::ResourcesUploaded:
        SignalFence(m_upload_fence, static_cast<CommandQueueDX&>(GetUploadCommandQueue()));
        WaitFence(m_upload_fence, true);
        break;

    case WaitFor::FramePresented:
        WaitFence(GetCurrentFrameFence(), true);
        break;

    case WaitFor::RenderComplete:
        for (uint32_t frame_buffer_index = 0; frame_buffer_index < m_settings.frame_buffers_count; ++frame_buffer_index)
        {
            WaitFence(m_frame_fences[frame_buffer_index], false);
        }
        break;
    }

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

void ContextDX::Reset(Device& device)
{
    ITT_FUNCTION_TASK();
    if (m_sp_device && std::addressof(*m_sp_device) == std::addressof(device))
        return;

    if (m_sp_device)
    {
        WaitForGpu(WaitFor::RenderComplete);
        static_cast<DeviceDX&>(*m_sp_device).ReleaseNativeDevice();
    }

    ContextBase::ResetInternal(static_cast<DeviceBase&>(device));

    SafeRelease(m_cp_swap_chain);
    Initialize(static_cast<DeviceDX&>(device));

    ContextBase::Reset(device);
}

void ContextDX::Present()
{
    ITT_FUNCTION_TASK();
    assert(m_cp_swap_chain);

    // Preset frame to screen
    const uint32_t present_flags  = 0; // DXGI_PRESENT_DO_NOT_WAIT
    const uint32_t vsync_interval = GetPresentVSyncInterval();
    ThrowIfFailed(m_cp_swap_chain->Present(vsync_interval, present_flags));

    // Schedule a signal command in the queue for a currently finished frame
    SignalFence(GetCurrentFrameFence(), static_cast<CommandQueueDX&>(GetRenderCommandQueue()));

    OnPresentComplete();

    // Update current frame buffer index
    m_frame_buffer_index = m_cp_swap_chain->GetCurrentBackBufferIndex();
}

void ContextDX::SignalFence(const FrameFence& frame_fence, CommandQueueDX& dx_command_queue)
{
    wrl::ComPtr<ID3D12CommandQueue>& cp_command_queue = dx_command_queue.GetNativeCommandQueue();
    assert(!!cp_command_queue);
    assert(!!frame_fence.cp_fence);

    ThrowIfFailed(cp_command_queue->Signal(frame_fence.cp_fence.Get(), frame_fence.value));
}

void ContextDX::WaitFence(FrameFence& frame_fence, bool increment_value)
{
    assert(!!frame_fence.cp_fence);
    assert(!!m_fence_event);

    // If the next frame is not ready to be rendered yet, wait until it is ready
    if (frame_fence.cp_fence->GetCompletedValue() < frame_fence.value)
    {
        ThrowIfFailed(frame_fence.cp_fence->SetEventOnCompletion(frame_fence.value, m_fence_event));
        WaitForSingleObjectEx(m_fence_event, INFINITE, FALSE);
    }

    // Set the fence value for the next frame
    if (increment_value)
    {
        frame_fence.value++;
    }
}

CommandQueueDX& ContextDX::DefaultCommandQueueDX()
{
    ITT_FUNCTION_TASK();
    return static_cast<class CommandQueueDX&>(GetRenderCommandQueue());
}
