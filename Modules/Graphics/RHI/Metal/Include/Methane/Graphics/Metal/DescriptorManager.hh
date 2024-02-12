/******************************************************************************

Copyright 2024 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Metal/DescriptorManager.hh
Metal descriptor manager of the argument buffer

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/DescriptorManager.h>
#include <Methane/Data/RangeSet.hpp>

namespace Methane::Graphics::Metal
{

class Buffer;

class DescriptorManager final
    : public Base::DescriptorManager
{
public:
    explicit DescriptorManager(Base::Context& context);

    const Buffer* GetArgumentBuffer() const noexcept { return m_argument_buffer.get(); }

    // Rhi::IDescriptorManager
    void CompleteInitialization() override;
    void Release() override;

private:
    using ArgumentsRangeSet = Data::RangeSet<Data::Index>;
    using ArgumentsRange = Data::Range<Data::Index>;

    ArgumentsRangeSet m_free_argument_ranges;
    Ptr<Buffer>       m_argument_buffer;
};

} // namespace Methane::Graphics::Metal
