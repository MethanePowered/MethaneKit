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

FILE: Methane/Graphics/DirectX/CommandListDebugGroup.h
DirectX 12 command list debug group implementation.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/CommandListDebugGroup.h>

#include <string>

namespace Methane::Graphics::DirectX
{

class CommandListDebugGroup final
    : public Base::CommandListDebugGroup
{
public:
    explicit CommandListDebugGroup(std::string_view name);

    const std::wstring& GetWideName() const noexcept;

private:
    const std::wstring m_wide_name;
};

} // namespace Methane::Graphics::DirectX
