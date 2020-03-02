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

#include <Methane/Graphics/RenderContext.h>
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
    using BaseBuffers = gfx::TexturedMeshBuffers<AsteroidUniforms>;

    struct Settings
    {
        gfx::Camera&    view_camera;
        float           scale                    = 1.f;
        uint32_t        instance_count           = 100u;
        uint32_t        unique_mesh_count        = 50u;
        uint32_t        subdivisions_count       = 3u;
        uint32_t        textures_count           = 10u;
        gfx::Dimensions texture_dimensions       = { 256u, 256u };
        uint32_t        random_seed              = 1337u;
        float           orbit_radius_ratio       = 10.f;
        float           disc_radius_ratio        = 3.f;
        float           mesh_lod_min_screen_size = 0.06f;
        float           min_asteroid_scale_ratio = 0.1f;
        float           max_asteroid_scale_ratio = 0.7f;
        bool            textures_array_enabled   = false;
        bool            depth_reversed           = false;
    };

    class UberMesh : public gfx::UberMesh<Asteroid::Vertex>
    {
    public:
        UberMesh(uint32_t instance_count, uint32_t subdivisions_count, uint32_t random_seed);

        uint32_t GetInstanceCount() const       { return m_instance_count; }
        uint32_t GetSubdivisionsCount() const   { return m_subdivisions_count; }

        uint32_t             GetSubsetIndex(uint32_t instance_index, uint32_t subdivision_index);
        uint32_t             GetSubsetSubdivision(uint32_t subset_index) const;
        const gfx::Vector2f& GetSubsetDepthRange(uint32_t subset_index) const;

    private:
        const uint32_t             m_instance_count;
        const uint32_t             m_subdivisions_count;
        std::vector<gfx::Vector2f> m_depth_ranges;
    };

    using Parameters               = std::vector<Asteroid::Parameters>;
    using TextureArraySubresources = std::vector<gfx::Resource::SubResources>;

    struct ContentState : public std::enable_shared_from_this<ContentState>
    {
        ContentState(const Settings& settings);

        using MeshSubsetTextureIndices = std::vector<uint32_t>;

        UberMesh                 uber_mesh;
        TextureArraySubresources texture_array_subresources;
        MeshSubsetTextureIndices mesh_subset_texture_indices;
        Parameters               parameters;
    };

    AsteroidsArray(gfx::RenderContext& context, Settings settings);
    AsteroidsArray(gfx::RenderContext& context, Settings settings, ContentState& state);

    const Settings& GetSettings() const         { return m_settings; }
    const Ptr<ContentState>& GetState() const   { return m_sp_content_state; }
    using BaseBuffers::GetUniformsBufferSize;

    Ptrs<gfx::ProgramBindings> CreateProgramBindings(const Ptr<gfx::Buffer>& sp_constants_buffer,
                                                      const Ptr<gfx::Buffer>& sp_scene_uniforms_buffer,
                                                      const Ptr<gfx::Buffer>& sp_asteroids_uniforms_buffer);

    void Resize(const gfx::FrameSize& frame_size);
    bool Update(double elapsed_seconds, double delta_seconds);
    void Draw(gfx::RenderCommandList& cmd_list, gfx::MeshBufferBindings& buffer_bindings);
    void DrawParallel(gfx::ParallelRenderCommandList& parallel_cmd_list, gfx::MeshBufferBindings& buffer_bindings);

    bool  IsMeshLodColoringEnabled() const                           { return m_mesh_lod_coloring_enabled; }
    void  SetMeshLodColoringEnabled(bool mesh_lod_coloring_enabled)  { m_mesh_lod_coloring_enabled = mesh_lod_coloring_enabled; }

    float GetMinMeshLodScreenSize() const;
    void  SetMinMeshLodScreenSize(float mesh_lod_min_screen_size);

protected:
    // MeshBuffers overrides
    uint32_t GetSubsetByInstanceIndex(uint32_t instance_index) const override;

private:
    using MeshSubsetByInstanceIndex = std::vector<uint32_t>;

    const Settings            m_settings;
    Ptr<ContentState>         m_sp_content_state;
    Textures                  m_unique_textures;
    Ptr<gfx::Sampler>         m_sp_texture_sampler;
    Ptr<gfx::RenderState>     m_sp_render_state;
    MeshSubsetByInstanceIndex m_mesh_subset_by_instance_index;
    bool  m_mesh_lod_coloring_enabled = false;
    float m_min_mesh_lod_screen_size_log_2;
};

} // namespace Methane::Samples
