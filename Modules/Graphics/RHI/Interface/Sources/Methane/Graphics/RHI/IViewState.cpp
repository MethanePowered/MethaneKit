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

FILE: Methane/Graphics/RHI/IViewState.cpp
Methane view state interface: viewports and clipping rects setup.

******************************************************************************/

#include <Methane/Graphics/RHI/IViewState.h>
#include <Methane/Graphics/TypeFormatters.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace Methane::Graphics::Rhi
{

bool ViewSettings::operator==(const ViewSettings& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(viewports, scissor_rects) ==
           std::tie(other.viewports, other.scissor_rects);
}

bool ViewSettings::operator!=(const ViewSettings& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(viewports, scissor_rects) !=
           std::tie(other.viewports, other.scissor_rects);
}

bool ViewSettings::operator<(const ViewSettings& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(viewports, scissor_rects) <
           std::tie(other.viewports, other.scissor_rects);
}

ViewSettings::operator std::string() const
{
    META_FUNCTION_TASK();
    return fmt::format("  - Viewports: {};\n  - Scissor Rects: {}.", fmt::join(viewports, ", "), fmt::join(scissor_rects, ", "));
}

} // namespace Methane::Graphics::Rhi
