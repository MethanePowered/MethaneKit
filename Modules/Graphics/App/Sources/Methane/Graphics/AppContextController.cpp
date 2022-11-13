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

FILE: Methane/Graphics/AppContextController.cpp
Graphics context controller for switching parameters in runtime.

******************************************************************************/

#include <Methane/Graphics/AppContextController.h>

#include <Methane/Graphics/RHI/IRenderContext.h>
#include <Methane/Graphics/RHI/IDevice.h>
#include <Methane/Platform/Utils.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics
{

AppContextController::AppContextController(Rhi::IRenderContext& context, const ActionByKeyboardState& action_by_keyboard_state)
    : Controller("GRAPHICS SETTINGS")
    , Platform::Keyboard::ActionControllerBase<AppContextAction>(action_by_keyboard_state, {})
    , m_context(context)
{
    META_FUNCTION_TASK();
}

void AppContextController::OnKeyboardChanged(Platform::Keyboard::Key key, Platform::Keyboard::KeyState key_state,
                                             const Platform::Keyboard::StateChange& state_change)
{
    META_FUNCTION_TASK();
    Platform::Keyboard::ActionControllerBase<AppContextAction>::OnKeyboardChanged(key, key_state, state_change);
}

void AppContextController::OnKeyboardStateAction(AppContextAction action)
{
    META_FUNCTION_TASK();
    switch (action)
    {
    case AppContextAction::SwitchVSync:
        m_context.SetVSyncEnabled(!m_context.GetSettings().vsync_enabled);
        break;

    case AppContextAction::AddFrameBufferToSwapChain:
        m_context.SetFrameBuffersCount(m_context.GetSettings().frame_buffers_count + 1);
        break;

    case AppContextAction::RemoveFrameBufferFromSwapChain:
        m_context.SetFrameBuffersCount(m_context.GetSettings().frame_buffers_count - 1);
        break;

    case AppContextAction::SwitchDevice:
        ResetContextWithNextDevice();
        break;

    default: META_UNEXPECTED_ARG(action);
    }
}

std::string AppContextController::GetKeyboardActionName(AppContextAction action) const
{
    META_FUNCTION_TASK();
    switch (action)
    {
        case AppContextAction::None:                            return "none";
        case AppContextAction::SwitchVSync:                     return "switch vertical synchronization";
        case AppContextAction::SwitchDevice:                    return "switch device used for rendering";
        case AppContextAction::AddFrameBufferToSwapChain:       return "add frame buffer to swap-chain";
        case AppContextAction::RemoveFrameBufferFromSwapChain:  return "remove frame buffer from swap-chain";
        default:                                                META_UNEXPECTED_ARG_RETURN(action, "");
    }
}

Platform::Input::IHelpProvider::HelpLines AppContextController::GetHelp() const
{
    META_FUNCTION_TASK();
    return GetKeyboardHelp();
}

void AppContextController::ResetContextWithNextDevice()
{
    META_FUNCTION_TASK();
    const Ptr<Rhi::IDevice> next_device_ptr = Rhi::ISystem::Get().GetNextGpuDevice(m_context.GetDevice());
    if (!next_device_ptr)
        return;

    m_context.Reset(*next_device_ptr);
}

} // namespace Methane::Graphics