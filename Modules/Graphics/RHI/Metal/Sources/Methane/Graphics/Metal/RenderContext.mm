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

FILE: Methane/Graphics/Metal/RenderContext.mm
Metal implementation of the render context interface.

******************************************************************************/

#include <Methane/Graphics/Metal/RenderContext.hh>
#include <Methane/Graphics/Metal/RenderPass.hh>
#include <Methane/Graphics/Metal/RenderState.hh>
#include <Methane/Graphics/Metal/RenderPattern.hh>
#include <Methane/Graphics/Metal/Types.hh>
#include <Methane/Graphics/Metal/RenderContextAppView.hh>

#include <Methane/Instrumentation.h>
#include <Methane/Platform/Apple/Types.hh>

// Either use dispatch queue semaphore or fence primitives for CPU-GPU frames rendering synchronization
// NOTE: when fences are used for frames synchronization,
// application runs slower than expected when started from XCode, but runs normally when started from Finder
//#define USE_DISPATCH_QUEUE_SEMAPHORE

// Enables automatic capture of all initialization commands before the first frame rendering
//#define CAPTURE_INITIALIZATION_SCOPE

namespace Methane::Graphics::Metal
{

RenderContext::RenderContext(const Platform::AppEnvironment& env, Base::Device& device, tf::Executor& parallel_executor, const Rhi::RenderContextSettings& settings)
    : Context<Base::RenderContext>(device, parallel_executor, settings)
    , m_app_view(CreateRenderContextAppView(env, settings))
    , m_frame_capture_scope([[MTLCaptureManager sharedCaptureManager] newCaptureScopeWithDevice:Context<Base::RenderContext>::GetMetalDevice().GetNativeDevice()])
#ifdef USE_DISPATCH_QUEUE_SEMAPHORE
    , m_dispatch_semaphore(dispatch_semaphore_create(settings.frame_buffers_count))
#endif
{
    META_FUNCTION_TASK();
    META_UNUSED(m_dispatch_semaphore);

    m_frame_capture_scope.label = MacOS::ConvertToNsString(fmt::format("{} Frame Scope", device.GetName()));
    [MTLCaptureManager sharedCaptureManager].defaultCaptureScope = m_frame_capture_scope;

#ifdef CAPTURE_INITIALIZATION_SCOPE
    // Begin frame capture scope at context creation to enable capturing all initialization commands
    Capture(m_frame_capture_scope);
    BeginFrameCaptureScope();
#endif

    // Start redrawing main view
    m_app_view.redrawing = YES;
}

RenderContext::~RenderContext()
{
    META_FUNCTION_TASK();

#ifdef USE_DISPATCH_QUEUE_SEMAPHORE
    dispatch_release(m_dispatch_semaphore);
#endif
}

Ptr<Rhi::IRenderState> RenderContext::CreateRenderState(const Rhi::RenderStateSettings& settings) const
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderState>(*this, settings);
}

Ptr<Rhi::IRenderPattern> RenderContext::CreateRenderPattern(const Rhi::RenderPatternSettings& settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderPattern>(*this, settings);
}

void RenderContext::Release()
{
    META_FUNCTION_TASK();
    
    m_app_view.redrawing = NO;
    
#ifdef USE_DISPATCH_QUEUE_SEMAPHORE
    dispatch_release(m_dispatch_semaphore);
#endif

    Context<Base::RenderContext>::Release();
}

void RenderContext::Initialize(Base::Device& device, bool is_callback_emitted)
{
    META_FUNCTION_TASK();

    Context<Base::RenderContext>::Initialize(device, is_callback_emitted);
    
#ifdef USE_DISPATCH_QUEUE_SEMAPHORE
    m_dispatch_semaphore = dispatch_semaphore_create(GetSettings().frame_buffers_count);
#endif
    
    m_app_view.redrawing = YES;
}

bool RenderContext::ReadyToRender() const
{
    META_FUNCTION_TASK();
    return m_app_view.redrawing;
}

