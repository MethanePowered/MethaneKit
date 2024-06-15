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

FILE: Methane/Graphics/SkyBox.cpp
SkyBox rendering primitive

******************************************************************************/

#include <Methane/Graphics/SkyBox.h>
#include <Methane/Graphics/MeshBuffers.hpp>
#include <Methane/Graphics/CubeMesh.hpp>
#include <Methane/Graphics/Camera.h>
#include <Methane/Graphics/Types.h>

#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/RenderState.h>
#include <Methane/Graphics/RHI/ViewState.h>
#include <Methane/Graphics/RHI/RenderContext.h>
#include <Methane/Graphics/RHI/RenderCommandList.h>
#include <Methane/Graphics/RHI/CommandListDebugGroup.h>
#include <Methane/Graphics/RHI/Program.h>
#include <Methane/Graphics/RHI/Buffer.h>
#include <Methane/Graphics/RHI/Texture.h>
#include <Methane/Graphics/RHI/Sampler.h>

#include <Methane/Data/EnumMask.hpp>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>
#include <Methane/Pimpl.hpp>

#include <hlsl++_matrix_float.h>

namespace hlslpp // NOSONAR
{
#pragma pack(push, 16)
#include <SkyBoxUniforms.h> // NOSONAR
#pragma pack(pop)
}

namespace Methane::Graphics
{

struct META_UNIFORM_ALIGN SkyBoxUniforms
{
    hlslpp::float4x4 mvp_matrix;
};

struct SkyBoxVertex
{
    Mesh::Position position;

    inline static const Mesh::VertexLayout layout{
        Mesh::VertexField::Position,
    };
};

class SkyBox::Impl
{
private:
    using Uniforms       = SkyBoxUniforms;
    using Vertex         = SkyBoxVertex;
    using TexMeshBuffers = TexturedMeshBuffers<hlslpp::SkyBoxUniforms>;

    Settings                m_settings;
    const Rhi::CommandQueue m_render_cmd_queue;
    Rhi::RenderContext      m_context;
    Rhi::Program            m_program;
    TexMeshBuffers          m_mesh_buffers;
    Rhi::Sampler            m_texture_sampler;
    Rhi::RenderState        m_render_state;

    Impl(const Rhi::CommandQueue& render_cmd_queue,
         const Rhi::RenderPattern& render_pattern,
         const Rhi::Texture& cube_map_texture,
         const Settings& settings,
         const BaseMesh<Vertex>& mesh)
        : m_settings(settings)
        , m_render_cmd_queue(render_cmd_queue)
        , m_context(render_pattern.GetRenderContext())
        , m_mesh_buffers(render_cmd_queue, mesh, "Sky-Box")
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_EQUAL(cube_map_texture.GetSettings().dimension_type, Rhi::TextureDimensionType::Cube);
        m_mesh_buffers.SetTexture(cube_map_texture);

        m_program = m_context.CreateProgram(
            Rhi::Program::Settings
            {
                Rhi::Program::ShaderSet
                {
                    { Rhi::ShaderType::Vertex, { Data::ShaderProvider::Get(), { "SkyBox", "SkyboxVS" }, { } } },
                    { Rhi::ShaderType::Pixel,  { Data::ShaderProvider::Get(), { "SkyBox", "SkyboxPS" }, { } } },
                },
                Rhi::ProgramInputBufferLayouts
                {
                    Rhi::Program::InputBufferLayout
                    {
                        Rhi::Program::InputBufferLayout::ArgumentSemantics { mesh.GetVertexLayout().GetSemantics() }
                    }
                },
                Rhi::ProgramArgumentAccessors{ },
                render_pattern.GetAttachmentFormats()
            });
        m_program.SetName("Sky-box shading");

        Rhi::RenderState::Settings state_settings{ m_program, render_pattern };
        state_settings.depth.enabled        = m_settings.render_options.HasAnyBit(Option::DepthEnabled);
        state_settings.depth.write_enabled  = false;
        state_settings.depth.compare        = m_settings.render_options.HasAnyBit(Option::DepthReversed) ? Compare::GreaterEqual : Compare::Less;
        state_settings.rasterizer.is_front_counter_clockwise = true;

