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

FILE: Methane/Graphics/RHI/ICommandListSet.h
Methane command list interface: this is uncreatable common command list interface,
to create instance refer to IRenderCommandList, etc. for specific derived interface.

******************************************************************************/

#pragma once

#include <Methane/Memory.hpp>
#include <Methane/Data/Types.h>

namespace Methane::Graphics::Rhi
{

struct ICommandList;

struct ICommandListSet
{
    [[nodiscard]] static Ptr<ICommandListSet> Create(const Refs<ICommandList>& command_list_refs, Opt<Data::Index> frame_index_opt = {});

    [[nodiscard]] virtual Data::Size                GetCount() const noexcept = 0;
    [[nodiscard]] virtual const Refs<ICommandList>& GetRefs() const noexcept = 0;
    [[nodiscard]] virtual ICommandList&             operator[](Data::Index index) const = 0;
    [[nodiscard]] virtual const Opt<Data::Index>&   GetFrameIndex() const noexcept = 0;
    [[nodiscard]] virtual Ptr<ICommandListSet>      GetPtr() = 0;

    virtual ~ICommandListSet() = default;
};

} // namespace Methane::Graphics::Rhi
