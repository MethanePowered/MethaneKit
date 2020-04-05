/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Graphics/BufferBase.h
Base implementation of the buffer interface.

******************************************************************************/

#pragma once

#include "Native/ResourceNT.h"

#include <Methane/Graphics/Buffer.h>

namespace Methane::Graphics
{

class BufferBase
    : public Buffer
    , public ResourceNT
{
public:
    BufferBase(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage = DescriptorByUsage());

    // Resource interface
    Data::Size GetDataSize(Data::MemoryState size_type = Data::MemoryState::Reserved) const noexcept override;

    // Buffer interface
    const Settings& GetSettings() const noexcept override       { return m_settings; }
    uint32_t GetFormattedItemsCount() const noexcept override;

    Ptr<BufferBase> GetPtr();
    std::string GetBufferTypeName() const noexcept              { return Buffer::GetBufferTypeName(m_settings.type); }

private:
    Settings    m_settings;
};

} // namespace Methane::Graphics
