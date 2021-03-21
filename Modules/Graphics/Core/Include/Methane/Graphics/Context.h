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

FILE: Methane/Graphics/Context.h
Methane base context interface: wraps graphics device used for GPU interaction.

******************************************************************************/

#pragma once

#include "Object.h"
#include "CommandList.h"

#include <Methane/Memory.hpp>
#include <Methane/Graphics/Types.h>
#include <Methane/Data/IEmitter.h>

namespace tf // NOSONAR
{
// TaskFlow Executor class forward declaration:
// #include <taskflow/core/executor.hpp>
class Executor;
}

namespace Methane::Graphics
{

struct Device;
struct CommandQueue;
struct SyncCommandList;
struct BlitCommandList;
struct CommandListSet;
struct Context;

struct IContextCallback
{
    virtual void OnContextReleased(Context& context) = 0;
    virtual void OnContextCompletingInitialization(Context& context) = 0;
    virtual void OnContextInitialized(Context& context) = 0;

    virtual ~IContextCallback() = default;
};

struct Context
    : virtual Object
    , virtual Data::IEmitter<IContextCallback>
{
    enum class Type
    {
        Render,
    };

    enum class WaitFor
    {
        RenderComplete,
        FramePresented,
        ResourcesUploaded
    };

    enum class DeferredAction : uint32_t
    {
        None = 0U,
        UploadResources,
        CompleteInitialization
    };

    enum class Options : uint32_t
    {
        Default = 0U,
        BlitWithCopyQueueOnWindows  = 1U << 0U, // Blit command lists are created with COPY type instead of DIRECT (requires manual resource states transitions)
        EmulatedRenderPassOnWindows = 1U << 1U, // Render passes are emulated with traditional DX API API, instead of using native DX render-pass API
    };

    // Context interface
    [[nodiscard]] virtual Type GetType() const noexcept = 0;
    [[nodiscard]] virtual Options GetOptions() const noexcept = 0;
    [[nodiscard]] virtual tf::Executor& GetParallelExecutor() const noexcept = 0;
    [[nodiscard]] virtual Object::Registry& GetObjectsRegistry() noexcept = 0;
    virtual void RequestDeferredAction(DeferredAction action) const noexcept = 0;
    virtual void CompleteInitialization() = 0;
    [[nodiscard]] virtual bool IsCompletingInitialization() const noexcept = 0;
    virtual void WaitForGpu(WaitFor wait_for) = 0;
    virtual void Reset(Device& device) = 0;
    virtual void Reset() = 0;

    [[nodiscard]] virtual Device&          GetDevice() = 0;
    [[nodiscard]] virtual CommandQueue&    GetDefaultCommandQueue(CommandList::Type type) = 0;
    [[nodiscard]] virtual CommandQueue&    GetSyncCommandQueue() = 0;
    [[nodiscard]] virtual CommandQueue&    GetUploadCommandQueue() = 0;
    [[nodiscard]] virtual SyncCommandList& GetSyncCommandList() = 0;
    [[nodiscard]] virtual BlitCommandList& GetUploadCommandList() = 0;
    [[nodiscard]] virtual CommandListSet&  GetUploadCommandListSet() = 0;
};

} // namespace Methane::Graphics
