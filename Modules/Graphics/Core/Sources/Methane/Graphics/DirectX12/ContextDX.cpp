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
#include "CommandQueueDX.h"
#include "TypesDX.h"

#include <Methane/Data/Instrumentation.h>
#include <Methane/Data/ScopeTimer.h>
#include <Methane/Graphics/Windows/Helpers.h>

#ifdef COMMAND_EXECUTION_LOGGING
#include <Methane/Platform/Utils.h>
#endif

#include <shellscalingapi.h>
#include <nowide/convert.hpp>
#include <cassert>

namespace Methane::Graphics
{

static void SetWindowTopMostFlag(HWND window_handle, bool is_top_most)
{
    ITT_FUNCTION_TASK();

    RECT window_rect = {};
    GetWindowRect(window_handle, &window_rect);

    const HWND window_position = is_top_most ? HWND_TOPMOST : HWND_NOTOPMOST;
    SetWindowPos(window_handle, window_position,
                 window_rect.left,    window_rect.top,
                 window_rect.right  - window_rect.left,
                 window_rect.bottom - window_rect.top,
                 SWP_FRAMECHANGED | SWP_NOACTIVATE);
}

static float GetDeviceScaleRatio(DEVICE_SCALE_FACTOR device_scale_factor)
{
    ITT_FUNCTION_TASK();

    switch (device_scale_factor)
    {
    case SCALE_100_PERCENT: return 1.0f;
    case SCALE_120_PERCENT: return 1.2f;
    case SCALE_125_PERCENT: return 1.25f;
    case SCALE_140_PERCENT: return 1.4f;
    case SCALE_150_PERCENT: return 1.5f;
    case SCALE_160_PERCENT: return 1.6f;
    case SCALE_175_PERCENT: return 1.75f;
    case SCALE_180_PERCENT: return 1.8f;
    case SCALE_200_PERCENT: return 2.f;
    case SCALE_225_PERCENT: return 2.25f;
    case SCALE_250_PERCENT: return 2.5f;
    case SCALE_300_PERCENT: return 3.f;
    case SCALE_350_PERCENT: return 3.5f;
    case SCALE_400_PERCENT: return 4.f;
    case SCALE_450_PERCENT: return 4.5f;
    case SCALE_500_PERCENT: return 5.f;
    default:                assert(0);
    }

    return 1.f;
}

Context::Ptr Context::Create(const Platform::AppEnvironment& env, Device& device, const Context::Settings& settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ContextDX>(env, static_cast<DeviceBase&>(device), settings);
}

ContextDX::ContextDX(const Platform::AppEnvironment& env, DeviceBase& device, const Context::Settings& settings)
    : ContextBase(device, settings)
    , m_platform_env(env)
{
    ITT_FUNCTION_TASK();
    Initialize(device, true);
}

ContextDX::~ContextDX()
{
    ITT_FUNCTION_TASK();
}

void ContextDX::Release()
{
    ITT_FUNCTION_TASK();

    m_cp_swap_chain.Reset();
    m_sp_upload_fence.reset();
    m_sp_render_fence.reset();
    m_frame_fences.clear();

    if (m_sp_device)
    {
        static_cast<DeviceDX&>(*m_sp_device).ReleaseNativeDevice();
        m_sp_device.reset();
    }

    ContextBase::Release();

    static_cast<SystemDX&>(System::Get()).ReportLiveObjects();
}

void ContextDX::Initialize(Device& device, bool deferred_heap_allocation)
{
    ITT_FUNCTION_TASK();

    m_sp_device = static_cast<DeviceBase&>(device).GetPtr();

    // DXGI does not allow creating a swapchain targeting a window which has fullscreen styles(no border + topmost)
    if (m_settings.is_full_screen)
    {
        // Temporary remove top-most flag and restore it when swap-chain is created
        SetWindowTopMostFlag(m_platform_env.window_handle, false);
    }

    // Initialize swap-chain

    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
    swap_chain_desc.Width                 = m_settings.frame_size.width;
    swap_chain_desc.Height                = m_settings.frame_size.height;
    swap_chain_desc.Format                = TypeConverterDX::DataFormatToDXGI(m_settings.color_format);
    swap_chain_desc.BufferCount           = m_settings.frame_buffers_count;
    swap_chain_desc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_chain_desc.SampleDesc.Count      = 1;

    const wrl::ComPtr<IDXGIFactory5>& cp_dxgi_factory = SystemDX::Get().GetNativeFactory();
    assert(!!cp_dxgi_factory);

    BOOL present_tearing_suport = FALSE;
    ThrowIfFailed(cp_dxgi_factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &present_tearing_suport, sizeof(present_tearing_suport)));
    if (present_tearing_suport)
    {
        swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    wrl::ComPtr<ID3D12CommandQueue>& cp_command_queue = GetRenderCommandQueueDX().GetNativeCommandQueue();
    assert(!!cp_command_queue);

    wrl::ComPtr<IDXGISwapChain1> cp_swap_chain;
    ThrowIfFailed(cp_dxgi_factory->CreateSwapChainForHwnd(cp_command_queue.Get(), m_platform_env.window_handle, &swap_chain_desc, NULL, NULL, &cp_swap_chain));
    assert(!!cp_swap_chain);

    if (m_settings.is_full_screen)
    {
        // Restore top-most flag
        SetWindowTopMostFlag(m_platform_env.window_handle, true);
    }

    ThrowIfFailed(cp_swap_chain.As(&m_cp_swap_chain));

    // With tearing support enabled we will handle ALT+Enter key presses in the window message loop rather than let DXGI handle it by calling SetFullscreenState
    ThrowIfFailed(cp_dxgi_factory->MakeWindowAssociation(m_platform_env.window_handle, DXGI_MWA_NO_ALT_ENTER));

    // Initialize frame fences

    assert(!!m_cp_swap_chain);
    m_frame_buffer_index = m_cp_swap_chain->GetCurrentBackBufferIndex();

    const wrl::ComPtr<ID3D12Device>& cp_device = static_cast<DeviceDX&>(device).GetNativeDevice();
    assert(!!cp_device);

    m_frame_fences.clear();
    for (uint32_t frame_index = 0; frame_index < m_settings.frame_buffers_count; ++frame_index)
    {
        m_frame_fences.emplace_back(std::make_unique<FenceDX>(GetRenderCommandQueueDX(), frame_index));
    }

    m_sp_render_fence = std::make_unique<FenceDX>(GetRenderCommandQueueDX());
    m_sp_upload_fence = std::make_unique<FenceDX>(GetUploadCommandQueueDX());

    SetName(GetName());

    ContextBase::Initialize(device, deferred_heap_allocation);
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

    for (FenceDX::Ptr& sp_frame_fence : m_frame_fences)
    {
        assert(!!sp_frame_fence);
        sp_frame_fence->SetName(name + " Frame " + std::to_string(sp_frame_fence->GetFrame()) + " Fence");
    }

    m_sp_render_fence->SetName(name + " Render Fence");
    m_sp_upload_fence->SetName(name + " Upload Fence");
}

const DeviceDX& ContextDX::GetDeviceDX() const
{
    ITT_FUNCTION_TASK();
    return static_cast<const DeviceDX&>(GetDeviceBase());
}

void ContextDX::WaitForGpu(WaitFor wait_for)
{
    ITT_FUNCTION_TASK();

    ContextBase::WaitForGpu(wait_for);

    switch (wait_for)
    {
    case WaitFor::ResourcesUploaded:
    {
        SCOPE_TIMER("ContextDX::WaitForGpu::ResourcesUploaded");
        assert(!!m_sp_upload_fence);
        m_sp_upload_fence->Flush();
    } break;

    case WaitFor::RenderComplete:
    {
        SCOPE_TIMER("ContextDX::WaitForGpu::RenderComplete");
        assert(m_sp_render_fence);
        m_sp_render_fence->Flush();
    } break;

    case WaitFor::FramePresented:
    {
        SCOPE_TIMER("ContextDX::WaitForGpu::FramePresented");
        GetCurrentFrameFence().Wait();
    } break;
    }

    ContextBase::OnGpuWaitComplete(wait_for);
}

void ContextDX::Resize(const FrameSize& frame_size)
{
    ITT_FUNCTION_TASK();

    WaitForGpu(WaitFor::RenderComplete);

    ContextBase::Resize(frame_size);

    // Resize the swap chain to the desired dimensions
    DXGI_SWAP_CHAIN_DESC1 desc = {};
    m_cp_swap_chain->GetDesc1(&desc);
    ThrowIfFailed(m_cp_swap_chain->ResizeBuffers(m_settings.frame_buffers_count, frame_size.width, frame_size.height, desc.Format, desc.Flags));

    m_frame_buffer_index = m_cp_swap_chain->GetCurrentBackBufferIndex();
}

void ContextDX::Present()
{
    ITT_FUNCTION_TASK();
    SCOPE_TIMER("ContextDX::Present");

    ContextBase::Present();

    // Preset frame to screen
    const uint32_t present_flags  = 0; // DXGI_PRESENT_DO_NOT_WAIT
    const uint32_t vsync_interval = GetPresentVSyncInterval();

    assert(m_cp_swap_chain);
    ThrowIfFailed(m_cp_swap_chain->Present(vsync_interval, present_flags));

    // Schedule a signal command in the queue for a currently finished frame
    GetCurrentFrameFence().Signal();

    OnCpuPresentComplete();

    // Update current frame buffer index
    m_frame_buffer_index = m_cp_swap_chain->GetCurrentBackBufferIndex();
}

float ContextDX::GetContentScalingFactor() const
{
    DEVICE_SCALE_FACTOR device_scale_factor = DEVICE_SCALE_FACTOR_INVALID;
    HMONITOR monitor_handle = MonitorFromWindow(m_platform_env.window_handle, MONITOR_DEFAULTTONEAREST);
    ThrowIfFailed(GetScaleFactorForMonitor(monitor_handle, &device_scale_factor));
    return GetDeviceScaleRatio(device_scale_factor);
}

CommandQueueDX& ContextDX::GetUploadCommandQueueDX()
{
    ITT_FUNCTION_TASK();
    return static_cast<CommandQueueDX&>(GetUploadCommandQueue());
}

CommandQueueDX& ContextDX::GetRenderCommandQueueDX()
{
    ITT_FUNCTION_TASK();
    return static_cast<CommandQueueDX&>(GetRenderCommandQueue());
}

ContextDX::FenceDX& ContextDX::GetCurrentFrameFence()
{
    FenceDX::Ptr& sp_current_fence = GetCurrentFrameFencePtr();
    assert(!!sp_current_fence);
    return *sp_current_fence;
}

ContextDX::FenceDX::FenceDX(CommandQueueDX& command_queue, uint32_t frame)
    : m_command_queue(command_queue)
    , m_frame(frame)
    , m_event(CreateEvent(nullptr, FALSE, FALSE, nullptr))
{
    ITT_FUNCTION_TASK();
    if (!m_event)
    {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    const wrl::ComPtr<ID3D12Device>& cp_device = m_command_queue.GetContextDX().GetDeviceDX().GetNativeDevice();
    assert(!!cp_device);

    ThrowIfFailed(cp_device->CreateFence(m_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_cp_fence)));
}

ContextDX::FenceDX::~FenceDX()
{
    ITT_FUNCTION_TASK();
    SafeCloseHandle(m_event);
}

void ContextDX::FenceDX::Signal()
{
    ITT_FUNCTION_TASK();
    wrl::ComPtr<ID3D12CommandQueue>& cp_command_queue = m_command_queue.GetNativeCommandQueue();
    assert(!!cp_command_queue);
    assert(!!m_cp_fence);

    m_value++;

#ifdef COMMAND_EXECUTION_LOGGING
    Platform::PrintToDebugOutput("SIGNAL fence \"" + m_name + "\" with value " + std::to_string(m_value));
#endif

    ThrowIfFailed(cp_command_queue->Signal(m_cp_fence.Get(), m_value));
}

void ContextDX::FenceDX::Wait()
{
    ITT_FUNCTION_TASK();
    assert(!!m_cp_fence);
    assert(!!m_event);

#ifdef COMMAND_EXECUTION_LOGGING
    Platform::PrintToDebugOutput("WAIT fence \"" + m_name + "\" with value " + std::to_string(m_value));
#endif

    if (m_cp_fence->GetCompletedValue() < m_value)
    {
        ThrowIfFailed(m_cp_fence->SetEventOnCompletion(m_value, m_event));
        WaitForSingleObjectEx(m_event, INFINITE, FALSE);
    }
}

void ContextDX::FenceDX::Flush()
{
    ITT_FUNCTION_TASK();
    Signal();
    Wait();
}

void ContextDX::FenceDX::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();
    if (m_name == name)
        return;

    m_name = name;

    assert(!!m_cp_fence);
    m_cp_fence->SetName(nowide::widen(name).c_str());
}

} // namespace Methane::Graphics
