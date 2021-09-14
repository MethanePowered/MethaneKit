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

FILE: AsteroidsAppController.h
Asteroids application controller.

******************************************************************************/

#pragma once

#include <Methane/Platform/Input/Controller.h>
#include <Methane/Platform/KeyboardActionControllerBase.hpp>

namespace Methane::Samples
{

class AsteroidsApp;

enum class AsteroidsAppAction
{
    None,
    SwitchParallelRendering,
    SwitchMeshLodsColoring,
    IncreaseMeshLodComplexity,
    DecreaseMeshLodComplexity,
    IncreaseComplexity,
    DecreaseComplexity,
    SetComplexity0,
    SetComplexity1,
    SetComplexity2,
    SetComplexity3,
    SetComplexity4,
    SetComplexity5,
    SetComplexity6,
    SetComplexity7,
    SetComplexity8,
    SetComplexity9,
};

class AsteroidsAppController final
    : public Platform::Input::Controller
    , public Platform::Keyboard::ActionControllerBase<AsteroidsAppAction>
{
public:
    AsteroidsAppController(AsteroidsApp& asteroids_app, const ActionByKeyboardState& action_by_keyboard_state);

    // Input::Controller implementation
    void OnKeyboardChanged(Platform::Keyboard::Key, Platform::Keyboard::KeyState, const Platform::Keyboard::StateChange& state_change) override;
    HelpLines GetHelp() const override;
    
protected:
    // Keyboard::ActionControllerBase interface
    void        OnKeyboardKeyAction(AsteroidsAppAction, Platform::Keyboard::KeyState) override { /* not handled in this controller */ }
    void        OnKeyboardStateAction(AsteroidsAppAction action) override;
    std::string GetKeyboardActionName(AsteroidsAppAction action) const override;

private:
    AsteroidsApp& m_asteroids_app;
};

} // namespace Methane::Graphics
