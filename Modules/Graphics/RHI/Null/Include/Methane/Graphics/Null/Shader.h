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

FILE: Methane/Graphics/Null/Shader.h
Null implementation of the shader interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/Shader.h>

#include <map>

namespace Methane::Graphics::Null
{

struct ResourceArgumentDesc
{
    Rhi::ResourceType resource_type;
    uint32_t resource_count;
    uint32_t buffer_size;
};

using ResourceArgumentDescs = std::map<Rhi::ProgramArgumentAccessor, ResourceArgumentDesc>;

class Shader final
    : public Base::Shader
{
public:
    using Base::Shader::Shader;

    // Base::Shader interface
    Ptrs<Base::ProgramArgumentBinding> GetArgumentBindings(const Rhi::ProgramArgumentAccessors& argument_accessors) const override;

    void InitArgumentBindings(const ResourceArgumentDescs& argument_descriptions);

private:
    ResourceArgumentDescs m_argument_descriptions;
};

} // namespace Methane::Graphics::Null
