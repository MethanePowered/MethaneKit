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

FILE: Methane/Graphics/Vulkan/CommandListDebugGroup.h
Vulkan command list debug group implementation.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/CommandListDebugGroup.h>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics::Vulkan
{

class CommandListDebugGroup final
    : public Base::CommandListDebugGroup
{
public:
    explicit CommandListDebugGroup(std::string_view name);

    const vk::DebugUtilsLabelEXT& GetNativeDebugLabel() const noexcept;

private:
    vk::DebugUtilsLabelEXT m_vk_debug_label;
};

} // namespace Methane::Graphics::Vulkan
