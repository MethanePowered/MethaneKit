/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/CommandQueue.h
Methane CommandQueue PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include <Methane/Pimpl.h>

#include <Methane/Graphics/RHI/ICommandQueue.h>

namespace Methane::Graphics::META_GFX_NAME
{
class CommandQueue;
}

namespace Methane::Graphics::Rhi
{

class CommandKit;
class CommandListSet;
class Fence;
class RenderPass;
class RenderContext;
class ComputeContext;
class RenderCommandList;
class ParallelRenderCommandList;
class TransferCommandList;
class ComputeCommandList;

class CommandQueue // NOSONAR - constructors and assignment operators are required to use forward declared Impl and Ptr<Impl> in header
{
public:
    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(CommandQueue);
    META_PIMPL_METHODS_COMPARE_DECLARE(CommandQueue);

    META_PIMPL_API explicit CommandQueue(const Ptr<ICommandQueue>& interface_ptr);
    META_PIMPL_API explicit CommandQueue(ICommandQueue& interface_ref);
    META_PIMPL_API CommandQueue(const RenderContext& context, CommandListType command_lists_type);
    META_PIMPL_API CommandQueue(const ComputeContext& context, CommandListType command_lists_type);

    META_PIMPL_API bool IsInitialized() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API ICommandQueue& GetInterface() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API Ptr<ICommandQueue> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    META_PIMPL_API bool SetName(std::string_view name) const;
    META_PIMPL_API std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // Data::IEmitter<IObjectCallback> interface methods
    META_PIMPL_API void Connect(Data::Receiver<IObjectCallback>& receiver) const;
    META_PIMPL_API void Disconnect(Data::Receiver<IObjectCallback>& receiver) const;

    // ICommandQueue interface methods
    [[nodiscard]] META_PIMPL_API CommandKit                      CreateCommandKit() const;
    [[nodiscard]] META_PIMPL_API Fence                           CreateFence() const;
    [[nodiscard]] META_PIMPL_API TransferCommandList             CreateTransferCommandList() const;
    [[nodiscard]] META_PIMPL_API ComputeCommandList              CreateComputeCommandList() const;
    [[nodiscard]] META_PIMPL_API RenderCommandList               CreateRenderCommandList(const RenderPass& render_pass) const;
    [[nodiscard]] META_PIMPL_API ParallelRenderCommandList       CreateParallelRenderCommandList(const RenderPass& render_pass) const;
    [[nodiscard]] META_PIMPL_API const IContext&                 GetContext() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API CommandListType                 GetCommandListType() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API uint32_t                        GetFamilyIndex() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API const Ptr<ITimestampQueryPool>& GetTimestampQueryPoolPtr();
    META_PIMPL_API void Execute(const CommandListSet& command_lists, const ICommandList::CompletedCallback& completed_callback = {}) const;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::CommandQueue;

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_PIMPL_INLINE

#include <Methane/Graphics/RHI/CommandQueue.cpp>

#endif // META_PIMPL_INLINE
