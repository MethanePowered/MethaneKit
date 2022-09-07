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

FILE: Methane/Graphics/DirectX12/RenderContextDX.cpp
DirectX 12 implementation of the render context interface.

******************************************************************************/

#include "RenderContextDX.h"
#include "DeviceDX.h"
#include "CommandQueueDX.h"
#include "TypesDX.h"

#include <Methane/Graphics/Windows/DirectXErrorHandling.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <nowide/convert.hpp>

namespace Methane::Graphics
{

static void SetWindowTopMostFlag(HWND window_handle, bool is_top_most)
{
    META_FUNCTION_TASK();

    RECT window_rect{};
    GetWindowRect(window_handle, &window_rect);

    const HWND window_position = is_top_most ? HWND_TOPMOST : HWND_NOTOPMOST;
    SetWindowPos(window_handle, window_position,
                 window_rect.left,    window_rect.top,
                 window_rect.right  - window_rect.left,
                 window_rect.bottom - window_rect.top,
                 SWP_FRAMECHANGED | SWP_NOACTIVATE);
}

Ptr<RenderContext> RenderContext::Create(const Platform::AppEnvironment& env, Device& device, tf::Executor& parallel_executor, const RenderContext::Settings& settings)
{
    META_FUNCTION_TASK();
    auto& device_base = static_cast<DeviceBase&>(device);
    const auto render_context_ptr = std::make_shared<RenderContextDX>(env, device_base, parallel_executor, settings);
    render_context_ptr->Initialize(device_base, true);
    return render_context_ptr;
}

RenderContextDX::RenderContextDX(const Platform::AppEnvironment& env, DeviceBase& device, tf::Executor& parallel_executor, const RenderContext::Settings& settings)
    : ContextDX<RenderContextBase>(device, parallel_executor, settings)
    , m_platform_env(env)
{
    META_FUNCTION_TASK();
}

RenderContextDX::~RenderContextDX()
{
    META_FUNCTION_TASK();
}

void RenderContextDX::WaitForGpu(WaitFor wait_for)
{
    META_FUNCTION_TASK();
    ContextDX<RenderContextBase>::WaitForGpu(wait_for);

    std::optional<Data::Index> frame_buffer_index;
    CommandList::Type cl_type = CommandList::Type::Render;
    switch (wait_for)
    {
    case WaitFor::RenderComplete:
        break;

    case WaitFor::FramePresented:
        WaitForSwapChainLatency();
        frame_buffer_index = GetFrameBufferIndex();
        break;

    case WaitFor::ResourcesUploaded:
        cl_type = CommandList::Type::Blit;
        break;

    default: META_UNEXPECTED_ARG(wait_for);
    }

    GetDefaultCommandQueueDX(cl_type).CompleteExecution(frame_buffer_index);
}

void RenderContextDX::Release()
{
    META_FUNCTION_TASK();

    m_cp_swap_chain.Reset();

    ContextDX<RenderContextBase>::Release();
}

void RenderContextDX::Initialize(DeviceBase& device, bool is_callback_emitted)
{
    META_FUNCTION_TASK();

    const Settings& settings = GetSettings();

    SetDevice(device);

    // DXGI does not allow creating a swapchain targeting a window which has fullscreen styles(no border + topmost)
    if (settings.is_full_screen)
    {
        // Temporary remove top-most flag and restore it when swap-chain is created
        SetWindowTopMostFlag(m_platform_env.window_handle, false);
    }

    // Initialize swap-chain

    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc{};
    swap_chain_desc.Width            = settings.frame_size.GetWidth();
    swap_chain_desc.Height           = settings.frame_size.GetHeight();
    swap_chain_desc.Format           = TypeConverterDX::PixelFormatToDxgi(settings.color_format);
    swap_chain_desc.Stereo           = FALSE;
    swap_chain_desc.BufferCount      = settings.frame_buffers_count;
    swap_chain_desc.BufferUsage      = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.Scaling          = DXGI_SCALING_NONE;
    swap_chain_desc.SwapEffect       = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_chain_desc.AlphaMode        = DXGI_ALPHA_MODE_IGNORE;
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.Flags            = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT; // requires Windows 8.1

    const wrl::ComPtr<IDXGIFactory5>& cp_dxgi_factory = SystemDX::Get().GetNativeFactory();
    META_CHECK_ARG_NOT_NULL(cp_dxgi_factory);

    BOOL present_tearing_support = FALSE;
    ID3D12Device* p_native_device = GetDeviceDX().GetNativeDevice().Get();
    ThrowIfFailed(cp_dxgi_factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &present_tearing_support, sizeof(present_tearing_support)), p_native_device);
    if (present_tearing_support)
    {
        swap_chain_desc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        m_is_tearing_supported = true;
    }
    else
    {
        m_is_tearing_supported = false;
    }