void RenderContext::WaitForGpu(WaitFor wait_for)
{
    META_FUNCTION_TASK();
    
#ifdef USE_DISPATCH_QUEUE_SEMAPHORE
    if (wait_for != WaitFor::FramePresented)
        Context<Base::RenderContext>::WaitForGpu(wait_for);
#else
    Context<Base::RenderContext>::WaitForGpu(wait_for);
#endif
    
    if (wait_for == WaitFor::FramePresented)
    {
#ifdef USE_DISPATCH_QUEUE_SEMAPHORE
        OnGpuWaitStart(wait_for);
        dispatch_semaphore_wait(m_dispatch_semaphore, DISPATCH_TIME_FOREVER);
        OnGpuWaitComplete(wait_for);
#endif
        BeginFrameCaptureScope();
    }
}

void RenderContext::Resize(const FrameSize& frame_size)
{
    META_FUNCTION_TASK();
    Context<Base::RenderContext>::Resize(frame_size);
}

void RenderContext::Present()
{
    META_FUNCTION_TASK();
    Context<Base::RenderContext>::Present();

    id<MTLCommandBuffer> mtl_cmd_buffer = [GetMetalDefaultCommandQueue(Rhi::CommandListType::Render).GetNativeCommandQueue() commandBuffer];
    mtl_cmd_buffer.label = [NSString stringWithFormat:@"%@ Present Command", GetNsName()];
#ifdef USE_DISPATCH_QUEUE_SEMAPHORE
    [mtl_cmd_buffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull) {
        dispatch_semaphore_signal(m_dispatch_semaphore);
    }];
#endif
    [mtl_cmd_buffer presentDrawable:GetNativeDrawable()];
    [mtl_cmd_buffer commit];

    EndFrameCaptureScope();

#ifdef USE_DISPATCH_QUEUE_SEMAPHORE
    Context<Base::RenderContext>::OnCpuPresentComplete(false);
#else
    Context<Base::RenderContext>::OnCpuPresentComplete(true);
#endif
    
    UpdateFrameBufferIndex();
}

bool RenderContext::SetVSyncEnabled(bool vsync_enabled)
{
    META_FUNCTION_TASK();
    if (Context<Base::RenderContext>::SetVSyncEnabled(vsync_enabled))
    {
        m_app_view.vsyncEnabled = vsync_enabled;
        return true;
    }
    return false;
}

bool RenderContext::SetFrameBuffersCount(uint32_t frame_buffers_count)
{
    META_FUNCTION_TASK();
    frame_buffers_count = std::min(std::max(2U, frame_buffers_count), 3U); // Metal supports only 2 or 3 drawable buffers
    if (Context<Base::RenderContext>::SetFrameBuffersCount(frame_buffers_count))
    {
        m_app_view.drawableCount = frame_buffers_count;
        return true;
    }
    return false;
}

void RenderContext::BeginFrameCaptureScope()
{
    META_FUNCTION_TASK();
    if (m_frame_capture_scope_begun)
    {
        [m_frame_capture_scope endScope];
    }
    else
    {
        m_frame_capture_scope_begun = true;
    }

    [m_frame_capture_scope beginScope];
}

void RenderContext::EndFrameCaptureScope()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_TRUE_DESCR(m_frame_capture_scope_begun, "Metal frame capture scope was not begun");

    [m_frame_capture_scope endScope];
    m_frame_capture_scope_begun = false;
}

void RenderContext::Capture(const id<MTLCaptureScope>& mtl_capture_scope)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(mtl_capture_scope);
    
    MTLCaptureManager*    mtl_capture_manager = [MTLCaptureManager sharedCaptureManager];
    MTLCaptureDescriptor* mtl_capture_desc    = [[MTLCaptureDescriptor alloc] init];
    mtl_capture_desc.captureObject = mtl_capture_scope;

    NSError* ns_error = nil;
    const bool capture_success = [mtl_capture_manager startCaptureWithDescriptor:mtl_capture_desc error:&ns_error];
    META_CHECK_ARG_TRUE_DESCR(capture_success, "failed to capture Metal scope '{}', error: {}",
                              MacOS::ConvertFromNsString(mtl_capture_scope.label),
                              MacOS::ConvertFromNsString([ns_error localizedDescription]));
}

} // namespace Methane::Graphics::Metal
