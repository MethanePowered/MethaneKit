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

#include <Methane/Graphics/Types.h>

#include <hlsl++_matrix_float.h>

namespace hlslpp // NOSONAR
{
#pragma pack(push, 16)
#include <SkyBoxUniforms.h> // NOSONAR
#pragma pack(pop)
}

#include <memory>
#include <array>

namespace Methane::Graphics::Rhi
{

struct ICommandQueue;
struct IRenderContext;
struct IRenderState;
struct ISampler;

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics
{

class Camera;

class SkyBox
{
public:
    enum class Options : uint32_t
    {
        None            = 0U,
        DepthEnabled    = 1U << 0U,
        DepthReversed   = 1U << 1U,
        All             = ~0U,
    };

    struct Settings
    {
        const Camera& view_camera;
        float         scale;
        Options       render_options = Options::None;
        float         lod_bias       = 0.F;
    };

    struct META_UNIFORM_ALIGN Uniforms
    {
        hlslpp::float4x4 mvp_matrix;
    };

    SkyBox(Rhi::ICommandQueue& render_cmd_queue, Rhi::IRenderPattern& render_pattern, Rhi::ITexture& cube_map_texture, const Settings& settings);

    Ptr<Rhi::IProgramBindings> CreateProgramBindings(const Ptr<Rhi::IBuffer>& uniforms_buffer_ptr, Data::Index frame_index) const;
    void Update();
    void Draw(Rhi::IRenderCommandList& cmd_list, const MeshBufferBindings& buffer_bindings, Rhi::IViewState& view_state);

private:
    struct Vertex
    {
        Mesh::Position position;

        inline static const Mesh::VertexLayout layout{
            Mesh::VertexField::Position,
        };
    };

    SkyBox(Rhi::ICommandQueue& render_cmd_queue, Rhi::IRenderPattern& render_pattern, Rhi::ITexture& cube_map_texture,
           const Settings& settings, const BaseMesh<Vertex>& mesh);

    using TexMeshBuffers = TexturedMeshBuffers<hlslpp::SkyBoxUniforms>;

    Settings                      m_settings;
    const Ptr<Rhi::ICommandQueue> m_render_cmd_queue_ptr;
    Rhi::IRenderContext&          m_context;
    TexMeshBuffers                m_mesh_buffers;
    Ptr<Rhi::ISampler>            m_texture_sampler_ptr;
    Ptr<Rhi::IRenderState>        m_render_state_ptr;
};

} // namespace Methane::Graphics