    wrl::ComPtr<IDXGISwapChain1> cp_swap_chain;
    ID3D12CommandQueue& dx_command_queue = GetDefaultCommandQueueDX(CommandList::Type::Render).GetNativeCommandQueue();
    ThrowIfFailed(cp_dxgi_factory->CreateSwapChainForHwnd(&dx_command_queue, m_platform_env.window_handle, &swap_chain_desc, nullptr, nullptr, &cp_swap_chain), p_native_device);

    META_CHECK_ARG_NOT_NULL(cp_swap_chain);
    ThrowIfFailed(cp_swap_chain.As(&m_cp_swap_chain), p_native_device);

    // Create waitable object to reduce frame latency (https://docs.microsoft.com/en-us/windows/uwp/gaming/reduce-latency-with-dxgi-1-3-swap-chains)
    m_cp_swap_chain->SetMaximumFrameLatency(settings.frame_buffers_count);
    m_frame_latency_waitable_object = m_cp_swap_chain->GetFrameLatencyWaitableObject();
    META_CHECK_ARG_NOT_ZERO_DESCR(m_frame_latency_waitable_object, "swap-chain waitable object is null");

    if (settings.is_full_screen)
    {
        // Restore top-most flag
        SetWindowTopMostFlag(m_platform_env.window_handle, true);
    }

    // With tearing support enabled we will handle ALT+Enter key presses in the window message loop rather than let DXGI handle it by calling SetFullscreenState
    ThrowIfFailed(cp_dxgi_factory->MakeWindowAssociation(m_platform_env.window_handle, DXGI_MWA_NO_ALT_ENTER), p_native_device);

    UpdateFrameBufferIndex();

    ContextDX<RenderContextBase>::Initialize(device, is_callback_emitted);
}

void RenderContextDX::Resize(const FrameSize& frame_size)
{
    META_FUNCTION_TASK();

    WaitForGpu(WaitFor::RenderComplete);

    ContextDX<RenderContextBase>::Resize(frame_size);

    // Resize the swap chain to the desired dimensions
    DXGI_SWAP_CHAIN_DESC1 desc{};
    m_cp_swap_chain->GetDesc1(&desc);
    ThrowIfFailed(m_cp_swap_chain->ResizeBuffers(GetSettings().frame_buffers_count, frame_size.GetWidth(), frame_size.GetHeight(), desc.Format, desc.Flags),
                  GetDeviceDX().GetNativeDevice().Get());

    UpdateFrameBufferIndex();
}

void RenderContextDX::Present()
{
    META_FUNCTION_TASK();
    META_SCOPE_TIMER("RenderContextDX::Present");

    ContextDX<RenderContextBase>::Present();

    // Preset frame to screen
    const uint32_t present_flags  = GetPresentFlags();
    const uint32_t vsync_interval = GetPresentVSyncInterval();

    META_CHECK_ARG_NOT_NULL(m_cp_swap_chain);
    ThrowIfFailed(m_cp_swap_chain->Present(vsync_interval, present_flags),
                  GetDeviceDX().GetNativeDevice().Get());

    ContextDX<RenderContextBase>::OnCpuPresentComplete();
    UpdateFrameBufferIndex();
}

uint32_t RenderContextDX::GetNextFrameBufferIndex()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_cp_swap_chain);
    return m_cp_swap_chain->GetCurrentBackBufferIndex();
}

void RenderContextDX::WaitForSwapChainLatency()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_frame_latency_waitable_object);
    const DWORD frame_latency_wait_result = WaitForSingleObjectEx(
        m_frame_latency_waitable_object,
        1000, // 1 second timeout (should not ever happen)
        true
    );
    META_CHECK_ARG_NOT_EQUAL_DESCR(frame_latency_wait_result, WAIT_TIMEOUT, "timeout reached while waiting for swap-chain latency");
    META_CHECK_ARG_EQUAL_DESCR(frame_latency_wait_result, WAIT_OBJECT_0, "failed to wait for swap-chain latency");
}

} // namespace Methane::Graphics
