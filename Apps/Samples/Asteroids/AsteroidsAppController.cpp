/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: AsteroidsAppController.cpp
Asteroids application controller.

******************************************************************************/

#include "AsteroidsAppController.h"
#include "AsteroidsApp.h"

#include <Methane/Instrumentation.h>


namespace Methane::Samples
{

AsteroidsAppController::AsteroidsAppController(AsteroidsApp& asteroids_app, const ActionByKeyboardState& action_by_keyboard_state)
    : Controller("ASTEROIDS SETTINGS")
    , Platform::Keyboard::ActionControllerBase<AsteroidsAppAction>(action_by_keyboard_state, {})
    , m_asteroids_app(asteroids_app)
{
    ITT_FUNCTION_TASK();
}

void AsteroidsAppController::OnKeyboardChanged(Platform::Keyboard::Key key, Platform::Keyboard::KeyState key_state,
                                               const Platform::Keyboard::StateChange& state_change)
{
    ITT_FUNCTION_TASK();
    Platform::Keyboard::ActionControllerBase<AsteroidsAppAction>::OnKeyboardChanged(key, key_state, state_change);
}

void AsteroidsAppController::OnKeyboardStateAction(AsteroidsAppAction action)
{
    ITT_FUNCTION_TASK();
    
    const uint32_t asteroids_complexity = m_asteroids_app.GetAsteroidsComplexity();
    switch(action)
    {
    case AsteroidsAppAction::ShowParameters:
        m_asteroids_app.Alert({
            pal::AppBase::Message::Type::Information,
            "Methane Asteroids",
            m_asteroids_app.GetParametersString()
        });
        break;

    case AsteroidsAppAction::IncreaseComplexity:
        m_asteroids_app.SetAsteroidsComplexity(asteroids_complexity + 1);
        break;
        
    case AsteroidsAppAction::DecreaseComplexity:
        m_asteroids_app.SetAsteroidsComplexity(asteroids_complexity > 1 ? asteroids_complexity - 1 : 0);
        break;
        
    case AsteroidsAppAction::SwitchParallelRendering:
        m_asteroids_app.SetParallelRenderingEnabled(!m_asteroids_app.IsParallelRenderingEnabled());
        break;

    case AsteroidsAppAction::SwitchMeshLodsColoring:
        m_asteroids_app.GetAsteroidsArray().SetMeshLodColoringEnabled(!m_asteroids_app.GetAsteroidsArray().IsMeshLodColoringEnabled());
        break;

    case AsteroidsAppAction::IncreaseMeshLodComplexity:
        m_asteroids_app.GetAsteroidsArray().SetMinMeshLodScreenSize(m_asteroids_app.GetAsteroidsArray().GetMinMeshLodScreenSize() / 2.f);
        break;

    case AsteroidsAppAction::DecreaseMeshLodComplexity:
        m_asteroids_app.GetAsteroidsArray().SetMinMeshLodScreenSize(m_asteroids_app.GetAsteroidsArray().GetMinMeshLodScreenSize() * 2.f);
        break;
        
    default:
        assert(0);
    }
}

std::string AsteroidsAppController::GetKeyboardActionName(AsteroidsAppAction action) const
{
    ITT_FUNCTION_TASK();
    switch(action)
    {
    case AsteroidsAppAction::ShowParameters:            return "show simulation parameters";
    case AsteroidsAppAction::IncreaseComplexity:        return "increase scene complexity";
    case AsteroidsAppAction::DecreaseComplexity:        return "decrease scene complexity";
    case AsteroidsAppAction::SwitchParallelRendering:   return "switch parallel rendering";
    case AsteroidsAppAction::SwitchMeshLodsColoring:    return "switch mesh LOD coloring";
    case AsteroidsAppAction::IncreaseMeshLodComplexity: return "increase mesh LOD complexity";
    case AsteroidsAppAction::DecreaseMeshLodComplexity: return "decrease mesh LOD complexity";
    default: assert(0);
    }
    return "";
}

Platform::Input::IHelpProvider::HelpLines AsteroidsAppController::GetHelp() const
{
    ITT_FUNCTION_TASK();
    return GetKeyboardHelp();
}

} // namespace Methane::Graphics
