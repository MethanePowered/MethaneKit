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

FILE: Methane/Graphics/AppContextController.h
Graphics context controller for switching parameters in runtime.

******************************************************************************/

#pragma once

#include <Methane/Platform/Input/Controller.h>
#include <Methane/Platform/KeyboardActionControllerBase.hpp>

#include <map>

namespace Methane::Graphics::Rhi
{

struct IRenderContext;

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics
{

enum class AppContextAction
{
    None,
    SwitchVSync,
    SwitchDevice,
    AddFrameBufferToSwapChain,
    RemoveFrameBufferFromSwapChain
};

class AppContextController final
    : public Platform::Input::Controller
    , public Platform::Keyboard::ActionControllerBase<AppContextAction>
{
public:
    inline static const ActionByKeyboardState default_action_by_keyboard_state{
        { { Platform::Keyboard::Key::LeftControl, Platform::Keyboard::Key::V     }, AppContextAction::SwitchVSync                    },
        { { Platform::Keyboard::Key::LeftControl, Platform::Keyboard::Key::X     }, AppContextAction::SwitchDevice                   },
        { { Platform::Keyboard::Key::LeftControl, Platform::Keyboard::Key::Equal }, AppContextAction::AddFrameBufferToSwapChain      },
        { { Platform::Keyboard::Key::LeftControl, Platform::Keyboard::Key::Minus }, AppContextAction::RemoveFrameBufferFromSwapChain },
    };

    AppContextController(Rhi::IRenderContext& context, const ActionByKeyboardState& action_by_keyboard_state = default_action_by_keyboard_state);

    // Input::Controller implementation
    void OnKeyboardChanged(Platform::Keyboard::Key, Platform::Keyboard::KeyState, const Platform::Keyboard::StateChange& state_change) override;
    HelpLines GetHelp() const override;
    
protected:
    // Keyboard::ActionControllerBase interface
    void        OnKeyboardKeyAction(AppContextAction, Platform::Keyboard::KeyState) override { /* not handled in this controller */ }
    void        OnKeyboardStateAction(AppContextAction action) override;
    std::string GetKeyboardActionName(AppContextAction action) const override;

private:
    void ResetContextWithNextDevice();

    Rhi::IRenderContext& m_context;
};

} // namespace Methane::Graphics
