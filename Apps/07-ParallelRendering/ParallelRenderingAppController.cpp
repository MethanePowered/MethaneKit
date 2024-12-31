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

FILE: ParallelRenderingAppController.cpp
Parallel Rendering application controller.

******************************************************************************/

#include "ParallelRenderingAppController.h"
#include "ParallelRenderingApp.h"

#include <Methane/Checks.hpp>

namespace Methane::Tutorials
{

ParallelRenderingAppController::ParallelRenderingAppController(ParallelRenderingApp& app, const ActionByKeyboardState& action_by_keyboard_state)
    : Controller("PARALLEL RENDERING SETTINGS")
    , pin::Keyboard::ActionControllerBase<ParallelRenderingAppAction>(action_by_keyboard_state, {})
    , m_app(app)
{
}

void ParallelRenderingAppController::OnKeyboardChanged(pin::Keyboard::Key key, pin::Keyboard::KeyState key_state,
                                               const pin::Keyboard::StateChange& state_change)
{
    pin::Keyboard::ActionControllerBase<ParallelRenderingAppAction>::OnKeyboardChanged(key, key_state, state_change);
}

void ParallelRenderingAppController::OnKeyboardStateAction(ParallelRenderingAppAction action)
{
    ParallelRenderingApp::Settings app_settings = m_app.GetSettings();

    switch(action)
    {
    case ParallelRenderingAppAction::SwitchParallelRendering:
        app_settings.parallel_rendering_enabled = !app_settings.parallel_rendering_enabled;
        break;

    case ParallelRenderingAppAction::IncreaseCubesGridSize:
        app_settings.cubes_grid_size++;
        break;

    case ParallelRenderingAppAction::DecreaseCubesGridSize:
        app_settings.cubes_grid_size = std::max(2U, app_settings.cubes_grid_size - 1U);
        break;

    case ParallelRenderingAppAction::IncreaseRenderThreadsCount:
        app_settings.render_thread_count++;
        break;

    case ParallelRenderingAppAction::DecreaseRenderThreadsCount:
        app_settings.render_thread_count = std::min(std::max(2U, app_settings.render_thread_count - 1U), app_settings.GetTotalCubesCount());
        break;

    default:
        META_UNEXPECTED(action);
    }

    m_app.SetSettings(app_settings);
}

std::string ParallelRenderingAppController::GetKeyboardActionName(ParallelRenderingAppAction action) const
{
    switch(action)
    {
    case ParallelRenderingAppAction::SwitchParallelRendering:    return "switch parallel rendering";
    case ParallelRenderingAppAction::IncreaseCubesGridSize:      return "increase cubes grid size";
    case ParallelRenderingAppAction::DecreaseCubesGridSize:      return "decrease cubes grid size";
    case ParallelRenderingAppAction::IncreaseRenderThreadsCount: return "increase render threads count";
    case ParallelRenderingAppAction::DecreaseRenderThreadsCount: return "decrease render threads count";
    default: META_UNEXPECTED_RETURN(action, "");
    }
}

pin::IHelpProvider::HelpLines ParallelRenderingAppController::GetHelp() const
{
    return GetKeyboardHelp();
}

} // namespace Methane::Graphics
