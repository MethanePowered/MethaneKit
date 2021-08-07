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

FILE: AsteroidsArray.h
Random generated asteroids array with uber-mesh and textures ready for rendering.

******************************************************************************/

#pragma once

#include "Asteroid.h"

#include <Methane/Graphics/Mesh.h>
#include <Methane/Graphics/Sampler.h>
#include <Methane/Graphics/RenderPass.h>
#include <Methane/Graphics/RenderState.h>
#include <Methane/Graphics/MeshBuffers.hpp>
#include <Methane/Graphics/Camera.h>

namespace hlslpp // NOSONAR
{
#include "Shaders/AsteroidUniforms.h"
}

#include <taskflow/taskflow.hpp>

namespace Methane::Samples
{

namespace gfx = Graphics;

class AsteroidsArray final : public gfx::TexturedMeshBuffers<hlslpp::AsteroidUniforms>
{
public:
    using BaseBuffers = gfx::TexturedMeshBuffers<hlslpp::AsteroidUniforms>;

    struct Settings
    {
        gfx::Camera&    view_camera;
        float           scale                    = 1.F;
        uint32_t        instance_count           = 100U;
        uint32_t        unique_mesh_count        = 50U;
        uint32_t        subdivisions_count       = 3U;
        uint32_t        textures_count           = 10U;
        gfx::Dimensions texture_dimensions       { 256U, 256U };
        uint32_t        random_seed              = 1337U;
        float           orbit_radius_ratio       = 10.F;
        float           disc_radius_ratio        = 3.F;
        float           mesh_lod_min_screen_size = 0.06F;
        float           min_asteroid_scale_ratio = 0.1F;
        float           max_asteroid_scale_ratio = 0.7F;
        bool            textures_array_enabled   = false;
        bool            depth_reversed           = false;
    };

    class UberMesh : public gfx::UberMesh<Asteroid::Vertex>
    {
    public:
        UberMesh(tf::Executor& parallel_executor, uint32_t instance_count, uint32_t subdivisions_count, uint32_t random_seed);

        [[nodiscard]] uint32_t GetInstanceCount() const noexcept      { return m_instance_count; }
        [[nodiscard]] uint32_t GetSubdivisionsCount() const noexcept  { return m_subdivisions_count; }

        [[nodiscard]] uint32_t GetSubsetIndex(uint32_t instance_index, uint32_t subdivision_index) const;
        [[nodiscard]] uint32_t GetSubsetSubdivision(uint32_t subset_index) const;
        [[nodiscard]] const Asteroid::Mesh::DepthRange& GetSubsetDepthRange(uint32_t subset_index) const;

    private:
        using DepthRanges = std::vector<Asteroid::Mesh::DepthRange>;

        const uint32_t m_instance_count;
        const uint32_t m_subdivisions_count;
        DepthRanges    m_depth_ranges;
    };

    using Parameters               = std::vector<Asteroid::Parameters>;
    using TextureArraySubresources = std::vector<gfx::Resource::SubResources>;

    struct ContentState : public std::enable_shared_from_this<ContentState>
    {
        ContentState(tf::Executor& parallel_executor, const Settings& settings);

        using MeshSubsetTextureIndices = std::vector<uint32_t>;

        UberMesh                 uber_mesh;
        TextureArraySubresources texture_array_subresources;
        MeshSubsetTextureIndices mesh_subset_texture_indices;
        Parameters               parameters;
    };

    AsteroidsArray(gfx::RenderPattern& render_pattern, const Settings& settings);
    AsteroidsArray(gfx::RenderPattern& render_pattern, const Settings& settings, ContentState& state);

    [[nodiscard]] const Settings& GetSettings() const         { return m_settings; }
    [[nodiscard]] const Ptr<ContentState>& GetState() const   { return m_content_state_ptr; }
    using BaseBuffers::GetUniformsBufferSize;

    Ptrs<gfx::ProgramBindings> CreateProgramBindings(const Ptr<gfx::Buffer>& constants_buffer_ptr,
                                                     const Ptr<gfx::Buffer>& scene_uniforms_buffer_ptr,
                                                     const Ptr<gfx::Buffer>& asteroids_uniforms_buffer_ptr,
                                                     Data::Index frame_index) const;

    Ptr<gfx::Resource::Barriers> CreateBeginningResourceBarriers(gfx::Buffer& constants_buffer);

    bool Update(double elapsed_seconds, double delta_seconds);
    void Draw(gfx::RenderCommandList& cmd_list, const gfx::MeshBufferBindings& buffer_bindings, gfx::ViewState& view_state);
    void DrawParallel(gfx::ParallelRenderCommandList& parallel_cmd_list, const gfx::MeshBufferBindings& buffer_bindings, gfx::ViewState& view_state);

    [[nodiscard]] bool IsMeshLodColoringEnabled() const             { return m_mesh_lod_coloring_enabled; }
    void SetMeshLodColoringEnabled(bool mesh_lod_coloring_enabled)  { m_mesh_lod_coloring_enabled = mesh_lod_coloring_enabled; }

    [[nodiscard]] float GetMinMeshLodScreenSize() const;
    void SetMinMeshLodScreenSize(float mesh_lod_min_screen_size);

protected:
    // MeshBuffers overrides
    uint32_t GetSubsetByInstanceIndex(uint32_t instance_index) const override;

private:
    using MeshSubsetByInstanceIndex = std::vector<uint32_t>;

    void UpdateAsteroidUniforms(const Asteroid::Parameters& asteroid_parameters, const hlslpp::float3& eye_position, float elapsed_radians);

    const Settings               m_settings;
    Ptr<ContentState>            m_content_state_ptr;
    Textures                     m_unique_textures;
    Ptr<gfx::Sampler>            m_texture_sampler_ptr;
    Ptr<gfx::RenderState>        m_render_state_ptr;
    MeshSubsetByInstanceIndex    m_mesh_subset_by_instance_index;
    bool                         m_mesh_lod_coloring_enabled = false;
    float                        m_min_mesh_lod_screen_size_log_2;
};

} // namespace Methane::Samples
