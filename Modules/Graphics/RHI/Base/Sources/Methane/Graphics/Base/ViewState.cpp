/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Base/ViewState.cpp
Base implementation of the view state interface.

******************************************************************************/

#include "Methane/Data/Emitter.hpp"
#include "Methane/Graphics/RHI/IViewState.h"
#include <Methane/Graphics/Base/ViewState.h>

#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics::Base
{

inline void Validate(const Viewports& viewports)
{
    META_CHECK_ARG_NOT_EMPTY_DESCR(viewports, "can not set empty viewports to state");
}

inline void Validate(const ScissorRects& scissor_rects)
{
    META_CHECK_ARG_NOT_EMPTY_DESCR(scissor_rects, "can not set empty scissor rectangles to state");
}

ViewState::ViewState(const Settings& settings)
    : m_settings(settings)
{
    META_FUNCTION_TASK();
    Validate(settings.viewports);
    Validate(settings.scissor_rects);
}

ViewState::~ViewState()
{
    META_FUNCTION_TASK();
    try
    {
        Data::Emitter<ICallback>::Emit(&ICallback::OnViewStateDestroyed, *this);
    }
    catch([[maybe_unused]] const std::exception& e)
    {
        META_LOG("WARNING: Unexpected error during view-state destruction: {}", e.what());
        assert(false);
    }
}

bool ViewState::Reset(const Settings& settings)
{
    META_FUNCTION_TASK();
    if (m_settings == settings)
        return false;

    Validate(settings.viewports);
    Validate(settings.scissor_rects);

    m_settings = settings;
    return true;
}

bool ViewState::SetViewports(const Viewports& viewports)
{
    META_FUNCTION_TASK();
    if (m_settings.viewports == viewports)
        return false;

    Validate(viewports);
    m_settings.viewports = viewports;
    return true;
}

bool ViewState::SetScissorRects(const ScissorRects& scissor_rects)
{
    META_FUNCTION_TASK();
    if (m_settings.scissor_rects == scissor_rects)
        return false;

    Validate(scissor_rects);
    m_settings.scissor_rects = scissor_rects;
    return true;
}

} // namespace Methane::Graphics::Base
