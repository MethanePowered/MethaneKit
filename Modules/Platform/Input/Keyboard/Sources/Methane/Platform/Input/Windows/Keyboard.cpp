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
        using enum Key;
        case 0x00B: return Num0;
        case 0x002: return Num1;
        case 0x003: return Num2;
        case 0x004: return Num3;
        case 0x005: return Num4;
        case 0x006: return Num5;
        case 0x007: return Num6;
        case 0x008: return Num7;
        case 0x009: return Num8;
        case 0x00A: return Num9;

        case 0x01E: return A;
        case 0x030: return B;
        case 0x02E: return C;
        case 0x020: return D;
        case 0x012: return E;
        case 0x021: return F;
        case 0x022: return G;
        case 0x023: return H;
        case 0x017: return I;
        case 0x024: return J;
        case 0x025: return K;
        case 0x026: return L;
        case 0x032: return M;
        case 0x031: return N;
        case 0x018: return O;
        case 0x019: return P;
        case 0x010: return Q;
        case 0x013: return R;
        case 0x01F: return S;
        case 0x014: return T;
        case 0x016: return U;
        case 0x02F: return V;
        case 0x011: return W;
        case 0x02D: return X;
        case 0x015: return Y;
        case 0x02C: return Z;

        case 0x028: return Apostrophe;
        case 0x02B: return BackSlash;
        case 0x033: return Comma;
        case 0x00D: return Equal;
        case 0x029: return GraveAccent;
        case 0x027: return Semicolon;
        case 0x035: return Slash;
        case 0x00C: return Minus;
        case 0x034: return Period;
        case 0x01A: return LeftBracket;
        case 0x01B: return RightBracket;
        case 0x056: return World2;

        case 0x00E: return Backspace;
        case 0x153: return Delete;
        case 0x147: return Home;
        case 0x14F: return End;
        case 0x01C: return Enter;
        case 0x001: return Escape;

        case 0x152: return Insert;
        case 0x15D: return Menu;
        case 0x151: return PageDown;
        case 0x149: return PageUp;
        case 0x045: return Pause;
        case 0x146: return Pause;
        case 0x039: return Space;
        case 0x00F: return Tab;
        case 0x03A: return CapsLock;
        case 0x145: return NumLock;
        case 0x046: return ScrollLock;
        case 0x03B: return F1;
        case 0x03C: return F2;
        case 0x03D: return F3;
        case 0x03E: return F4;
        case 0x03F: return F5;
        case 0x040: return F6;
        case 0x041: return F7;
        case 0x042: return F8;
        case 0x043: return F9;
        case 0x044: return F10;
        case 0x057: return F11;
        case 0x058: return F12;
        case 0x064: return F13;
        case 0x065: return F14;
        case 0x066: return F15;
        case 0x067: return F16;
        case 0x068: return F17;
        case 0x069: return F18;
        case 0x06A: return F19;
        case 0x06B: return F20;
        case 0x06C: return F21;
        case 0x06D: return F22;
        case 0x06E: return F23;
        case 0x076: return F24;
        case 0x038: return LeftAlt;
        case 0x01D: return LeftControl;
        case 0x02A: return LeftShift;
        case 0x15B: return LeftSuper;
        case 0x137: return PrintScreen;
        case 0x138: return RightAlt;
        case 0x11D: return RightControl;
        case 0x036: return RightShift;
        case 0x15C: return RightSuper;
        case 0x150: return Down;
        case 0x14B: return Left;
        case 0x14D: return Right;
        case 0x148: return Up;

        case 0x052: return KeyPad0;
        case 0x04F: return KeyPad1;
        case 0x050: return KeyPad2;
        case 0x051: return KeyPad3;
        case 0x04B: return KeyPad4;
        case 0x04C: return KeyPad5;
        case 0x04D: return KeyPad6;
        case 0x047: return KeyPad7;
        case 0x048: return KeyPad8;
        case 0x049: return KeyPad9;
        case 0x04E: return KeyPadAdd;
        case 0x053: return KeyPadDecimal;
        case 0x135: return KeyPadDivide;
        case 0x11C: return KeyPadEnter;
        case 0x059: return KeyPadEqual;
        case 0x037: return KeyPadMultiply;
        case 0x04A: return KeyPadSubtract;
        default:    return Unknown;
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
    using enum Modifier;
    case VK_CONTROL: return ModifierMask(Control);
    case VK_SHIFT:   return ModifierMask(Shift);
    case VK_CAPITAL: return ModifierMask(CapsLock);
    default:         return ModifierMask();
    }
}

} // namespace Methane::Platform::Input::Keyboard
