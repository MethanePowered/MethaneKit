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

FILE: Methane/Platform/Linux/Keyboard.cpp
Linux platform specific types and implementation of Keyboard abstractions.

******************************************************************************/

#include <Methane/Platform/Keyboard.h>
#include <Methane/Instrumentation.h>

#include <xcb/xcb.h>
#include <X11/keysym.h>

namespace Methane::Platform::Keyboard
{

Key KeyConverter::GetKeyByNativeCode(const NativeKey& native_key) noexcept
{
    META_FUNCTION_TASK();
    switch (native_key.key) // NOSONAR - long switch
    {
    case XK_0:            return Key::Num0;
    case XK_1:            return Key::Num1;
    case XK_2:            return Key::Num2;
    case XK_3:            return Key::Num3;
    case XK_4:            return Key::Num4;
    case XK_5:            return Key::Num5;
    case XK_6:            return Key::Num6;
    case XK_7:            return Key::Num7;
    case XK_8:            return Key::Num8;
    case XK_9:            return Key::Num9;

    case XK_a:            return Key::A;
    case XK_b:            return Key::B;
    case XK_c:            return Key::C;
    case XK_d:            return Key::D;
    case XK_e:            return Key::E;
    case XK_f:            return Key::F;
    case XK_g:            return Key::G;
    case XK_h:            return Key::H;
    case XK_i:            return Key::I;
    case XK_j:            return Key::J;
    case XK_k:            return Key::K;
    case XK_l:            return Key::L;
    case XK_m:            return Key::M;
    case XK_n:            return Key::N;
    case XK_o:            return Key::O;
    case XK_p:            return Key::P;
    case XK_q:            return Key::Q;
    case XK_r:            return Key::R;
    case XK_s:            return Key::S;
    case XK_t:            return Key::T;
    case XK_u:            return Key::U;
    case XK_v:            return Key::V;
    case XK_w:            return Key::W;
    case XK_x:            return Key::X;
    case XK_y:            return Key::Y;
    case XK_z:            return Key::Z;

    case XK_apostrophe:   return Key::Apostrophe;
    case XK_backslash:    return Key::BackSlash;
    case XK_comma:        return Key::Comma;
    case XK_equal:        return Key::Equal;
    case XK_grave:        return Key::GraveAccent;
    case XK_semicolon:    return Key::Semicolon;
    case XK_slash:        return Key::Slash;
    case XK_minus:        return Key::Minus;
    case XK_period:       return Key::Period;
    case XK_bracketleft:  return Key::LeftBracket;
    case XK_bracketright: return Key::RightBracket;

    case XK_BackSpace:    return Key::Backspace;
    case XK_Delete:       return Key::Delete;
    case XK_Home:         return Key::Home;
    case XK_End:          return Key::End;
    case XK_Prior:        return Key::PageUp;
    case XK_Next:         return Key::PageDown;
    case XK_Return:       return Key::Enter;
    case XK_Escape:       return Key::Escape;

    case XK_Insert:       return Key::Insert;
    case XK_Menu:         return Key::Menu;
    case XK_KP_Page_Down: return Key::PageDown;
    case XK_KP_Page_Up:   return Key::PageUp;
    case XK_Pause:        return Key::Pause;
    case XK_space:        return Key::Space;
    case XK_Tab:          return Key::Tab;
    case XK_F1:           return Key::F1;
    case XK_F2:           return Key::F2;
    case XK_F3:           return Key::F3;
    case XK_F4:           return Key::F4;
    case XK_F5:           return Key::F5;
    case XK_F6:           return Key::F6;
    case XK_F7:           return Key::F7;
    case XK_F8:           return Key::F8;
    case XK_F9:           return Key::F9;
    case XK_F10:          return Key::F10;
    case XK_F11:          return Key::F11;
    case XK_F12:          return Key::F12;
    case XK_F13:          return Key::F13;
    case XK_F14:          return Key::F14;
    case XK_F15:          return Key::F15;

    case XK_Shift_L:      return Key::LeftShift;
    case XK_Shift_R:      return Key::RightShift;
    case XK_Control_L:    return Key::LeftControl;
    case XK_Control_R:    return Key::RightControl;
    case XK_Alt_L:        return Key::LeftAlt;
    case XK_Alt_R:        return Key::RightAlt;
    case XK_Super_L:      return Key::LeftSuper;
    case XK_Super_R:      return Key::RightSuper;

    case XK_Left:         return Key::Left;
    case XK_Right:        return Key::Right;
    case XK_Up:           return Key::Up;
    case XK_Down:         return Key::Down;

    case XK_KP_Enter:     return Key::KeyPadEnter;
    case XK_KP_Add:       return Key::KeyPadAdd;
    case XK_KP_Subtract:  return Key::KeyPadSubtract;
    case XK_KP_Multiply:  return Key::KeyPadMultiply;
    case XK_KP_Divide:    return Key::KeyPadDivide;
    case XK_KP_Insert:    return Key::Insert;
    case XK_KP_End:       return Key::End;
    case XK_KP_Down:      return Key::Down;
    case XK_KP_Left:      return Key::Left;
    case XK_KP_Right:     return Key::Right;
    case XK_KP_Home:      return Key::Home;
    case XK_KP_Up:        return Key::Up;
    case XK_KP_0:         return Key::KeyPad0;
    case XK_KP_1:         return Key::KeyPad1;
    case XK_KP_2:         return Key::KeyPad2;
    case XK_KP_3:         return Key::KeyPad3;
    case XK_KP_4:         return Key::KeyPad4;
    case XK_KP_5:         return Key::KeyPad5;
    case XK_KP_6:         return Key::KeyPad6;
    case XK_KP_7:         return Key::KeyPad7;
    case XK_KP_8:         return Key::KeyPad8;
    case XK_KP_9:         return Key::KeyPad9;

    default:              return Key::Unknown;
    }
}

ModifierMask KeyConverter::GetModifiersByNativeCode(const NativeKey& native_key) noexcept
{
    META_FUNCTION_TASK();
    ModifierMask modifiers_mask;
    if (native_key.flags & XCB_MOD_MASK_SHIFT)
        modifiers_mask |= Keyboard::Modifier::Shift;
    if (native_key.flags & XCB_MOD_MASK_CONTROL)
        modifiers_mask |= Keyboard::Modifier::Control;
    if (native_key.flags & XCB_MOD_MASK_1)
        modifiers_mask |= Keyboard::Modifier::Alt;
    if (native_key.flags & XCB_MOD_MASK_4)
        modifiers_mask |= Keyboard::Modifier::Super;
    if (native_key.flags & XCB_MOD_MASK_2)
        modifiers_mask |= Keyboard::Modifier::NumLock;
    if (native_key.flags & XCB_MOD_MASK_LOCK)
        modifiers_mask |= Keyboard::Modifier::CapsLock;

    return modifiers_mask;
}

} // namespace Methane::Platform::Keyboard