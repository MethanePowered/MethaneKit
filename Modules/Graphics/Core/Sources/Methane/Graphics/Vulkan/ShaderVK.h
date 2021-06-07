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

FILE: Methane/Graphics/Vulkan/ShaderVK.h
Vulkan implementation of the shader interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ShaderBase.h>

#include <vulkan/vulkan.hpp>

#include <string>
#include <memory>

namespace Methane::Graphics
{

struct IContextVK;
class ProgramVK;

class ShaderVK final : public ShaderBase
{
public:
    ShaderVK(Shader::Type shader_type, const ContextBase& context, const Settings& settings);
    ~ShaderVK() override;

    // ShaderBase interface
    ArgumentBindings GetArgumentBindings(const Program::ArgumentAccessors& argument_accessors) const override;

    const vk::ShaderModule& GetNativeModule() const noexcept { return m_vk_module; }
    vk::PipelineShaderStageCreateInfo GetNativeStageCreateInfo() const;

private:
    const IContextVK& GetContextVK() const noexcept;

    vk::ShaderModule m_vk_module;
};

} // namespace Methane::Graphics
