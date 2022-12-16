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

FILE: Methane/Graphics/Base/CommandListDebugGroup.h
Base implementation of the command list debug group interface.

******************************************************************************/

#pragma once

#include "Object.h"

#include <Methane/Graphics/RHI/ICommandListDebugGroup.h>
#include <Methane/Memory.hpp>

namespace Methane::Graphics::Base
{

class CommandListDebugGroup
    : public Rhi::ICommandListDebugGroup
    , public Object
{
public:
    explicit CommandListDebugGroup(std::string_view name);

    // IObject overrides
    bool SetName(std::string_view) override;

    // IDebugGroup interface
    Rhi::ICommandListDebugGroup& AddSubGroup(Data::Index id, const std::string& name) final;
    Rhi::ICommandListDebugGroup* GetSubGroup(Data::Index id) const noexcept final;
    bool                         HasSubGroups() const noexcept final { return !m_sub_groups.empty(); }

private:
    Ptrs<Rhi::ICommandListDebugGroup> m_sub_groups;
};

} // namespace Methane::Graphics::Base
