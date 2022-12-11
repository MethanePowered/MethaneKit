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

FILE: Methane/Graphics/DirectX/ICommandList.cpp
DirectX 12 command list interface and debug group implementation.

******************************************************************************/

#include <Methane/Graphics/DirectX/ICommandList.h>

#include <Methane/Instrumentation.h>

#include <nowide/convert.hpp>

namespace Methane::Graphics::Rhi
{

Ptr<ICommandListDebugGroup> ICommandListDebugGroup::Create(std::string_view name)
{
    META_FUNCTION_TASK();
    return std::make_shared<DirectX::CommandListDebugGroup>(name);
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::DirectX
{

CommandListDebugGroup::CommandListDebugGroup(std::string_view name)
    : Base::CommandListDebugGroup(name)
    , m_wide_name(nowide::widen(Base::Object::GetName()))
{
    META_FUNCTION_TASK();
}

const std::wstring& CommandListDebugGroup::GetWideName() const noexcept
{
    return m_wide_name;
}

} // namespace Methane::Graphics::DirectX
