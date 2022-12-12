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

FILE: Methane/Graphics/Vulkan/CommandList.cpp
Vulkan command lists sequence implementation

******************************************************************************/

#include <Methane/Graphics/Vulkan/ICommandList.h>

#include <Methane/Instrumentation.h>

#include <sstream>

namespace Methane::Graphics::Rhi
{

Ptr<ICommandListDebugGroup> Rhi::ICommandListDebugGroup::Create(std::string_view name)
{
    META_FUNCTION_TASK();
    return std::make_shared<Vulkan::CommandListDebugGroup>(name);
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Vulkan
{

CommandListDebugGroup::CommandListDebugGroup(std::string_view name)
    : Base::CommandList::DebugGroup(name)
    , m_vk_debug_label(Base::Object::GetName().data())
{
}

const vk::DebugUtilsLabelEXT& CommandListDebugGroup::GetNativeDebugLabel() const noexcept
{
    return m_vk_debug_label;
}

} // namespace Methane::Graphics::Vulkan
