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

FILE: Methane/Graphics/RHI/ComputeContext.h
Methane ComputeContext PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include <Methane/Pimpl.h>

#include <Methane/Graphics/RHI/IComputeContext.h>

namespace Methane::Graphics::META_GFX_NAME
{
class ComputeContext;
}

namespace Methane::Graphics::Rhi
{

class Device;
class CommandKit;
class CommandQueue;
class Shader;
class Program;
class Buffer;
class Texture;
class Sampler;
class RenderState;
class RenderPattern;

struct ShaderSettings;
struct ProgramSettingsImpl;
struct BufferSettings;
struct TextureSettings;
struct SamplerSettings;

enum class CommandListType;
enum class ShaderType : uint32_t;

class ComputeContext
{
public:
    using Settings              = ComputeContextSettings;
    using Type                  = ContextType;
    using WaitFor               = ContextWaitFor;
    using DeferredAction        = ContextDeferredAction;
    using Option                = ContextOption;
    using OptionMask            = ContextOptionMask;
    using IncompatibleException = ContextIncompatibleException;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(ComputeContext);
    META_PIMPL_METHODS_COMPARE_DECLARE(ComputeContext);

    META_PIMPL_API explicit ComputeContext(const Ptr<IComputeContext>& render_context_ptr);
    META_PIMPL_API explicit ComputeContext(IComputeContext& render_context);
    META_PIMPL_API ComputeContext(const Device& device, tf::Executor& parallel_executor, const Settings& settings);

    META_PIMPL_API bool IsInitialized() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API IComputeContext& GetInterface() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API Ptr<IComputeContext> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    META_PIMPL_API bool SetName(std::string_view name) const;
    META_PIMPL_API std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // Data::IEmitter<IObjectCallback> interface methods
    META_PIMPL_API void Connect(Data::Receiver<IObjectCallback>& receiver) const;
    META_PIMPL_API void Disconnect(Data::Receiver<IObjectCallback>& receiver) const;

    // IContext interface methods
    [[nodiscard]] META_PIMPL_API CommandQueue     CreateCommandQueue(CommandListType type) const;
    [[nodiscard]] META_PIMPL_API CommandKit       CreateCommandKit(CommandListType type) const;
    [[nodiscard]] META_PIMPL_API Shader           CreateShader(ShaderType type, const ShaderSettings& settings) const;
    [[nodiscard]] META_PIMPL_API Program          CreateProgram(const ProgramSettingsImpl& settings) const;
    [[nodiscard]] META_PIMPL_API Buffer           CreateBuffer(const BufferSettings& settings) const;
    [[nodiscard]] META_PIMPL_API Texture          CreateTexture(const TextureSettings& settings) const;
    [[nodiscard]] META_PIMPL_API Sampler          CreateSampler(const SamplerSettings& settings) const;
    [[nodiscard]] META_PIMPL_API OptionMask       GetOptions() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API tf::Executor&    GetParallelExecutor() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API IObjectRegistry& GetObjectRegistry() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API void RequestDeferredAction(DeferredAction action) const META_PIMPL_NOEXCEPT;
    META_PIMPL_API void CompleteInitialization() const;
    [[nodiscard]] META_PIMPL_API bool IsCompletingInitialization() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API void WaitForGpu(WaitFor wait_for) const;
    META_PIMPL_API void Reset(const Device& device) const;
    META_PIMPL_API void Reset() const;
    [[nodiscard]] META_PIMPL_API Device GetDevice() const;
    [[nodiscard]] META_PIMPL_API CommandKit GetDefaultCommandKit(CommandListType type) const;
    [[nodiscard]] META_PIMPL_API CommandKit GetDefaultCommandKit(const CommandQueue& cmd_queue) const;
    [[nodiscard]] META_PIMPL_API CommandKit GetUploadCommandKit() const;
    [[nodiscard]] META_PIMPL_API CommandKit GetComputeCommandKit() const;

    // Data::IEmitter<IContextCallback> interface methods
    META_PIMPL_API void Connect(Data::Receiver<IContextCallback>& receiver) const;
    META_PIMPL_API void Disconnect(Data::Receiver<IContextCallback>& receiver) const;

    // IComputeContext interface methods
    [[nodiscard]] META_PIMPL_API const Settings&    GetSettings() const META_PIMPL_NOEXCEPT;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::ComputeContext;

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_PIMPL_INLINE

#include <Methane/Graphics/RHI/ComputeContext.cpp>

#endif // META_PIMPL_INLINE
