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

FILE: Methane/Graphics/AppContextController.cpp
Graphics context controller for switching parameters in runtime.

******************************************************************************/

#include <Methane/Graphics/AppContextController.h>

#include <Methane/Graphics/Context.h>
#include <Methane/Graphics/Device.h>
#include <Methane/Platform/Utils.h>

#include <cassert>

using namespace Methane;
using namespace Methane::Graphics;
using namespace Methane::Platform;

AppContextController::AppContextController(Context& context, const ActionByKeyboardState& action_by_keyboard_state)
    : Controller("GRAPHICS SETTINGS")
    , Keyboard::ActionControllerBase<AppContextAction>(action_by_keyboard_state, {})
    , m_context(context)
{
}

void AppContextController::OnKeyboardChanged(Keyboard::Key key, Keyboard::KeyState key_state, const Keyboard::StateChange& state_change)
{
    Keyboard::ActionControllerBase<AppContextAction>::OnKeyboardChanged(key, key_state, state_change);
}

void AppContextController::OnKeyboardStateAction(AppContextAction action)
{
    switch (action)
    {
        case AppContextAction::SwitchVSync:
        {
            m_context.SetVSyncEnabled(!m_context.GetSettings().vsync_enabled);
        } break;

        case AppContextAction::SwitchDevice:
        {
            const Device::Ptr sp_next_device = System::Get().GetNextGpuDevice(m_context.GetDevice());
            if (sp_next_device)
            {
                m_context.Reset(*sp_next_device);
            }
        } break;

        case AppContextAction::AddFrameBufferToSwapChain:
        {
            m_context.SetFrameBuffersCount(m_context.GetSettings().frame_buffers_count + 1);
        } break;

        case AppContextAction::RemoveFrameBufferFromSwapChain:
        {
            m_context.SetFrameBuffersCount(m_context.GetSettings().frame_buffers_count - 1);
        } break;

        default: return;
    }
}

std::string AppContextController::GetKeyboardActionName(AppContextAction action) const
{
    switch (action)
    {
        case AppContextAction::None:                            return "none";
        case AppContextAction::SwitchVSync:                     return "switch vertical synchronization";
        case AppContextAction::SwitchDevice:                    return "switch device used for rendering";
        case AppContextAction::AddFrameBufferToSwapChain:       return "add frame buffer to swap-chain";
        case AppContextAction::RemoveFrameBufferFromSwapChain:  return "remove frame buffer from swap-chain";
        default: assert(0);                                     return "";
    }
}

Input::IHelpProvider::HelpLines AppContextController::GetHelp() const
{
    HelpLines help_lines = GetKeyboardHelp();

    // Add description of system graphics devices
    help_lines.push_back({ "\n" + System::Get().ToString(), "" });

    return help_lines;
}
