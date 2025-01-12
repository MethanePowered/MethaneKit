/******************************************************************************

Copyright 2019-2024 Evgeny Gorodetskiy

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
#include <Methane/Graphics/Metal/Program.hh>
#include <Methane/Graphics/Metal/Shader.hh>
#include <Methane/Graphics/Metal/Buffer.hh>
#include <Methane/Graphics/Metal/Texture.hh>
#include <Methane/Graphics/Metal/Sampler.hh>

#include <magic_enum/magic_enum.hpp>

namespace Methane::Graphics::Metal
{

using NativeBuffers       = ProgramArgumentBinding::NativeBuffers;
using NativeTextures      = ProgramArgumentBinding::NativeTextures;
using NativeSamplerStates = ProgramArgumentBinding::NativeSamplerStates;
using NativeOffsets       = ProgramArgumentBinding::NativeOffsets;

class CallbackBlocker
{
public:
    CallbackBlocker(ProgramArgumentBinding& program_argument_binding)
        : m_program_argument_binding(program_argument_binding)
    {
        m_program_argument_binding.SetEmitCallbackEnabled(false);
    }

    ~CallbackBlocker()
    {
        m_program_argument_binding.SetEmitCallbackEnabled(true);
    }

private:
    ProgramArgumentBinding& m_program_argument_binding;
};

static MTLRenderStages ConvertShaderTypeToMetalRenderStages(Rhi::ShaderType shader_type)
{
    META_FUNCTION_TASK();
    MTLRenderStages mtl_render_stages{};
    switch(shader_type)
    {
        using enum Rhi::ShaderType;
        case All:    mtl_render_stages |= MTLRenderStageVertex
                                       |  MTLRenderStageFragment; break;
        case Vertex: mtl_render_stages |= MTLRenderStageVertex; break;
        case Pixel:  mtl_render_stages |= MTLRenderStageFragment; break;
        case Compute: /* Compute is not Render stage */ break;
        default: META_UNEXPECTED(shader_type);
    }
    return mtl_render_stages;
}

ProgramArgumentBinding::ProgramArgumentBinding(const Base::Context& context, const Settings& settings)
    : Base::ProgramArgumentBinding(context, settings)
    , m_settings_mt(settings)
    , m_mtl_render_stages(ConvertShaderTypeToMetalRenderStages(settings.argument.GetShaderType()))
{
}

Ptr<Base::ProgramArgumentBinding> ProgramArgumentBinding::CreateCopy() const
{
    META_FUNCTION_TASK();
    return std::make_shared<ProgramArgumentBinding>(*this);
}

void ProgramArgumentBinding::MergeSettings(const Base::ProgramArgumentBinding& other)
{
    META_FUNCTION_TASK();
    Base::ProgramArgumentBinding::MergeSettings(other);
    m_settings_mt.argument = Base::ProgramArgumentBinding::GetSettings().argument;

    const Settings& metal_settings = dynamic_cast<const ProgramArgumentBinding&>(other).GetMetalSettings();
    META_CHECK_EQUAL(m_settings_mt.argument_index, metal_settings.argument_index);
    for(const auto& [shader_type, struct_offset] : metal_settings.argument_buffer_offset_by_shader_type)
    {
        const auto argument_buffer_offset_it = m_settings_mt.argument_buffer_offset_by_shader_type.find(shader_type);
        if (argument_buffer_offset_it == m_settings_mt.argument_buffer_offset_by_shader_type.end())
            m_settings_mt.argument_buffer_offset_by_shader_type.emplace(shader_type, struct_offset);
        else if (!argument_buffer_offset_it->second)
            argument_buffer_offset_it->second = struct_offset;
    }
}

