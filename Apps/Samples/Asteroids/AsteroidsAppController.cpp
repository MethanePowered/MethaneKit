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

FILE: AsteroidsAppController.cpp
Asteroids application controller.

******************************************************************************/

#include "AsteroidsAppController.h"
#include "AsteroidsApp.h"

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Samples
{

AsteroidsAppController::AsteroidsAppController(AsteroidsApp& asteroids_app, const ActionByKeyboardState& action_by_keyboard_state)
    : Controller("ASTEROIDS SETTINGS")
    , Platform::Keyboard::ActionControllerBase<AsteroidsAppAction>(action_by_keyboard_state, {})
    , m_asteroids_app(asteroids_app)
{
    META_FUNCTION_TASK();
}

void AsteroidsAppController::OnKeyboardChanged(Platform::Keyboard::Key key, Platform::Keyboard::KeyState key_state,
                                               const Platform::Keyboard::StateChange& state_change)
{
    META_FUNCTION_TASK();
    Platform::Keyboard::ActionControllerBase<AsteroidsAppAction>::OnKeyboardChanged(key, key_state, state_change);
}

void AsteroidsAppController::OnKeyboardStateAction(AsteroidsAppAction action)
{
    META_FUNCTION_TASK();
    
    const uint32_t asteroids_complexity = m_asteroids_app.GetAsteroidsComplexity();
    switch(action)
    {
    case AsteroidsAppAction::SwitchParallelRendering:
        m_asteroids_app.SetParallelRenderingEnabled(!m_asteroids_app.IsParallelRenderingEnabled());
        break;

    case AsteroidsAppAction::SwitchMeshLodsColoring:
        m_asteroids_app.GetAsteroidsArray().SetMeshLodColoringEnabled(!m_asteroids_app.GetAsteroidsArray().IsMeshLodColoringEnabled());
        break;

    case AsteroidsAppAction::IncreaseMeshLodComplexity:
        m_asteroids_app.GetAsteroidsArray().SetMinMeshLodScreenSize(m_asteroids_app.GetAsteroidsArray().GetMinMeshLodScreenSize() / 2.F);
        break;

    case AsteroidsAppAction::DecreaseMeshLodComplexity:
        m_asteroids_app.GetAsteroidsArray().SetMinMeshLodScreenSize(m_asteroids_app.GetAsteroidsArray().GetMinMeshLodScreenSize() * 2.F);
        break;

    case AsteroidsAppAction::IncreaseComplexity:
        m_asteroids_app.SetAsteroidsComplexity(asteroids_complexity + 1);
        break;

    case AsteroidsAppAction::DecreaseComplexity:
        m_asteroids_app.SetAsteroidsComplexity(asteroids_complexity > 1 ? asteroids_complexity - 1 : 0);
        break;

    case AsteroidsAppAction::SetComplexity0:
    case AsteroidsAppAction::SetComplexity1:
    case AsteroidsAppAction::SetComplexity2:
    case AsteroidsAppAction::SetComplexity3:
    case AsteroidsAppAction::SetComplexity4:
    case AsteroidsAppAction::SetComplexity5:
    case AsteroidsAppAction::SetComplexity6:
    case AsteroidsAppAction::SetComplexity7:
    case AsteroidsAppAction::SetComplexity8:
    case AsteroidsAppAction::SetComplexity9:
        m_asteroids_app.SetAsteroidsComplexity(static_cast<uint32_t>(action) - static_cast<uint32_t>(AsteroidsAppAction::SetComplexity0));
        break;
        
    default:
        META_UNEXPECTED_ARG(action);
    }
}

std::string AsteroidsAppController::GetKeyboardActionName(AsteroidsAppAction action) const
{
    META_FUNCTION_TASK();
    switch(action)
    {
    case AsteroidsAppAction::SwitchParallelRendering:   return "switch parallel rendering";
    case AsteroidsAppAction::SwitchMeshLodsColoring:    return "switch mesh LOD coloring";
    case AsteroidsAppAction::IncreaseMeshLodComplexity: return "increase mesh LOD complexity";
    case AsteroidsAppAction::DecreaseMeshLodComplexity: return "decrease mesh LOD complexity";
    case AsteroidsAppAction::IncreaseComplexity:        return "increase scene complexity";
    case AsteroidsAppAction::DecreaseComplexity:        return "decrease scene complexity";
    case AsteroidsAppAction::SetComplexity0:            return "set 0 scene complexity";
    case AsteroidsAppAction::SetComplexity1:            return "set 1 scene complexity";
    case AsteroidsAppAction::SetComplexity2:            return "set 2 scene complexity";
    case AsteroidsAppAction::SetComplexity3:            return "set 3 scene complexity";
    case AsteroidsAppAction::SetComplexity4:            return "set 4 scene complexity";
    case AsteroidsAppAction::SetComplexity5:            return "set 5 scene complexity";
    case AsteroidsAppAction::SetComplexity6:            return "set 6 scene complexity";
    case AsteroidsAppAction::SetComplexity7:            return "set 7 scene complexity";
    case AsteroidsAppAction::SetComplexity8:            return "set 8 scene complexity";
    case AsteroidsAppAction::SetComplexity9:            return "set 9 scene complexity";
    default:                                            META_UNEXPECTED_ARG_RETURN(action, "");
    }
}

Platform::Input::IHelpProvider::HelpLines AsteroidsAppController::GetHelp() const
{
    META_FUNCTION_TASK();
    return GetKeyboardHelp();
}

} // namespace Methane::Graphics
