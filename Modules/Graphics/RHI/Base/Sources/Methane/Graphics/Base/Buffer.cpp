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

FILE: Methane/Graphics/Base/Buffer.cpp
Base implementation of the buffer interface.

******************************************************************************/

#include <Methane/Graphics/Base/Buffer.h>
#include <Methane/Graphics/Base/Context.h>

#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Base
{

Buffer::Buffer(const Context& context, const Settings& settings,
                       State initial_state, Opt<State> auto_transition_source_state_opt)
    : Resource(context, Rhi::IResource::Type::Buffer, settings.usage_mask, initial_state, auto_transition_source_state_opt)
    , m_settings(settings)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_ZERO_DESCR(settings.size, "can not create buffer of zero size");

    SetSubResourceCount(SubResource::Count());
}

Data::Size Buffer::GetDataSize(Data::MemoryState size_type) const noexcept
{
    META_FUNCTION_TASK();
    return size_type == Data::MemoryState::Reserved ? m_settings.size : GetInitializedDataSize();
}

uint32_t Buffer::GetFormattedItemsCount() const noexcept
{
    META_FUNCTION_TASK();
    return m_settings.item_stride_size > 0U ? GetDataSize(Data::MemoryState::Initialized) / m_settings.item_stride_size : 0U;
}

} // namespace Methane::Graphics::Base