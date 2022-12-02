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

FILE: Methane/Platform/Input/ControllersPool.cpp
A pool of input controllers for user actions handling in separate application components.

******************************************************************************/

#include <Methane/Platform/Input/ControllersPool.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Platform::Input
{

void ControllersPool::OnMouseButtonChanged(Mouse::Button button, Input::Mouse::ButtonState button_state, const Input::Mouse::StateChange& state_change)
{
    META_FUNCTION_TASK();
    META_FUNCTION_THREAD_MARKER();
    ITT_MARKER_ARG("Mouse-State", state_change.current.ToString());
    META_LOG("Mouse button: {}", state_change.current.ToString());

    for (const Ptr<Controller>& controller_ptr : *this)
    {
        META_CHECK_ARG_NOT_NULL(controller_ptr);
        if (!controller_ptr || !controller_ptr->IsEnabled())
            continue;
        
        controller_ptr->OnMouseButtonChanged(button, button_state, state_change);
    }
}

void ControllersPool::OnMousePositionChanged(const Input::Mouse::Position& mouse_position, const Input::Mouse::StateChange& state_change)
{
    META_FUNCTION_TASK();
    META_FUNCTION_THREAD_MARKER();
    ITT_MARKER_ARG("Mouse-State", state_change.current.ToString());
    META_LOG("Mouse position: {}", state_change.current.ToString());

    for (const Ptr<Controller>& controller_ptr : *this)
    {
        META_CHECK_ARG_NOT_NULL(controller_ptr);
        if (!controller_ptr || !controller_ptr->IsEnabled())
            continue;

        controller_ptr->OnMousePositionChanged(mouse_position, state_change);
    }
}

void ControllersPool::OnMouseScrollChanged(const Input::Mouse::Scroll& mouse_scroll_delta, const Input::Mouse::StateChange& state_change)
{
    META_FUNCTION_TASK();
    META_FUNCTION_THREAD_MARKER();
    ITT_MARKER_ARG("Mouse-State", state_change.current.ToString());
    META_LOG("Mouse scroll: {}, scroll delta: ({} x {})", state_change.current.ToString(), mouse_scroll_delta.GetX(), mouse_scroll_delta.GetY());

    for (const Ptr<Controller>& controller_ptr : *this)
    {
        META_CHECK_ARG_NOT_NULL(controller_ptr);
        if (!controller_ptr || !controller_ptr->IsEnabled())
            continue;

        controller_ptr->OnMouseScrollChanged(mouse_scroll_delta, state_change);
    }
}

void ControllersPool::OnMouseInWindowChanged(bool is_mouse_in_window, const Input::Mouse::StateChange& state_change)
{
    META_FUNCTION_TASK();
    META_FUNCTION_THREAD_MARKER();
    ITT_MARKER_ARG("Mouse-State", state_change.current.ToString());
    META_LOG("Mouse in-window: {}", state_change.current.ToString());

    for (const Ptr<Controller>& controller_ptr : *this)
    {
        META_CHECK_ARG_NOT_NULL(controller_ptr);
        if (!controller_ptr || !controller_ptr->IsEnabled())
            continue;

        controller_ptr->OnMouseInWindowChanged(is_mouse_in_window, state_change);
    }
}

void ControllersPool::OnKeyboardChanged(Keyboard::Key key, Input::Keyboard::KeyState key_state, const Input::Keyboard::StateChange& state_change)
{
    META_FUNCTION_TASK();
    META_FUNCTION_THREAD_MARKER();
    ITT_MARKER_ARG("Keyboard-State", state_change.current.ToString());
    META_LOG("Keyboard key: {}", state_change.current.ToString());

    for (const Ptr<Controller>& controller_ptr : *this)
    {
        META_CHECK_ARG_NOT_NULL(controller_ptr);
        if (!controller_ptr || !controller_ptr->IsEnabled())
            continue;
        
        controller_ptr->OnKeyboardChanged(key, key_state, state_change);
    }
}

void ControllersPool::OnModifiersChanged(Keyboard::ModifierMask modifiers, const Input::Keyboard::StateChange& state_change)
{
    META_FUNCTION_TASK();
    META_FUNCTION_THREAD_MARKER();
    ITT_MARKER_ARG("Keyboard-State", state_change.current.ToString());
    META_LOG("Keyboard modifiers: {}", state_change.current.ToString());

    for (const Ptr<Controller>& controller_ptr : *this)
    {
        META_CHECK_ARG_NOT_NULL(controller_ptr);
        if (!controller_ptr || !controller_ptr->IsEnabled())
            continue;
        
        controller_ptr->OnModifiersChanged(modifiers, state_change);
    }
}

IHelpProvider::HelpLines ControllersPool::GetHelp() const
{
    META_FUNCTION_TASK();

    HelpLines all_help_lines;
    for (const Ptr<Controller>& controller_ptr : *this)
    {
        META_CHECK_ARG_NOT_NULL(controller_ptr);
        if (!controller_ptr || !controller_ptr->IsEnabled())
            continue;

        const HelpLines help_lines = controller_ptr->GetHelp();

        all_help_lines.emplace_back("", controller_ptr->GetControllerName());
        all_help_lines.insert(all_help_lines.end(), help_lines.begin(), help_lines.end());
    }
    return all_help_lines;
}

} // namespace Methane::Platform::Input
