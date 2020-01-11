/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: AsteroidsArray.h
Random generated asteroids array with uber-mesh and textures ready for rendering.

******************************************************************************/

#pragma once

#include "Asteroid.h"

#include <Methane/Graphics/Context.h>
#include <Methane/Graphics/Mesh.h>
#include <Methane/Graphics/Sampler.h>
#include <Methane/Graphics/RenderState.h>
#include <Methane/Graphics/MeshBuffers.hpp>
#include <Methane/Graphics/Camera.h>

namespace Methane::Samples
{

namespace gfx = Graphics;

class AsteroidsArray final : protected gfx::TexturedMeshBuffers<AsteroidUniforms>
{
public:
    using Ptr = std::unique_ptr<AsteroidsArray>;
    using BaseBuffers = gfx::TexturedMeshBuffers<AsteroidUniforms>;

    struct Settings
    {
        gfx::Camera&    view_camera;
        float           scale                    = 1.f;
        uint32_t        instance_count           = 100u;
        uint32_t        unique_mesh_count        = 50u;
        uint32_t        minimum_subdivision      = 0u;
        uint32_t        subdivisions_count       = 3u;
        uint32_t        textures_count           = 10u;
        gfx::Dimensions texture_dimensions       = { 256u, 256u };
        uint32_t        random_seed              = 1337u;
        float           orbit_radius_ratio       = 10.f;
        float           disc_radius_ratio        = 3.f;
        float           min_asteroid_scale_ratio = 0.1f;
        float           max_asteroid_scale_ratio = 0.7f;
        bool            textures_array_enabled   = false;
        bool            depth_reversed           = false;
    };

    class UberMesh : public gfx::UberMesh<Asteroid::Vertex>
    {
    public:
        UberMesh(uint32_t instance_count, uint32_t subdivisions_count, uint32_t min_subdivision, uint32_t random_seed);

        uint32_t GetInstanceCount() const       { return m_instance_count; }
        uint32_t GetSubdivisionsCount() const   { return m_subdivisions_count; }
        uint32_t GetMinSubdivision() const      { return m_min_subdivision; }

        const gfx::Vector2f& GetSubsetDepthRange(uint32_t subset_index) const;
        uint32_t             GetSubsetSubdivision(uint32_t subset_index) const;

    private:
        const uint32_t             m_instance_count;
        const uint32_t             m_subdivisions_count;
        const uint32_t             m_min_subdivision;
        std::vector<gfx::Vector2f> m_depth_ranges;
    };

    using Parameters               = std::vector<Asteroid::Parameters>;
    using TextureArraySubresources = std::vector<gfx::Resource::SubResources>;

    struct ContentState : public std::enable_shared_from_this<ContentState>
    {
        using Ptr = std::shared_ptr<ContentState>;
        ContentState(const Settings& settings);

        using MeshSubsetTextureIndices = std::vector<uint32_t>;

        UberMesh                 uber_mesh;
        TextureArraySubresources texture_array_subresources;
        MeshSubsetTextureIndices mesh_subset_texture_indices;
        Parameters               parameters;
    };

    AsteroidsArray(gfx::Context& context, Settings settings);
    AsteroidsArray(gfx::Context& context, Settings settings, ContentState& state);

    const Settings& GetSettings() const         { return m_settings; }
    const ContentState::Ptr& GetState() const   { return m_sp_content_state; }
    Data::Size GetUniformsBufferSize() const    { return BaseBuffers::GetUniformsBufferSize(); }

    gfx::MeshBufferBindings::ResourceBindingsArray CreateResourceBindings(const gfx::Buffer::Ptr& sp_constants_buffer,
                                                                          const gfx::Buffer::Ptr& sp_scene_uniforms_buffer,
                                                                          const gfx::Buffer::Ptr& sp_asteroids_uniforms_buffer);

    void Resize(const gfx::FrameSize& frame_size);
    bool Update(double elapsed_seconds, double delta_seconds);
    void Draw(gfx::RenderCommandList& cmd_list, gfx::MeshBufferBindings& buffer_bindings);
    void DrawParallel(gfx::ParallelRenderCommandList& parallel_cmd_list, gfx::MeshBufferBindings& buffer_bindings);

protected:
    // MeshBuffers overrides
    uint32_t GetSubsetByInstanceIndex(uint32_t instance_index) const override;

private:
    const Settings        m_settings;
    ContentState::Ptr     m_sp_content_state;
    Textures              m_unique_textures;
    gfx::Sampler::Ptr     m_sp_texture_sampler;
    gfx::RenderState::Ptr m_sp_render_state;
};

} // namespace Methane::Samples
