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

void ControllersPool::OnKeyboardChanged(Keyboard::Key key, Keyboard::KeyState key_state, const Keyboard::StateChange& state_change)
{
#ifdef DEBUG_USER_INPUT
    PrintToDebugOutput(std::string("Keyboard: ") + state_change.current.ToString());
#endif

    for (const Controller::Ptr& sp_controller : *this)
    {
        assert(!!sp_controller);
        if (!sp_controller || !sp_controller->IsEnabled())
            continue;
        
        sp_controller->OnKeyboardChanged(key, key_state, state_change);
    }
}

void ControllersPool::OnMouseButtonChanged(Mouse::Button button, Mouse::ButtonState button_state, const Mouse::StateChange& state_change)
{
#ifdef DEBUG_USER_INPUT
    PrintToDebugOutput(std::string("Mouse (button): ") + state_change.current.ToString());
#endif

    for (const Controller::Ptr& sp_controller : *this)
    {
        assert(!!sp_controller);
        if (!sp_controller || !sp_controller->IsEnabled())
            continue;
        
        sp_controller->OnMouseButtonChanged(button, button_state, state_change);
    }
}

void ControllersPool::OnMousePositionChanged(const Mouse::Position& mouse_position, const Mouse::StateChange& state_change)
{
#ifdef DEBUG_USER_INPUT
    PrintToDebugOutput(std::string("Mouse (position): ") + state_change.current.ToString());
#endif

    for (const Controller::Ptr& sp_controller : *this)
    {
        assert(!!sp_controller);
        if (!sp_controller || !sp_controller->IsEnabled())
            continue;

        sp_controller->OnMousePositionChanged(mouse_position, state_change);
    }
}

void ControllersPool::OnMouseScrollChanged(const Mouse::Scroll& mouse_scroll_delta, const Mouse::StateChange& state_change)
{
#ifdef DEBUG_USER_INPUT
    PrintToDebugOutput(std::string("Mouse scroll delta: ") + std::to_string(mouse_scroll_delta.x()) + " x " + std::to_string(mouse_scroll_delta.y()));
    PrintToDebugOutput(std::string("Mouse (scroll): ") + state_change.current.ToString());
#endif

    for (const Controller::Ptr& sp_controller : *this)
    {
        assert(!!sp_controller);
        if (!sp_controller || !sp_controller->IsEnabled())
            continue;

        sp_controller->OnMouseScrollChanged(mouse_scroll_delta, state_change);
    }
}

void ControllersPool::OnMouseInWindowChanged(bool is_mouse_in_window, const Mouse::StateChange& state_change)
{
#ifdef DEBUG_USER_INPUT
    PrintToDebugOutput(std::string("Mouse (in-window): ") + state_change.current.ToString());
#endif

    for (const Controller::Ptr& sp_controller : *this)
    {
        assert(!!sp_controller);
        if (!sp_controller || !sp_controller->IsEnabled())
            continue;

        sp_controller->OnMouseInWindowChanged(is_mouse_in_window, state_change);
    }
}
