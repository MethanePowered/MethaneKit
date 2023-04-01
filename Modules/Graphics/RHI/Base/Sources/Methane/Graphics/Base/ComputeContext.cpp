/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Base/ComputeContext.cpp
Base implementation of the compute context interface.

******************************************************************************/

#include <Methane/Graphics/Base/ComputeContext.h>
#include <Methane/Graphics/Base/Device.h>

#include <Methane/Graphics/TypeFormatters.hpp>
#include <Methane/Graphics/RHI/ICommandKit.h>
#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Base
{

ComputeContext::ComputeContext(Device& device, UniquePtr<Rhi::IDescriptorManager>&& descriptor_manager_ptr,
                               tf::Executor& parallel_executor, const Settings& settings)
    : Context(device, std::move(descriptor_manager_ptr), parallel_executor, Type::Compute)
    , m_settings(settings)
{
    META_FUNCTION_TASK();
}

void ComputeContext::WaitForGpu(WaitFor wait_for)
{
    META_FUNCTION_TASK();
    Context::WaitForGpu(wait_for);

    switch (wait_for)
    {
    case WaitFor::RenderComplete:
    case WaitFor::ComputeComplete:
        WaitForGpuComputeComplete(); break;
    case WaitFor::ResourcesUploaded: break; // Handled in Context::WaitForGpu
    default: META_UNEXPECTED_ARG(wait_for);
    }
}

void ComputeContext::WaitForGpuComputeComplete()
{
    META_FUNCTION_TASK();
    META_SCOPE_TIMER("ComputeContextDX::WaitForGpu::ComputeComplete");
    GetComputeFence().FlushOnCpu();
    META_CPU_FRAME_DELIMITER(0, 0);
}

Rhi::IFence& ComputeContext::GetComputeFence() const
{
    META_FUNCTION_TASK();
    return GetComputeCommandKit().GetFence(0U);
}

void ComputeContext::ResetWithSettings(const Settings& settings)
{
    META_FUNCTION_TASK();
    META_LOG("Render context '{}' RESET with new settings", GetName());

    WaitForGpu(WaitFor::ComputeComplete);

    Ptr<Device> device_ptr = GetBaseDevice().GetPtr<Device>();
    m_settings = settings;

    Release();
    Initialize(*device_ptr, true);
}

void ComputeContext::Initialize(Device& device, bool is_callback_emitted)
{
    META_FUNCTION_TASK();
    Context::Initialize(device, false);

    if (is_callback_emitted)
    {
        Data::Emitter<Rhi::IContextCallback>::Emit(&Rhi::IContextCallback::OnContextInitialized, *this);
    }
}

bool ComputeContext::UploadResources() const
{
    META_FUNCTION_TASK();
    if (!Context::UploadResources())
        return false;

    // Compute commands will wait for resources uploading completion in upload queue
    GetUploadCommandKit().GetFence().FlushOnGpu(GetComputeCommandKit().GetQueue());
    return true;
}

} // namespace Methane::Graphics::Base
