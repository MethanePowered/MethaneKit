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

namespace Methane::Graphics::Rhi
{

Ptr<ITexture> ITexture::Create(const IContext& context, const Settings& settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<Null::Texture>(dynamic_cast<const Base::Context&>(context), settings);
}

Ptr<ITexture> ITexture::CreateFrameBuffer(const IRenderContext& context, FrameBufferIndex frame_index)
{
    META_FUNCTION_TASK();
    const RenderContextSettings& context_settings = context.GetSettings();
    const Settings texture_settings = Settings::FrameBuffer(Dimensions(context_settings.frame_size), context_settings.color_format, frame_index);
    return std::make_shared<Null::Texture>(dynamic_cast<const Null::RenderContext&>(context), texture_settings, frame_index);
}

Ptr<ITexture> ITexture::CreateDepthStencil(const IRenderContext& context)
{
    META_FUNCTION_TASK();
    const RenderContextSettings& context_settings = context.GetSettings();
    const Settings texture_settings = Settings::DepthStencil(Dimensions(context_settings.frame_size), context_settings.depth_stencil_format, context_settings.clear_depth_stencil);
    return std::make_shared<Null::Texture>(dynamic_cast<const Base::Context&>(context), texture_settings);
}

Ptr<ITexture> ITexture::CreateImage(const IContext& context, const Dimensions& dimensions, const Opt<uint32_t>& array_length_opt, PixelFormat pixel_format, bool mipmapped)
{
    META_FUNCTION_TASK();
    const Settings texture_settings = Settings::Image(dimensions, array_length_opt, pixel_format, mipmapped, UsageMask({ Usage::ShaderRead }));
    return std::make_shared<Null::Texture>(dynamic_cast<const Base::Context&>(context), texture_settings);
}

Ptr<ITexture> ITexture::CreateCube(const IContext& context, uint32_t dimension_size, const Opt<uint32_t>& array_length_opt, PixelFormat pixel_format, bool mipmapped)
{
    META_FUNCTION_TASK();
    const Settings texture_settings = Settings::Cube(dimension_size, array_length_opt, pixel_format, mipmapped, UsageMask({ Usage::ShaderRead }));
    return std::make_shared<Null::Texture>(dynamic_cast<const Base::Context&>(context), texture_settings);
}

} // namespace Methane::Graphics::Rhi

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
