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

FILE: Methane/Graphics/Rect.hpp
2D Rect type based on cml::vector

******************************************************************************/

#pragma once

#include "Point.hpp"

#include <Methane/Data/Rect.hpp>

namespace Methane::Graphics
{

template<typename T, typename D>
using Rect = Data::Rect<T, D>;

using FrameRect    = Data::FrameRect;
using FrameSize    = Data::FrameSize;
using FramePoint   = Data::FramePoint;
using FloatRect    = Data::FloatRect;
using FloatSize    = Data::FloatSize;
using FloatPoint   = Data::FloatPoint;
using ScissorRect  = Rect<uint32_t, uint32_t>;
using ScissorRects = std::vector<ScissorRect>;

ScissorRect GetFrameScissorRect(const FrameRect& frame_rect, const FrameSize& render_attachment_size = FrameSize::Max());
ScissorRect GetFrameScissorRect(const FrameSize& frame_size);

} // namespace Methane::Graphics