        m_render_state = m_context.CreateRenderState( state_settings);
        m_render_state.SetName("Sky-box render state");

        m_texture_sampler = m_context.CreateSampler({
            Rhi::ISampler::Filter(Rhi::ISampler::Filter::MinMag::Linear),
            Rhi::ISampler::Address(Rhi::ISampler::Address::Mode::ClampToZero),
            Rhi::ISampler::LevelOfDetail(m_settings.lod_bias)
        });
        m_texture_sampler.SetName("Sky-box Texture Sampler");
    }

public:
    Impl(const Rhi::CommandQueue& render_cmd_queue,
         const Rhi::RenderPattern& render_pattern,
         const Rhi::Texture& cube_map_texture,
         const Settings& settings)
        : Impl(render_cmd_queue, render_pattern, cube_map_texture, settings, CubeMesh<Vertex>(Vertex::layout))
    {
    }

    Rhi::ProgramBindings CreateProgramBindings(const Rhi::Buffer& uniforms_buffer, Data::Index frame_index) const
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_TRUE(uniforms_buffer.IsInitialized());
        return Rhi::ProgramBindings(m_program, {
            { { Rhi::ShaderType::Vertex, "g_skybox_uniforms" }, uniforms_buffer.GetResourceView() },
            { { Rhi::ShaderType::Pixel,  "g_skybox_texture"  }, m_mesh_buffers.GetTexture().GetResourceView() },
            { { Rhi::ShaderType::Pixel,  "g_texture_sampler" }, m_texture_sampler.GetResourceView() },
        }, frame_index);
    }

    void Update()
    {
        META_FUNCTION_TASK();
        m_mesh_buffers.SetFinalPassUniforms({
            hlslpp::transpose(hlslpp::mul(
                hlslpp::mul(
                    hlslpp::float4x4::scale(m_settings.scale),
                    hlslpp::float4x4::translation(m_settings.view_camera.GetOrientation().eye)),
                m_settings.view_camera.GetViewProjMatrix()
            ))
        });
    }

    void Draw(const Rhi::RenderCommandList& render_cmd_list, const MeshBufferBindings& buffer_bindings, const Rhi::ViewState& view_state) const
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_TRUE(buffer_bindings.program_bindings.IsInitialized());
        META_CHECK_ARG_TRUE(buffer_bindings.uniforms_buffer.IsInitialized());
        META_CHECK_ARG_GREATER_OR_EQUAL(buffer_bindings.uniforms_buffer.GetDataSize(), sizeof(Uniforms));

        buffer_bindings.uniforms_buffer.SetData(m_render_cmd_queue, m_mesh_buffers.GetFinalPassUniformsSubresource());

        META_DEBUG_GROUP_VAR(s_debug_group, "Sky-box rendering");
        render_cmd_list.ResetWithStateOnce(m_render_state, &s_debug_group);
        render_cmd_list.SetViewState(view_state);
        m_mesh_buffers.Draw(render_cmd_list, buffer_bindings.program_bindings);
    }
};

Data::Size SkyBox::GetUniformsSize()
{
    return static_cast<Data::Size>(sizeof(SkyBoxUniforms));
}

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(SkyBox);

SkyBox::SkyBox(const Rhi::CommandQueue& render_cmd_queue,
               const Rhi::RenderPattern& render_pattern,
               const Rhi::Texture& cube_map_texture,
               const Settings& settings)
   : m_impl_ptr(std::make_shared<Impl>(render_cmd_queue, render_pattern, cube_map_texture, settings))
{
}

Rhi::ProgramBindings SkyBox::CreateProgramBindings(const Rhi::Buffer& uniforms_buffer, Data::Index frame_index) const
{
    return GetImpl(m_impl_ptr).CreateProgramBindings(uniforms_buffer, frame_index);
}

void SkyBox::Update() const
{
    GetImpl(m_impl_ptr).Update();
}

void SkyBox::Draw(const Rhi::RenderCommandList& render_cmd_list, const MeshBufferBindings& buffer_bindings, const Rhi::ViewState& view_state) const
{
    GetImpl(m_impl_ptr).Draw(render_cmd_list, buffer_bindings, view_state);
}

} // namespace Methane::Graphics
