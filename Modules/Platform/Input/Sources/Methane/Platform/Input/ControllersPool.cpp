/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Platform/Input/ControllersPool.cpp
A pool of input controllers for user actions handling in separate application components.

******************************************************************************/

#include <Methane/Platform/Input/ControllersPool.h>

#include <cassert>

// Uncomment define to print user input actions (keyboard, mouse) to debug output
//#define DEBUG_USER_INPUT

#ifdef DEBUG_USER_INPUT
#include <Methane/Platform/Utils.h>
#endif

using namespace Methane::Platform;
using namespace Methane::Platform::Input;

void ControllersPool::OnKeyboardStateChanged(const Keyboard::State& keyboard_state, const Keyboard::State& prev_keyboard_state, Keyboard::State::Property::Mask state_changes_hint)
{
#ifdef DEBUG_USER_INPUT
    PrintToDebugOutput(std::string("Keyboard: ") + keyboard_state.ToString());
#endif

    for (const Controller::Ptr& sp_controller : *this)
    {
        assert(!!sp_controller);
        if (!sp_controller || !sp_controller->IsEnabled())
            continue;
        
        sp_controller->OnKeyboardStateChanged(keyboard_state, prev_keyboard_state, state_changes_hint);
    }
}

void ControllersPool::OnMouseStateChanged(const Mouse::State& mouse_state, const Mouse::State& prev_mouse_state, Mouse::State::Property::Mask state_changes_hint)
{
#ifdef DEBUG_USER_INPUT
    PrintToDebugOutput(std::string(m_mouse_in_window ? "Mouse in window: " : "Mouse out of window: ") + mouse_state.ToString());
#endif

    for (const Controller::Ptr& sp_controller : *this)
    {
        assert(!!sp_controller);
        if (!sp_controller || !sp_controller->IsEnabled())
            continue;
        
        sp_controller->OnMouseStateChanged(mouse_state, prev_mouse_state, state_changes_hint);
    }
}
