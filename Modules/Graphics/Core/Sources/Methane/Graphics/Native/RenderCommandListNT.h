/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Native/RenderCommandListNT.h
Native implementation alias of the render command list interface.

******************************************************************************/

#pragma once

#if defined _WIN32

#include <Methane/Graphics/DirectX12/RenderCommandListDX.h>

#elif defined __APPLE__

#include <Methane/Graphics/Metal/RenderCommandListMT.hh>

#endif

namespace Methane
{
namespace Graphics
{

#if defined _WIN32

using RenderCommandListNT = RenderCommandListDX;

#elif defined __APPLE__

using RenderCommandListNT = RenderCommandListMT;

#endif

} // namespace Graphics
} // namespace Methane
