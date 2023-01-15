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

FILE: Methane/Graphics/RHI/IBufferSet.h
Methane buffer-set interface

******************************************************************************/

#pragma once

#include "IBuffer.h"

namespace Methane::Graphics::Rhi
{

struct IBufferSet
    : virtual IObject // NOSONAR
{
    [[nodiscard]] static Ptr<IBufferSet> Create(BufferType buffers_type, const Refs<IBuffer>& buffer_refs);

    [[nodiscard]] virtual BufferType           GetType() const noexcept = 0;
    [[nodiscard]] virtual Data::Size           GetCount() const noexcept = 0;
    [[nodiscard]] virtual const Refs<IBuffer>& GetRefs() const noexcept = 0;
    [[nodiscard]] virtual std::string          GetNames() const noexcept = 0;
    [[nodiscard]] virtual IBuffer&             operator[](Data::Index index) const = 0;
};

} // namespace Methane::Graphics::Rhi
