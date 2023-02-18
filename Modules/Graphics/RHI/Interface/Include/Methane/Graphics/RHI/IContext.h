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

#include <Methane/Memory.hpp>
#include <Methane/Graphics/Types.h>
#include <Methane/Data/IEmitter.h>
#include <Methane/Data/EnumMask.hpp>

#include <stdexcept>

namespace tf // NOSONAR
{
// TaskFlow Executor class forward declaration from <taskflow/core/executor.hpp>
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

enum class ContextOption : uint32_t
{
    TransferWithD3D12DirectQueue, // Transfer command lists and queues in DX API are created with DIRECT type instead of COPY type
    EmulateD3D12RenderPass        // Render passes are emulated with traditional DX API, instead of using native DX render pass API
};

using ContextOptionMask = Data::EnumMask<ContextOption>;

class ContextIncompatibleException
    : public std::runtime_error
{
public:
    using runtime_error::runtime_error;
};

struct IContext;

struct IContextCallback
{
    virtual void OnContextReleased(IContext& context) = 0;
    virtual void OnContextCompletingInitialization(IContext& context) = 0;
    virtual void OnContextInitialized(IContext& context) = 0;

    virtual ~IContextCallback() = default;
};

struct IDevice;
struct ICommandQueue;
struct ICommandKit;
struct IShader;
struct IProgram;
struct IComputeState;
struct IBuffer;
struct ITexture;
struct ISampler;

struct ShaderSettings;
struct ProgramSettings;
struct ComputeStateSettings;
struct BufferSettings;
struct TextureSettings;
struct SamplerSettings;

enum class CommandListType;
enum class ShaderType : uint32_t;

struct IContext
    : virtual IObject // NOSONAR
    , virtual Data::IEmitter<IContextCallback> // NOSONAR
{
    using Type                  = ContextType;
    using WaitFor               = ContextWaitFor;
    using DeferredAction        = ContextDeferredAction;
    using Option                = ContextOption;
    using OptionMask            = ContextOptionMask;
    using IncompatibleException = ContextIncompatibleException;

    // IContext interface
    [[nodiscard]] virtual Ptr<ICommandQueue> CreateCommandQueue(CommandListType type) const = 0;
    [[nodiscard]] virtual Ptr<ICommandKit>   CreateCommandKit(CommandListType type) const = 0;
    [[nodiscard]] virtual Ptr<IShader>       CreateShader(ShaderType type, const ShaderSettings& settings) const = 0;
    [[nodiscard]] virtual Ptr<IProgram>      CreateProgram(const ProgramSettings& settings) const = 0;
    [[nodiscard]] virtual Ptr<IComputeState> CreateComputeState(const ComputeStateSettings& settings) const = 0;
    [[nodiscard]] virtual Ptr<IBuffer>       CreateBuffer(const BufferSettings& settings) const = 0;
    [[nodiscard]] virtual Ptr<ITexture>      CreateTexture(const TextureSettings& settings) const = 0;
    [[nodiscard]] virtual Ptr<ISampler>      CreateSampler(const SamplerSettings& settings) const = 0;
    [[nodiscard]] virtual Type               GetType() const noexcept = 0;
    [[nodiscard]] virtual OptionMask         GetOptions() const noexcept = 0;
    [[nodiscard]] virtual tf::Executor&      GetParallelExecutor() const noexcept = 0;
    [[nodiscard]] virtual IObjectRegistry&   GetObjectRegistry() noexcept = 0;
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

    [[nodiscard]] ICommandKit& GetUploadCommandKit() const;
};

} // namespace Methane::Graphics::Rhi
