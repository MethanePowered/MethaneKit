/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Null/Texture.cpp
Null implementation of the texture interface.

******************************************************************************/

#include <Methane/Graphics/Null/Texture.h>
#include <Methane/Graphics/Null/RenderContext.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics::Null
{

Texture::Texture(const Base::Context& context, const Settings& settings)
    : Resource(context, settings)
{
}

Texture::Texture(const RenderContext& render_context, const Settings& settings, Data::Index frame_index)
    : Resource(render_context, settings)
{
    META_CHECK_ARG_TRUE(settings.frame_index_opt.has_value());
    META_CHECK_ARG_EQUAL(frame_index, settings.frame_index_opt.value());
}

} // namespace Methane::Graphics::Null
