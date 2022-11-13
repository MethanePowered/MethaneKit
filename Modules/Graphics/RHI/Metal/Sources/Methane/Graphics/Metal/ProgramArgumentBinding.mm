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

FILE: Methane/Graphics/DirectX12/ProgramArgumentBinding.mm
Metal implementation of the program argument binding interface.

******************************************************************************/

#include <Methane/Graphics/Metal/ProgramArgumentBinding.hh>
#include <Methane/Graphics/Metal/Buffer.hh>
#include <Methane/Graphics/Metal/Texture.hh>
#include <Methane/Graphics/Metal/Sampler.hh>

namespace Methane::Graphics::Base
{

Ptr<ProgramArgumentBinding> ProgramArgumentBinding::CreateCopy(const ProgramArgumentBinding& other_argument_binding)
{
    META_FUNCTION_TASK();
    return std::make_shared<Metal::ProgramArgumentBinding>(static_cast<const Metal::ProgramArgumentBinding&>(other_argument_binding));
}

} // namespace Methane::Graphics::Base

namespace Methane::Graphics::Metal
{

using NativeBuffers       = ProgramArgumentBinding::NativeBuffers;
using NativeTextures      = ProgramArgumentBinding::NativeTextures;
using NativeSamplerStates = ProgramArgumentBinding::NativeSamplerStates;
using NativeOffsets       = ProgramArgumentBinding::NativeOffsets;

ProgramArgumentBinding::ProgramArgumentBinding(const Base::Context& context, const Settings& settings)
    : Base::ProgramArgumentBinding(context, settings)
    , m_settings_mt(settings)
{
    META_FUNCTION_TASK();
}

bool ProgramArgumentBinding::SetResourceViews(const Rhi::ResourceViews& resource_views)
{
    META_FUNCTION_TASK();
    if (!Base::ProgramArgumentBinding::SetResourceViews(resource_views))
        return false;

    m_mtl_sampler_states.clear();
    m_mtl_textures.clear();
    m_mtl_buffers.clear();
    m_mtl_buffer_offsets.clear();

    switch(m_settings_mt.resource_type)
    {
    case Rhi::ResourceType::Sampler:
        m_mtl_sampler_states.reserve(resource_views.size());
        std::transform(resource_views.begin(), resource_views.end(), std::back_inserter(m_mtl_sampler_states),
                       [](const Rhi::ResourceView& resource_view)
                       { return dynamic_cast<const Sampler&>(resource_view.GetResource()).GetNativeSamplerState(); });
        break;

    case Rhi::ResourceType::Texture:
        m_mtl_textures.reserve(resource_views.size());
        std::transform(resource_views.begin(), resource_views.end(), std::back_inserter(m_mtl_textures),
                       [](const Rhi::ResourceView& resource_view)
                           { return dynamic_cast<const Texture&>(resource_view.GetResource()).GetNativeTexture(); });
        break;

    case Rhi::ResourceType::Buffer:
        m_mtl_buffers.reserve(resource_views.size());
        m_mtl_buffer_offsets.reserve(resource_views.size());
        for (const Rhi::ResourceView& resource_view : resource_views)
        {
            m_mtl_buffers.push_back(dynamic_cast<const Buffer&>(resource_view.GetResource()).GetNativeBuffer());
            m_mtl_buffer_offsets.push_back(static_cast<NSUInteger>(resource_view.GetOffset()));
        }
        break;

    default: META_UNEXPECTED_ARG(m_settings_mt.resource_type);
    }
    return true;
}

} // namespace Methane::Graphics::Metal