bool ProgramArgumentBinding::SetResourceViews(Rhi::ResourceViewSpan resource_views)
{
    META_FUNCTION_TASK();
    CallbackBlocker callback_blocker(*this);

    const Rhi::ResourceViews prev_resource_views = GetResourceViews();
    if (!Base::ProgramArgumentBinding::SetResourceViews(resource_views))
        return false;

    SetMetalResourcesForViews(resource_views);

    Data::Emitter<Rhi::IProgramBindings::IArgumentBindingCallback>::Emit(
        &Rhi::IProgramBindings::IArgumentBindingCallback::OnProgramArgumentBindingResourceViewsChanged,
        std::cref(*this), std::cref(prev_resource_views), std::cref(GetResourceViews())
    );
    return true;
}

void ProgramArgumentBinding::UpdateArgumentBufferOffsets(const Program& program)
{
    META_FUNCTION_TASK();
    if (m_settings_mt.argument_buffer_offset_by_shader_type.empty())
        return;

    Data::Size arg_buffer_offset = 0U;
    for(Rhi::ShaderType shader_type : program.GetShaderTypes())
    {
        if (arg_buffer_offset)
        {
            if (const auto argument_buffer_offset_it = m_settings_mt.argument_buffer_offset_by_shader_type.find(shader_type);
                argument_buffer_offset_it != m_settings_mt.argument_buffer_offset_by_shader_type.end())
                argument_buffer_offset_it->second += arg_buffer_offset;
        }

        const Rhi::ProgramArgumentAccessType arg_access_type = m_settings_mt.argument.GetAccessorType();
        if (const ArgumentBufferLayout* layout_ptr = program.GetMetalShader(shader_type).GetArgumentBufferLayoutPtr(arg_access_type))
        {
            arg_buffer_offset += layout_ptr->data_size;
        }
    }
}

bool ProgramArgumentBinding::UpdateRootConstantResourceViews()
{
    if (!Base::ProgramArgumentBinding::UpdateRootConstantResourceViews())
        return false;

    SetMetalResourcesForViews(Base::ProgramArgumentBinding::GetResourceViews());

    const Rhi::RootConstant root_constants = GetRootConstant();
    Data::Emitter<Rhi::IProgramBindings::IArgumentBindingCallback>::Emit(
        &Rhi::IProgramArgumentBindingCallback::OnProgramArgumentBindingRootConstantChanged,
        std::cref(*this), std::cref(root_constants)
    );
    return true;
}

void ProgramArgumentBinding::SetMetalResourcesForViews(Rhi::ResourceViewSpan resource_views)
{
    META_FUNCTION_TASK();

    m_mtl_resource_usage = {};
    m_mtl_resources.clear();
    m_mtl_sampler_states.clear();
    m_mtl_textures.clear();
    m_mtl_buffers.clear();
    m_mtl_buffer_offsets.clear();

    std::set<id<MTLResource>> mtl_resource_set;

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
        for(const Rhi::ResourceView& resource_view : resource_views)
        {
            const auto& texture = dynamic_cast<const Texture&>(resource_view.GetResource());
            m_mtl_resource_usage |= texture.GetNativeResourceUsage();
            mtl_resource_set.insert(static_cast<id<MTLResource>>(texture.GetNativeTexture()));
            m_mtl_textures.push_back(texture.GetNativeTexture());
        }
        break;

    case Rhi::ResourceType::Buffer:
        m_mtl_buffers.reserve(resource_views.size());
        m_mtl_buffer_offsets.reserve(resource_views.size());
        for (const Rhi::ResourceView& resource_view : resource_views)
        {
            const auto& buffer = dynamic_cast<const Buffer&>(resource_view.GetResource());
            m_mtl_resource_usage |= buffer.GetNativeResourceUsage();
            mtl_resource_set.insert(static_cast<id<MTLResource>>(buffer.GetNativeBuffer()));
            m_mtl_buffers.push_back(buffer.GetNativeBuffer());
            m_mtl_buffer_offsets.push_back(static_cast<NSUInteger>(resource_view.GetOffset()));
        }
        break;

    default: META_UNEXPECTED(m_settings_mt.resource_type);
    }

    std::copy(mtl_resource_set.begin(), mtl_resource_set.end(), std::back_inserter(m_mtl_resources));
}

} // namespace Methane::Graphics::Metal
