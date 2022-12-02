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

FILE: Methane/Platform/Windows/Keyboard.h
Windows platform specific types and implementation of Keyboard abstractions.

******************************************************************************/

#include <Methane/Platform/Input/Keyboard.h>

#include <Methane/Instrumentation.h>

namespace Methane::Platform::Input::Keyboard
{

Key KeyConverter::GetKeyByNativeCode(const NativeKey& native_key) noexcept
{
    META_FUNCTION_TASK();
    // The Ctrl keys require special handling
    if (native_key.w_param == VK_CONTROL)
        return GetControlKey(native_key);

    if (native_key.w_param == VK_PROCESSKEY)
        return Key::Unknown; // IME notifies that keys have been filtered by setting the virtual key-code to VK_PROCESSKEY

    switch(const auto native_key_code = static_cast<uint32_t>(HIWORD(native_key.l_param) & 0x1FF); // NOSONAR - long switch
           native_key_code)
    {
        case 0x00B: return Key::Num0;
        case 0x002: return Key::Num1;
        case 0x003: return Key::Num2;
        case 0x004: return Key::Num3;
        case 0x005: return Key::Num4;
        case 0x006: return Key::Num5;
        case 0x007: return Key::Num6;
        case 0x008: return Key::Num7;
        case 0x009: return Key::Num8;
        case 0x00A: return Key::Num9;

        case 0x01E: return Key::A;
        case 0x030: return Key::B;
        case 0x02E: return Key::C;
        case 0x020: return Key::D;
        case 0x012: return Key::E;
        case 0x021: return Key::F;
        case 0x022: return Key::G;
        case 0x023: return Key::H;
        case 0x017: return Key::I;
        case 0x024: return Key::J;
        case 0x025: return Key::K;
        case 0x026: return Key::L;
        case 0x032: return Key::M;
        case 0x031: return Key::N;
        case 0x018: return Key::O;
        case 0x019: return Key::P;
        case 0x010: return Key::Q;
        case 0x013: return Key::R;
        case 0x01F: return Key::S;
        case 0x014: return Key::T;
        case 0x016: return Key::U;
        case 0x02F: return Key::V;
        case 0x011: return Key::W;
        case 0x02D: return Key::X;
        case 0x015: return Key::Y;
        case 0x02C: return Key::Z;

        case 0x028: return Key::Apostrophe;
        case 0x02B: return Key::BackSlash;
        case 0x033: return Key::Comma;
        case 0x00D: return Key::Equal;
        case 0x029: return Key::GraveAccent;
        case 0x027: return Key::Semicolon;
        case 0x035: return Key::Slash;
        case 0x00C: return Key::Minus;
        case 0x034: return Key::Period;
        case 0x01A: return Key::LeftBracket;
        case 0x01B: return Key::RightBracket;
        case 0x056: return Key::World2;

        case 0x00E: return Key::Backspace;
        case 0x153: return Key::Delete;
        case 0x147: return Key::Home;
        case 0x14F: return Key::End;
        case 0x01C: return Key::Enter;
        case 0x001: return Key::Escape;

        case 0x152: return Key::Insert;
        case 0x15D: return Key::Menu;
        case 0x151: return Key::PageDown;
        case 0x149: return Key::PageUp;
        case 0x045: return Key::Pause;
        case 0x146: return Key::Pause;
        case 0x039: return Key::Space;
        case 0x00F: return Key::Tab;
        case 0x03A: return Key::CapsLock;
        case 0x145: return Key::NumLock;
        case 0x046: return Key::ScrollLock;
        case 0x03B: return Key::F1;
        case 0x03C: return Key::F2;
        case 0x03D: return Key::F3;
        case 0x03E: return Key::F4;
        case 0x03F: return Key::F5;
        case 0x040: return Key::F6;
        case 0x041: return Key::F7;
        case 0x042: return Key::F8;
        case 0x043: return Key::F9;
        case 0x044: return Key::F10;
        case 0x057: return Key::F11;
        case 0x058: return Key::F12;
        case 0x064: return Key::F13;
        case 0x065: return Key::F14;
        case 0x066: return Key::F15;
        case 0x067: return Key::F16;
        case 0x068: return Key::F17;
        case 0x069: return Key::F18;
        case 0x06A: return Key::F19;
        case 0x06B: return Key::F20;
        case 0x06C: return Key::F21;
        case 0x06D: return Key::F22;
        case 0x06E: return Key::F23;
        case 0x076: return Key::F24;
        case 0x038: return Key::LeftAlt;
        case 0x01D: return Key::LeftControl;
        case 0x02A: return Key::LeftShift;
        case 0x15B: return Key::LeftSuper;
        case 0x137: return Key::PrintScreen;
        case 0x138: return Key::RightAlt;
        case 0x11D: return Key::RightControl;
        case 0x036: return Key::RightShift;
        case 0x15C: return Key::RightSuper;
        case 0x150: return Key::Down;
        case 0x14B: return Key::Left;
        case 0x14D: return Key::Right;
        case 0x148: return Key::Up;

        case 0x052: return Key::KeyPad0;
        case 0x04F: return Key::KeyPad1;
        case 0x050: return Key::KeyPad2;
        case 0x051: return Key::KeyPad3;
        case 0x04B: return Key::KeyPad4;
        case 0x04C: return Key::KeyPad5;
        case 0x04D: return Key::KeyPad6;
        case 0x047: return Key::KeyPad7;
        case 0x048: return Key::KeyPad8;
        case 0x049: return Key::KeyPad9;
        case 0x04E: return Key::KeyPadAdd;
        case 0x053: return Key::KeyPadDecimal;
        case 0x135: return Key::KeyPadDivide;
        case 0x11C: return Key::KeyPadEnter;
        case 0x059: return Key::KeyPadEqual;
        case 0x037: return Key::KeyPadMultiply;
        case 0x04A: return Key::KeyPadSubtract;
        default:    return Key::Unknown;
    }
}

Key KeyConverter::GetControlKey(const NativeKey& native_key)
{
    MSG  next {};
    LONG time = 0;

    // Right side keys have the extended key bit set
    if (native_key.l_param & 0x01000000)
        return Key::Unknown;

    // HACK: Alt Gr sends Left Ctrl and then Right Alt in close sequence
    //       We only want the Right Alt message, so if the next message is
    //       Right Alt we ignore this (synthetic) Left Ctrl message
    time = GetMessageTime();

    if (!PeekMessageW(&next, nullptr, 0, 0, PM_NOREMOVE))
        return Key::LeftControl;

    if ((next.message == WM_KEYDOWN || next.message == WM_SYSKEYDOWN || next.message == WM_KEYUP || next.message == WM_SYSKEYUP) &&
        next.wParam == VK_MENU && (next.lParam & 0x01000000) && next.time == static_cast<DWORD>(time))
        return Key::Unknown; // Next message is Right Alt down so discard this

    return Key::LeftControl;
}

ModifierMask KeyConverter::GetModifiersByNativeCode(const NativeKey& native_key) noexcept
{
    META_FUNCTION_TASK();
    switch (native_key.w_param)
    {
    case VK_CONTROL: return ModifierMask(Modifier::Control);
    case VK_SHIFT:   return ModifierMask(Modifier::Shift);
    case VK_CAPITAL: return ModifierMask(Modifier::CapsLock);
    default:         return ModifierMask();
    }
}

} // namespace Methane::Platform::Input::Keyboard
