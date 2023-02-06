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

FILE: Methane/Graphics/Null/ProgramBindings.h
Null implementation of the program bindings interface.

******************************************************************************/

#include <Methane/Graphics/Null/ProgramBindings.h>
#include <Methane/Graphics/Null/Program.h>
#include <Methane/Graphics/Null/Device.h>

namespace Methane::Graphics::Null
{

Ptr<Rhi::IProgramBindings> ProgramBindings::CreateCopy(const ResourceViewsByArgument& replace_resource_views_by_argument, const Opt<Data::Index>& frame_index)
{
    META_FUNCTION_TASK();
    return std::make_shared<ProgramBindings>(*this, replace_resource_views_by_argument, frame_index);
}

} // namespace Methane::Graphics::Null