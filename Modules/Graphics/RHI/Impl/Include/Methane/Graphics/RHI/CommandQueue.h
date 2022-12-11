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

#include "Pimpl.h"

#include <Methane/Graphics/RHI/ICommandQueue.h>

namespace Methane::Graphics::Rhi
{

class RenderContext;
class CommandListSet;

class CommandQueue
{
public:
    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(CommandQueue);

    CommandQueue(const Ptr<ICommandQueue>& interface_ptr);
    CommandQueue(ICommandQueue& interface_ref);
    CommandQueue(const RenderContext& context, CommandListType command_lists_type);

    void Init(const RenderContext& context, CommandListType command_lists_type);
    void Release();

    bool IsInitialized() const META_PIMPL_NOEXCEPT;
    ICommandQueue& GetInterface() const META_PIMPL_NOEXCEPT;

    bool SetName(const std::string& name) const;
    const std::string& GetName() const META_PIMPL_NOEXCEPT;

    [[nodiscard]] const IContext&      GetContext() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] CommandListType      GetCommandListType() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] uint32_t             GetFamilyIndex() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] ITimestampQueryPool* GetTimestampQueryPool() const META_PIMPL_NOEXCEPT;
    void Execute(const CommandListSet& command_lists, const ICommandList::CompletedCallback& completed_callback = {}) const;

private:
    class Impl;

    UniquePtr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi
