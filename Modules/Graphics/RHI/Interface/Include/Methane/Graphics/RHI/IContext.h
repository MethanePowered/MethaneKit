/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/IContext.h
Methane base context interface: wraps graphics device used for GPU interaction.

******************************************************************************/

#pragma once

#include "IObject.h"
#include "ICommandList.h"

#include <Methane/Memory.hpp>
#include <Methane/Graphics/Types.h>
#include <Methane/Data/IEmitter.h>

#include <stdexcept>

namespace tf // NOSONAR
{
// TaskFlow Executor class forward declaration:
// #include <taskflow/core/executor.hpp>
class Executor;
}

namespace Methane::Graphics::Rhi
{

enum class ContextType
{
    Render,
};

enum class ContextWaitFor
{
    RenderComplete,
    FramePresented,
    ResourcesUploaded
};

enum class ContextDeferredAction : uint32_t
{
    None = 0U,
    UploadResources,
    CompleteInitialization
};

union ContextOptions
{
    struct
    {
        bool transfer_with_d3d12_direct_queue; // Transfer command lists and queues in DX API are created with DIRECT type instead of COPY type
        bool emulate_d3d12_render_pass;        // Render passes are emulated with traditional DX API, instead of using native DX render pass API
    };

    uint32_t mask = 0U;

    ContextOptions() = default;
    ContextOptions(uint32_t mask) : mask(mask) { }
};


class ContextIncompatibleException
    : public std::runtime_error
{
public:
    using runtime_error::runtime_error;
};

struct IDevice;
struct ICommandKit;
struct IContext;

struct IContextCallback
{
    virtual void OnContextReleased(IContext& context) = 0;
    virtual void OnContextCompletingInitialization(IContext& context) = 0;
    virtual void OnContextInitialized(IContext& context) = 0;

    virtual ~IContextCallback() = default;
};

struct IContext
    : virtual IObject // NOSONAR
    , virtual Data::IEmitter<IContextCallback> // NOSONAR
{
    using Type = ContextType;
    using WaitFor = ContextWaitFor;
    using DeferredAction = ContextDeferredAction;
    using Options = ContextOptions;
    using IncompatibleException = ContextIncompatibleException;

    // IContext interface
    [[nodiscard]] virtual Type GetType() const noexcept = 0;
    [[nodiscard]] virtual Options GetOptions() const noexcept = 0;
    [[nodiscard]] virtual tf::Executor& GetParallelExecutor() const noexcept = 0;
    [[nodiscard]] virtual IObjectRegistry& GetObjectRegistry() noexcept = 0;
    [[nodiscard]] virtual const IObjectRegistry& GetObjectRegistry() const noexcept = 0;
    virtual void RequestDeferredAction(DeferredAction action) const noexcept = 0;
    virtual void CompleteInitialization() = 0;
    [[nodiscard]] virtual bool IsCompletingInitialization() const noexcept = 0;
    virtual void WaitForGpu(WaitFor wait_for) = 0;
    virtual void Reset(IDevice& device) = 0;
    virtual void Reset() = 0;
    [[nodiscard]] virtual const IDevice& GetDevice() const = 0;
    [[nodiscard]] virtual ICommandKit& GetDefaultCommandKit(CommandListType type) const = 0;
    [[nodiscard]] virtual ICommandKit& GetDefaultCommandKit(ICommandQueue& cmd_queue) const = 0;
    [[nodiscard]] inline  ICommandKit& GetUploadCommandKit() const { return GetDefaultCommandKit(CommandListType::Transfer); }
};

} // namespace Methane::Graphics::Rhi
