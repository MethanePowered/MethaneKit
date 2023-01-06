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

FILE: Methane/Graphics/Null/CommandList.hpp
Null base template implementation of the command list interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/CommandList.h>

namespace Methane::Graphics::Null
{

template<class CommandListBaseT, typename = std::enable_if_t<std::is_base_of_v<Base::CommandList, CommandListBaseT>>>
class CommandList
    : public CommandListBaseT
{
public:
    template<typename... ConstructArgs>
    CommandList(ConstructArgs&&... construct_args)
        : CommandListBaseT(std::forward<ConstructArgs>(construct_args)...)
    {
    }

    void SetResourceBarriers(const Rhi::IResourceBarriers&) final
    {
        CommandListBaseT::VerifyEncodingState();
    }
};

} // namespace Methane::Graphics::Null
