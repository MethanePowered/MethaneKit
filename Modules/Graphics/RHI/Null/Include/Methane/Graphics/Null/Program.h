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

FILE: Methane/Graphics/Null/Program.h
Null implementation of the program interface.

******************************************************************************/

#pragma once

#include "Shader.h"

#include <Methane/Graphics/Base/Program.h>

namespace Methane::Graphics::Null
{

class Program final
    : public Base::Program
{
public:
    Program(const Base::Context& context, const Settings& settings);

    // IProgram interface
    [[nodiscard]] Ptr<Rhi::IProgramBindings> CreateBindings(const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index) override;

    void InitArgumentBindings(const ResourceArgumentDescs& argument_descriptions);
};

} // namespace Methane::Graphics::Null
