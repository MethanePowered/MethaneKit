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

FILE: Methane/Platform/Linux/Keyboard.cpp
Linux platform specific types and implementation of Keyboard abstractions.

******************************************************************************/

#include <Methane/Platform/Keyboard.h>
#include <Methane/Instrumentation.h>

#include <map>
#include <magic_enum.hpp>

#include <xcb/xcb.h>

namespace Methane::Platform::Keyboard
{

Key KeyConverter::GetKeyByNativeCode(const NativeKey& native_key)
{
    META_FUNCTION_TASK();
    static const std::map<NativeKey::Code, Key> s_key_by_native_code { };
    
    auto native_code_and_key_it = s_key_by_native_code.find(native_key.code);
    return native_code_and_key_it == s_key_by_native_code.end() ? Key::Unknown : native_code_and_key_it->second;
}

Modifiers KeyConverter::GetModifiersByNativeCode(const NativeKey& native_key)
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;

    Modifiers modifiers_mask = Modifiers::None;
    if (native_key.flags & XCB_MOD_MASK_SHIFT)
        modifiers_mask |= Keyboard::Modifiers::Shift;
    if (native_key.flags & XCB_MOD_MASK_CONTROL)
        modifiers_mask |= Keyboard::Modifiers::Control;
    if (native_key.flags & XCB_MOD_MASK_1)
        modifiers_mask |= Keyboard::Modifiers::Alt;
    if (native_key.flags & XCB_MOD_MASK_4)
        modifiers_mask |= Keyboard::Modifiers::Super;
    if (native_key.flags & XCB_MOD_MASK_2)
        modifiers_mask |= Keyboard::Modifiers::NumLock;
    if (native_key.flags & XCB_MOD_MASK_LOCK)
        modifiers_mask |= Keyboard::Modifiers::CapsLock;

    return modifiers_mask;
}

} // namespace Methane::Platform::Keyboard