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

FILE: Methane/Graphics/SkyBox.h
SkyBox rendering primitive

******************************************************************************/

#pragma once

#include "ImageLoader.h"
#include "MeshBuffers.hpp"

#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/RenderPass.h>
#include <Methane/Graphics/RHI/RenderState.h>
#include <Methane/Graphics/RHI/ViewState.h>
#include <Methane/Graphics/RHI/RenderContext.h>
#include <Methane/Graphics/RHI/RenderCommandList.h>
#include <Methane/Graphics/RHI/Texture.h>
#include <Methane/Graphics/RHI/Sampler.h>
#include <Methane/Graphics/Types.h>
#include <Methane/Data/EnumMask.hpp>

#include <hlsl++_matrix_float.h>

namespace hlslpp // NOSONAR
{
#pragma pack(push, 16)
#include <SkyBoxUniforms.h> // NOSONAR
#pragma pack(pop)
}

#include <memory>
#include <array>

namespace Methane::Graphics
{

class Camera;

class SkyBox
{
public:
    enum class Option : uint32_t
    {
        None            = 0U,
        DepthEnabled    = 1U << 0U,
        DepthReversed   = 1U << 1U,
        All             = ~0U,
    };

    using OptionMask = Data::EnumMask<Option>;

    struct Settings
    {
        const Camera& view_camera;
        float         scale = 1.F;
        OptionMask    render_options;
        float         lod_bias = 0.F;
    };

    struct META_UNIFORM_ALIGN Uniforms
    {
        hlslpp::float4x4 mvp_matrix;
    };

    SkyBox(const Rhi::CommandQueue& render_cmd_queue, const Rhi::RenderPattern& render_pattern, const Rhi::Texture& cube_map_texture, const Settings& settings);

    Rhi::ProgramBindings CreateProgramBindings(const Rhi::Buffer& uniforms_buffer_ptr, Data::Index frame_index) const;
    void Update();
    void Draw(const Rhi::RenderCommandList& cmd_list, const MeshBufferBindings& buffer_bindings, const Rhi::ViewState& view_state);

private:
    struct Vertex
    {
        Mesh::Position position;

        inline static const Mesh::VertexLayout layout{
            Mesh::VertexField::Position,
        };
    };

    SkyBox(const Rhi::CommandQueue& render_cmd_queue, const Rhi::RenderPattern& render_pattern, const Rhi::Texture& cube_map_texture,
           const Settings& settings, const BaseMesh<Vertex>& mesh);

    using TexMeshBuffers = TexturedMeshBuffers<hlslpp::SkyBoxUniforms>;

    Settings                m_settings;
    const Rhi::CommandQueue m_render_cmd_queue;
    Rhi::RenderContext      m_context;
    Rhi::Program            m_program;
    TexMeshBuffers          m_mesh_buffers;
    Rhi::Sampler            m_texture_sampler;
    Rhi::RenderState        m_render_state;
};

} // namespace Methane::Graphics
