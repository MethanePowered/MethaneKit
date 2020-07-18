/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/UserInterface/Context.cpp
Methane user interface context used by all widgets for rendering.

******************************************************************************/

#include <Methane/UserInterface/Context.h>

#include <Methane/Graphics/RenderContext.h>

namespace Methane::UserInterface
{

Context::Context(gfx::RenderContext& render_context)
    : m_render_context(render_context)
{
}

float Context::GetDotsToPixelsFactor() const
{
    return m_render_context.GetContentScalingFactor();
}

uint32_t Context::GetFontResolutionDPI() const
{
    return m_render_context.GetFontResolutionDPI();
}

} // namespace Methane::UserInterface
