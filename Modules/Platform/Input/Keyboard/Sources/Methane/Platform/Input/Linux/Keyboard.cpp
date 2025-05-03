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

#include <Methane/Platform/Input/Keyboard.h>

#include <Methane/Instrumentation.h>

#include <xcb/xcb.h>
#include <X11/keysym.h>

namespace Methane::Platform::Input::Keyboard
{

Key KeyConverter::GetKeyByNativeCode(const NativeKey& native_key) noexcept
{
    META_FUNCTION_TASK();
    switch (native_key.key) // NOSONAR - long switch
    {
    using enum Key;
    case XK_0:            return Num0;
    case XK_1:            return Num1;
    case XK_2:            return Num2;
    case XK_3:            return Num3;
    case XK_4:            return Num4;
    case XK_5:            return Num5;
    case XK_6:            return Num6;
    case XK_7:            return Num7;
    case XK_8:            return Num8;
    case XK_9:            return Num9;

    case XK_a:            return A;
    case XK_b:            return B;
    case XK_c:            return C;
    case XK_d:            return D;
    case XK_e:            return E;
    case XK_f:            return F;
    case XK_g:            return G;
    case XK_h:            return H;
    case XK_i:            return I;
    case XK_j:            return J;
    case XK_k:            return K;
    case XK_l:            return L;
    case XK_m:            return M;
    case XK_n:            return N;
    case XK_o:            return O;
    case XK_p:            return P;
    case XK_q:            return Q;
    case XK_r:            return R;
    case XK_s:            return S;
    case XK_t:            return T;
    case XK_u:            return U;
    case XK_v:            return V;
    case XK_w:            return W;
    case XK_x:            return X;
    case XK_y:            return Y;
    case XK_z:            return Z;

    case XK_apostrophe:   return Apostrophe;
    case XK_backslash:    return BackSlash;
    case XK_comma:        return Comma;
    case XK_equal:        return Equal;
    case XK_grave:        return GraveAccent;
    case XK_semicolon:    return Semicolon;
    case XK_slash:        return Slash;
    case XK_minus:        return Minus;
    case XK_period:       return Period;
    case XK_bracketleft:  return LeftBracket;
    case XK_bracketright: return RightBracket;

    case XK_BackSpace:    return Backspace;
    case XK_Delete:       return Delete;
    case XK_Home:         return Home;
    case XK_End:          return End;
    case XK_Prior:        return PageUp;
    case XK_Next:         return PageDown;
    case XK_Return:       return Enter;
    case XK_Escape:       return Escape;

    case XK_Insert:       return Insert;
    case XK_Menu:         return Menu;
    case XK_KP_Page_Down: return PageDown;
    case XK_KP_Page_Up:   return PageUp;
    case XK_Pause:        return Pause;
    case XK_space:        return Space;
    case XK_Tab:          return Tab;
    case XK_F1:           return F1;
    case XK_F2:           return F2;
    case XK_F3:           return F3;
    case XK_F4:           return F4;
    case XK_F5:           return F5;
    case XK_F6:           return F6;
    case XK_F7:           return F7;
    case XK_F8:           return F8;
    case XK_F9:           return F9;
    case XK_F10:          return F10;
    case XK_F11:          return F11;
    case XK_F12:          return F12;
    case XK_F13:          return F13;
    case XK_F14:          return F14;
    case XK_F15:          return F15;

    case XK_Shift_L:      return LeftShift;
    case XK_Shift_R:      return RightShift;
    case XK_Control_L:    return LeftControl;
    case XK_Control_R:    return RightControl;
    case XK_Alt_L:        return LeftAlt;
    case XK_Alt_R:        return RightAlt;
    case XK_Super_L:      return LeftSuper;
    case XK_Super_R:      return RightSuper;

    case XK_Left:         return Left;
    case XK_Right:        return Right;
    case XK_Up:           return Up;
    case XK_Down:         return Down;

    case XK_KP_Enter:     return KeyPadEnter;
    case XK_KP_Add:       return KeyPadAdd;
    case XK_KP_Subtract:  return KeyPadSubtract;
    case XK_KP_Multiply:  return KeyPadMultiply;
    case XK_KP_Divide:    return KeyPadDivide;
    case XK_KP_Insert:    return Insert;
    case XK_KP_End:       return End;
    case XK_KP_Down:      return Down;
    case XK_KP_Left:      return Left;
    case XK_KP_Right:     return Right;
    case XK_KP_Home:      return Home;
    case XK_KP_Up:        return Up;
    case XK_KP_0:         return KeyPad0;
    case XK_KP_1:         return KeyPad1;
    case XK_KP_2:         return KeyPad2;
    case XK_KP_3:         return KeyPad3;
    case XK_KP_4:         return KeyPad4;
    case XK_KP_5:         return KeyPad5;
    case XK_KP_6:         return KeyPad6;
    case XK_KP_7:         return KeyPad7;
    case XK_KP_8:         return KeyPad8;
    case XK_KP_9:         return KeyPad9;

    default:              return Unknown;
    }
}

ModifierMask KeyConverter::GetModifiersByNativeCode(const NativeKey& native_key) noexcept
{
    META_FUNCTION_TASK();
    ModifierMask modifiers_mask;

    using enum Keyboard::Modifier;
    if (native_key.flags & XCB_MOD_MASK_SHIFT)
        modifiers_mask.SetBitOn(Shift);

    if (native_key.flags & XCB_MOD_MASK_CONTROL)
        modifiers_mask.SetBitOn(Control);

    if (native_key.flags & XCB_MOD_MASK_1)
        modifiers_mask.SetBitOn(Alt);

    if (native_key.flags & XCB_MOD_MASK_4)
        modifiers_mask.SetBitOn(Super);

    if (native_key.flags & XCB_MOD_MASK_2)
        modifiers_mask.SetBitOn(NumLock);

    if (native_key.flags & XCB_MOD_MASK_LOCK)
        modifiers_mask.SetBitOn(CapsLock);

    return modifiers_mask;
}

} // namespace Methane::Platform::Input::Keyboard