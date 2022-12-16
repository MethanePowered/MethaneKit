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

FILE: Methane/Graphics/DirectX12/CommandListDebugGroup.hh
Metal command list debug group implementation.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/CommandListDebugGroup.h>

#import <Foundation/NSString.h>

namespace Methane::Graphics::Metal
{

class CommandListDebugGroup final
    : public Base::CommandListDebugGroup
{
public:
    explicit CommandListDebugGroup(std::string_view name);

    NSString* _Nonnull GetNSName() const noexcept { return m_ns_name; }

private:
    NSString* _Nonnull m_ns_name;
};

} // namespace Methane::Graphics::Metal
