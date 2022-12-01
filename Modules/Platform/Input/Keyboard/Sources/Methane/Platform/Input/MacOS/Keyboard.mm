/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

#include <Methane/Platform/Input/Keyboard.h>

#include <Methane/Instrumentation.h>

#import <AppKit/NSEvent.h>

namespace Methane::Platform::Keyboard
{

Key KeyConverter::GetKeyByNativeCode(const NativeKey& native_key) noexcept
{
    META_FUNCTION_TASK();
    switch(native_key.code) // NOSONAR - long switch
    {
        case 0x1D: return Key::Num0;
        case 0x12: return Key::Num1;
        case 0x13: return Key::Num2;
        case 0x14: return Key::Num3;
        case 0x15: return Key::Num4;
        case 0x17: return Key::Num5;
        case 0x16: return Key::Num6;
        case 0x1A: return Key::Num7;
        case 0x1C: return Key::Num8;
        case 0x19: return Key::Num9;

        case 0x00: return Key::A;
        case 0x0B: return Key::B;
        case 0x08: return Key::C;
        case 0x02: return Key::D;
        case 0x0E: return Key::E;
        case 0x03: return Key::F;
        case 0x05: return Key::G;
        case 0x04: return Key::H;
        case 0x22: return Key::I;
        case 0x26: return Key::J;
        case 0x28: return Key::K;
        case 0x25: return Key::L;
        case 0x2E: return Key::M;
        case 0x2D: return Key::N;
        case 0x1F: return Key::O;
        case 0x23: return Key::P;
        case 0x0C: return Key::Q;
        case 0x0F: return Key::R;
        case 0x01: return Key::S;
        case 0x11: return Key::T;
        case 0x20: return Key::U;
        case 0x09: return Key::V;
        case 0x0D: return Key::W;
        case 0x07: return Key::X;
        case 0x10: return Key::Y;
        case 0x06: return Key::Z;

        case 0x27: return Key::Apostrophe;
        case 0x2A: return Key::BackSlash;
        case 0x2B: return Key::Comma;
        case 0x18: return Key::Equal;
        case 0x32: return Key::GraveAccent;
        case 0x21: return Key::LeftBracket;
        case 0x1B: return Key::Minus;
        case 0x2F: return Key::Period;
        case 0x1E: return Key::RightBracket;
        case 0x29: return Key::Semicolon;
        case 0x2C: return Key::Slash;
        case 0x0A: return Key::World1;

        case 0x7B: return Key::Left;
        case 0x7C: return Key::Right;
        case 0x7E: return Key::Up;
        case 0x7D: return Key::Down;

        case 0x73: return Key::Home;
        case 0x77: return Key::End;
        case 0x74: return Key::PageUp;
        case 0x79: return Key::PageDown;
        case 0x72: return Key::Insert;
        case 0x75: return Key::Delete;
        case 0x24: return Key::Enter;
        case 0x35: return Key::Escape;
        case 0x31: return Key::Space;
        case 0x30: return Key::Tab;

        case 0x7A: return Key::F1;
        case 0x78: return Key::F2;
        case 0x63: return Key::F3;
        case 0x76: return Key::F4;
        case 0x60: return Key::F5;
        case 0x61: return Key::F6;
        case 0x62: return Key::F7;
        case 0x64: return Key::F8;
        case 0x65: return Key::F9;
        case 0x6D: return Key::F10;
        case 0x67: return Key::F11;
        case 0x6F: return Key::F12;
        case 0x69: return Key::F13;
        case 0x6B: return Key::F14;
        case 0x71: return Key::F15;
        case 0x6A: return Key::F16;
        case 0x40: return Key::F17;
        case 0x4F: return Key::F18;
        case 0x50: return Key::F19;
        case 0x5A: return Key::F20;

        case 0x3A: return Key::LeftAlt;
        case 0x3B: return Key::LeftControl;
        case 0x38: return Key::LeftShift;
        case 0x37: return Key::LeftSuper;
        case 0x3D: return Key::RightAlt;
        case 0x3E: return Key::RightControl;
        case 0x3C: return Key::RightShift;
        case 0x36: return Key::RightSuper;
        case 0x6E: return Key::Menu;
        case 0x47: return Key::NumLock;

        case 0x52: return Key::KeyPad0;
        case 0x53: return Key::KeyPad1;
        case 0x54: return Key::KeyPad2;
        case 0x55: return Key::KeyPad3;
        case 0x56: return Key::KeyPad4;
        case 0x57: return Key::KeyPad5;
        case 0x58: return Key::KeyPad6;
        case 0x59: return Key::KeyPad7;
        case 0x5B: return Key::KeyPad8;
        case 0x5C: return Key::KeyPad9;
        case 0x45: return Key::KeyPadAdd;
        case 0x41: return Key::KeyPadDecimal;
        case 0x4B: return Key::KeyPadDivide;
        case 0x4C: return Key::KeyPadEnter;
        case 0x51: return Key::KeyPadEqual;
        case 0x43: return Key::KeyPadMultiply;
        case 0x4E: return Key::KeyPadSubtract;
        default:   return Key::Unknown;
    }
}

ModifierMask KeyConverter::GetModifiersByNativeCode(const NativeKey& native_key) noexcept
{
    META_FUNCTION_TASK();
    ModifierMask modifiers_mask;
    
    if (native_key.flags & NSEventModifierFlagShift)
        modifiers_mask.SetBitOn(Modifier::Shift);
    
    if (native_key.flags & NSEventModifierFlagControl)
        modifiers_mask.SetBitOn(Modifier::Control);
    
    if (native_key.flags & NSEventModifierFlagOption)
        modifiers_mask.SetBitOn(Modifier::Alt);
    
    if (native_key.flags & NSEventModifierFlagCommand)
        modifiers_mask.SetBitOn(Modifier::Super);
    
    if (native_key.flags & NSEventModifierFlagCapsLock)
        modifiers_mask.SetBitOn(Modifier::CapsLock);
    
    if (native_key.flags & NSEventModifierFlagNumericPad)
        modifiers_mask.SetBitOn(Modifier::NumLock);
    
    return modifiers_mask;
}

} // namespace Methane::Platform::Keyboard