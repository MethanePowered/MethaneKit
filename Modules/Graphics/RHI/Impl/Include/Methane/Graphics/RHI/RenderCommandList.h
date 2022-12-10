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

FILE: Methane/Graphics/RHI/RenderCommandList.h
Methane RenderCommandList PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RHI/IRenderCommandList.h>

namespace Methane::Graphics::Rhi
{

class CommandQueue;
class RenderPass;

class RenderCommandList
{
public:
    using Primitive = RenderPrimitive;

    RenderCommandList() = default;
    RenderCommandList(const Ptr<IRenderCommandList>& interface_ptr);
    RenderCommandList(const CommandQueue& command_queue, const RenderPass& render_pass);

    void Init(const CommandQueue& command_queue, const RenderPass& render_pass);
    void Release();

    bool IsInitialized() const noexcept;
    IRenderCommandList& GetInterface() const noexcept;

    bool SetName(const std::string& name) const;
    const std::string& GetName() const noexcept;

private:
    class Impl;

    UniquePtr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi
