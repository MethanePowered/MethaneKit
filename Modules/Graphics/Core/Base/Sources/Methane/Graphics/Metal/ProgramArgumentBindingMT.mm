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

FILE: Methane/Graphics/DirectX12/ProgramArgumentBindingMT.mm
Metal implementation of the program argument binding interface.

******************************************************************************/

#include "ProgramArgumentBindingMT.hh"
#include "BufferMT.hh"
#include "TextureMT.hh"
#include "SamplerMT.hh"

namespace Methane::Graphics
{

using NativeBuffers       = ProgramArgumentBindingMT::NativeBuffers;
using NativeTextures      = ProgramArgumentBindingMT::NativeTextures;
using NativeSamplerStates = ProgramArgumentBindingMT::NativeSamplerStates;
using NativeOffsets       = ProgramArgumentBindingMT::NativeOffsets;

Ptr<ProgramArgumentBindingBase> ProgramArgumentBindingBase::CreateCopy(const ProgramArgumentBindingBase& other_argument_binding)
{
    META_FUNCTION_TASK();
    return std::make_shared<ProgramArgumentBindingMT>(static_cast<const ProgramArgumentBindingMT&>(other_argument_binding));
}

ProgramArgumentBindingMT::ProgramArgumentBindingMT(const ContextBase& context, const SettingsMT& settings)
    : ProgramArgumentBindingBase(context, settings)
    , m_settings_mt(settings)
{
    META_FUNCTION_TASK();
}

bool ProgramArgumentBindingMT::SetResourceViews(const IResource::Views& resource_views)
{
    META_FUNCTION_TASK();
    if (!ProgramArgumentBindingBase::SetResourceViews(resource_views))
        return false;

    m_mtl_sampler_states.clear();
    m_mtl_textures.clear();
    m_mtl_buffers.clear();
    m_mtl_buffer_offsets.clear();

    switch(m_settings_mt.resource_type)
    {
    case IResource::Type::Sampler:
        m_mtl_sampler_states.reserve(resource_views.size());
        std::transform(resource_views.begin(), resource_views.end(), std::back_inserter(m_mtl_sampler_states),
                       [](const IResource::View& resource_view)
                           { return dynamic_cast<const SamplerMT&>(resource_view.GetResource()).GetNativeSamplerState(); });
        break;

    case IResource::Type::Texture:
        m_mtl_textures.reserve(resource_views.size());
        std::transform(resource_views.begin(), resource_views.end(), std::back_inserter(m_mtl_textures),
                       [](const IResource::View& resource_view)
                           { return dynamic_cast<const TextureMT&>(resource_view.GetResource()).GetNativeTexture(); });
        break;

    case IResource::Type::Buffer:
        m_mtl_buffers.reserve(resource_views.size());
        m_mtl_buffer_offsets.reserve(resource_views.size());
        for (const IResource::View& resource_view : resource_views)
        {
            m_mtl_buffers.push_back(dynamic_cast<const BufferMT&>(resource_view.GetResource()).GetNativeBuffer());
            m_mtl_buffer_offsets.push_back(static_cast<NSUInteger>(resource_view.GetOffset()));
        }
        break;

    default: META_UNEXPECTED_ARG(m_settings_mt.resource_type);
    }
    return true;
}

} // namespace Methane::Graphics
