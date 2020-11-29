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

FILE: Methane/Platform/MacOS/Keyboard.mm
MacOS platform specific types and implementation of Keyboard abstractions.

******************************************************************************/

#include <Methane/Platform/Keyboard.h>
#include <Methane/Instrumentation.h>

#import <AppKit/AppKit.h>

#include <magic_enum.hpp>

namespace Methane::Platform::Keyboard
{

Modifiers KeyConverter::GetModifiersByNativeCode(const NativeKey& native_key)
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;

    Modifiers modifiers_mask = Modifiers::None;
    
    if (native_key.flags & NSEventModifierFlagShift)
        modifiers_mask |= Modifiers::Shift;
    
    if (native_key.flags & NSEventModifierFlagControl)
        modifiers_mask |= Modifiers::Control;
    
    if (native_key.flags & NSEventModifierFlagOption)
        modifiers_mask |= Modifiers::Alt;
    
    if (native_key.flags & NSEventModifierFlagCommand)
        modifiers_mask |= Modifiers::Super;
    
    if (native_key.flags & NSEventModifierFlagCapsLock)
        modifiers_mask |= Modifiers::CapsLock;
    
    if (native_key.flags & NSEventModifierFlagNumericPad)
        modifiers_mask |= Modifiers::NumLock;
    
    return modifiers_mask;
}

} // namespace Methane::Platform::Keyboard