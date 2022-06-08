/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: ParallelRenderingAppController.h
Parallel Rendering application controller.

******************************************************************************/

#pragma once

#include <Methane/Platform/Input/Controller.h>
#include <Methane/Platform/KeyboardActionControllerBase.hpp>

namespace Methane::Tutorials
{

class ParallelRenderingApp;

enum class ParallelRenderingAppAction
{
    None,
    SwitchParallelRendering,
    IncreaseCubesGridSize,
    DecreaseCubesGridSize,
    IncreaseRenderThreadsCount,
    DecreaseRenderThreadsCount,
};

class ParallelRenderingAppController final
    : public Platform::Input::Controller
    , public Platform::Keyboard::ActionControllerBase<ParallelRenderingAppAction>
{
public:
    ParallelRenderingAppController(ParallelRenderingApp& app, const ActionByKeyboardState& action_by_keyboard_state);

    // Input::Controller implementation
    void OnKeyboardChanged(Platform::Keyboard::Key, Platform::Keyboard::KeyState, const Platform::Keyboard::StateChange& state_change) override;
    HelpLines GetHelp() const override;
    
protected:
    // Keyboard::ActionControllerBase interface
    void        OnKeyboardKeyAction(ParallelRenderingAppAction, Platform::Keyboard::KeyState) override { /* not handled in this controller */ }
    void        OnKeyboardStateAction(ParallelRenderingAppAction action) override;
    std::string GetKeyboardActionName(ParallelRenderingAppAction action) const override;

private:
    ParallelRenderingApp& m_app;
};

} // namespace Methane::Tutorials
