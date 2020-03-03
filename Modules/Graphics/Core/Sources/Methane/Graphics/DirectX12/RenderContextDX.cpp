/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/DirectX12/RenderContextDX.cpp
DirectX 12 implementation of the render context interface.

******************************************************************************/

#include "RenderContextDX.h"
#include "DeviceDX.h"
#include "CommandQueueDX.h"
#include "TypesDX.h"

#include <Methane/Instrumentation.h>
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

Ptr<RenderContext> RenderContext::Create(const Platform::AppEnvironment& env, Device& device, const RenderContext::Settings& settings)
{
    ITT_FUNCTION_TASK();
    DeviceBase& device_base = static_cast<DeviceBase&>(device);
    Ptr<RenderContextDX> sp_render_context = std::make_shared<RenderContextDX>(env, device_base, settings);
    sp_render_context->Initialize(device_base, true);
    return sp_render_context;
}

RenderContextDX::RenderContextDX(const Platform::AppEnvironment& env, DeviceBase& device, const RenderContext::Settings& settings)
    : ContextDX<RenderContextBase>(device, settings)
    , m_platform_env(env)
{
    ITT_FUNCTION_TASK();
}

RenderContextDX::~RenderContextDX()
{
    ITT_FUNCTION_TASK();
}

void RenderContextDX::Release()
{
    ITT_FUNCTION_TASK();

    m_cp_swap_chain.Reset();

    ContextDX<RenderContextBase>::Release();
}

void RenderContextDX::Initialize(DeviceBase& device, bool deferred_heap_allocation)
{
    ITT_FUNCTION_TASK();

    const Settings& settings = GetSettings();

    SetDevice(device);

    // DXGI does not allow creating a swapchain targeting a window which has fullscreen styles(no border + topmost)
    if (settings.is_full_screen)
    {
        // Temporary remove top-most flag and restore it when swap-chain is created
        SetWindowTopMostFlag(m_platform_env.window_handle, false);
    }

    // Initialize swap-chain

    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
    swap_chain_desc.Width                 = settings.frame_size.width;
    swap_chain_desc.Height                = settings.frame_size.height;
    swap_chain_desc.Format                = TypeConverterDX::DataFormatToDXGI(settings.color_format);
    swap_chain_desc.BufferCount           = settings.frame_buffers_count;
    swap_chain_desc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_chain_desc.SampleDesc.Count      = 1;

    const wrl::ComPtr<IDXGIFactory5>& cp_dxgi_factory = SystemDX::Get().GetNativeFactory();
    assert(!!cp_dxgi_factory);

    BOOL present_tearing_support = FALSE;
    ThrowIfFailed(cp_dxgi_factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &present_tearing_support, sizeof(present_tearing_support)));
    if (present_tearing_support)
    {
        swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    wrl::ComPtr<IDXGISwapChain1> cp_swap_chain;
    ID3D12CommandQueue& dx_command_queue = GetRenderCommandQueueDX().GetNativeCommandQueue();
    ThrowIfFailed(cp_dxgi_factory->CreateSwapChainForHwnd(&dx_command_queue, m_platform_env.window_handle, &swap_chain_desc, NULL, NULL, &cp_swap_chain));
    assert(!!cp_swap_chain);

    if (settings.is_full_screen)
    {
        // Restore top-most flag
        SetWindowTopMostFlag(m_platform_env.window_handle, true);
    }

    ThrowIfFailed(cp_swap_chain.As(&m_cp_swap_chain));

    // With tearing support enabled we will handle ALT+Enter key presses in the window message loop rather than let DXGI handle it by calling SetFullscreenState
    ThrowIfFailed(cp_dxgi_factory->MakeWindowAssociation(m_platform_env.window_handle, DXGI_MWA_NO_ALT_ENTER));

    const wrl::ComPtr<ID3D12Device>& cp_device = static_cast<DeviceDX&>(device).GetNativeDevice();
    assert(!!cp_device);

    UpdateFrameBufferIndex();

    ContextDX<RenderContextBase>::Initialize(device, deferred_heap_allocation);
}

void RenderContextDX::Resize(const FrameSize& frame_size)
{
    ITT_FUNCTION_TASK();

    WaitForGpu(WaitFor::RenderComplete);

    ContextDX<RenderContextBase>::Resize(frame_size);

    // Resize the swap chain to the desired dimensions
    DXGI_SWAP_CHAIN_DESC1 desc = {};
    m_cp_swap_chain->GetDesc1(&desc);
    ThrowIfFailed(m_cp_swap_chain->ResizeBuffers(GetSettings().frame_buffers_count, frame_size.width, frame_size.height, desc.Format, desc.Flags));

    UpdateFrameBufferIndex();
}

void RenderContextDX::Present()
{
    ITT_FUNCTION_TASK();
    SCOPE_TIMER("RenderContextDX::Present");

    ContextDX<RenderContextBase>::Present();

    // Preset frame to screen
    const uint32_t present_flags  = 0; // DXGI_PRESENT_DO_NOT_WAIT
    const uint32_t vsync_interval = GetPresentVSyncInterval();

    assert(m_cp_swap_chain);
    ThrowIfFailed(m_cp_swap_chain->Present(vsync_interval, present_flags));

    OnCpuPresentComplete();
    UpdateFrameBufferIndex();
}

float RenderContextDX::GetContentScalingFactor() const
{
    DEVICE_SCALE_FACTOR device_scale_factor = DEVICE_SCALE_FACTOR_INVALID;
    HMONITOR monitor_handle = MonitorFromWindow(m_platform_env.window_handle, MONITOR_DEFAULTTONEAREST);
    ThrowIfFailed(GetScaleFactorForMonitor(monitor_handle, &device_scale_factor));
    return GetDeviceScaleRatio(device_scale_factor);
}

CommandQueueDX& RenderContextDX::GetRenderCommandQueueDX()
{
    ITT_FUNCTION_TASK();
    return static_cast<CommandQueueDX&>(GetRenderCommandQueue());
}

uint32_t RenderContextDX::GetNextFrameBufferIndex()
{
    ITT_FUNCTION_TASK();
    assert(!!m_cp_swap_chain);
    return m_cp_swap_chain->GetCurrentBackBufferIndex();
}

} // namespace Methane::Graphics
