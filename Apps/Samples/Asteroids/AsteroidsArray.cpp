/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: AsteroidsArray.cpp
Random generated asteroids array with uber mesh and textures ready for rendering.

******************************************************************************/

#include "AsteroidsArray.h"

#include <Methane/Graphics/Noise.hpp>
#include <Methane/Data/Parallel.hpp>

namespace Methane::Samples
{

static gfx::Point3f GetRandomDirection(std::mt19937& rng)
{
    std::normal_distribution<float> distribution;
    gfx::Point3f direction;
    do
    {
        direction = { distribution(rng), distribution(rng), distribution(rng) };
    }
    while (direction.length_squared() <= std::numeric_limits<float>::min());
    return cml::normalize(direction);
}

AsteroidsArray::UberMesh::UberMesh(uint32_t instance_count, uint32_t subdivisions_count, uint32_t random_seed)
    : gfx::UberMesh<Asteroid::Vertex>(gfx::Mesh::VertexLayoutFromArray(Asteroid::Vertex::layout))
{
    std::mt19937 rng(random_seed);

    m_depth_ranges.reserve(instance_count * subdivisions_count);
    for (uint32_t subdivision_index = 1; subdivision_index <= subdivisions_count; ++subdivision_index)
    {
        Asteroid::Mesh base_mesh(subdivision_index, false);
        base_mesh.Spherify();

        std::mutex data_mutex;
        Data::ParallelFor<uint32_t>(0u, instance_count, [&](uint32_t instance_index)
        {
            Asteroid::Mesh asteroid_mesh(base_mesh);
            asteroid_mesh.Randomize(rng());

            m_depth_ranges.emplace_back(asteroid_mesh.GetDepthRange());

            std::lock_guard<std::mutex> lock_guard(data_mutex);
            AddSubMesh(asteroid_mesh, true);
        });
    }
}

const gfx::Vector2f& AsteroidsArray::UberMesh::GetSubMeshDepthRange(uint32_t sub_mesh_index) const
{
    if (sub_mesh_index >= m_depth_ranges.size())
        std::invalid_argument("Sub-mesh index is out of range.");

    return m_depth_ranges[sub_mesh_index];
}

AsteroidsArray::State::State(const Settings& settings)
    : uber_mesh(settings.unique_mesh_count, settings.subdivisions_count, settings.random_seed)
{
    std::mt19937 rng(settings.random_seed);

    // Randomly generate perlin-noise textures
    std::normal_distribution<float>       noise_persistence_distribution(0.9f, 0.2f);
    std::uniform_real_distribution<float> noise_scale_distribution(0.05f, 0.1f);

    texture_array_subresources.resize(settings.textures_count);
    Data::ParallelFor<TextureArraySubresources::iterator, gfx::Resource::SubResources>(texture_array_subresources.begin(), texture_array_subresources.end(),
        [&](gfx::Resource::SubResources& sub_resources, Data::Index)
        {
            Asteroid::TextureNoiseParameters noise_parameters = {
                rng(),
                noise_persistence_distribution(rng),
                noise_scale_distribution(rng),
                1.5f
            };
            sub_resources = Asteroid::GenerateTextureArraySubresources(settings.texture_dimensions, 3, noise_parameters);
        });

    // Randomly generate parameters of each asteroid in array
    const uint32_t subset_count = settings.unique_mesh_count * settings.subdivisions_count;
    const float    orbit_radius = settings.orbit_radius_ratio * settings.scale;
    const float    disc_radius = settings.disc_radius_ratio * settings.scale;

    std::normal_distribution<float>         normal_distribution;
    std::uniform_int_distribution<uint32_t> subset_distribution(0u, subset_count - 1);
    std::uniform_int_distribution<uint32_t> colors_distribution(0, static_cast<uint32_t>(Asteroid::color_schema_size - 1));
    std::uniform_real_distribution<float>   scale_distribution(0.7f, 1.3f);
    std::uniform_real_distribution<float>   spin_velocity_distribution(-2.f, 2.f);
    std::uniform_real_distribution<float>   orbit_velocity_distribution(10.f, 30.f);
    std::normal_distribution<float>         orbit_radius_distribution(orbit_radius, 0.6f * disc_radius);
    std::normal_distribution<float>         orbit_height_distribution(0.0f, 0.4f * disc_radius);
    std::uniform_int_distribution<uint32_t> textures_distribution(0u, settings.textures_count - 1);

    parameters.reserve(settings.instance_count);

    for (uint32_t asteroid_index = 0; asteroid_index < settings.instance_count; ++asteroid_index)
    {
        const uint32_t      subset_index   = subset_distribution(rng);
        const float         orbit_radius   = orbit_radius_distribution(rng);
        const float         orbit_height   = orbit_height_distribution(rng);
        const gfx::Vector3f scale_proportions(scale_distribution(rng), scale_distribution(rng), scale_distribution(rng));
        const float         max_scale_proportion = std::max(std::max(scale_proportions[0], scale_proportions[1]), scale_proportions[2]);

        gfx::Matrix44f translation_matrix;
        cml::matrix_translation(translation_matrix, orbit_radius, orbit_height, 0.f);

        gfx::Matrix44f scale_matrix;
        cml::matrix_scale(scale_matrix, scale_proportions * settings.scale);

        Asteroid::Colors asteroid_colors = normal_distribution(rng) <= 1.f
                                         ? Asteroid::GetAsteroidIceColors(colors_distribution(rng), colors_distribution(rng))
                                         : Asteroid::GetAsteroidRockColors(colors_distribution(rng), colors_distribution(rng));

        parameters.emplace_back(
            Asteroid::Parameters
            {
                asteroid_index,
                subset_index,
                textures_distribution(rng),
                uber_mesh.GetSubMeshDepthRange(subset_index),
                std::move(asteroid_colors),
                std::move(scale_matrix),
                std::move(translation_matrix),
                GetRandomDirection(rng),
                orbit_velocity_distribution(rng) / (max_scale_proportion * settings.scale * orbit_radius),
                spin_velocity_distribution(rng)  / (max_scale_proportion * settings.scale),
                cml::constants<float>::pi() * normal_distribution(rng),
                cml::constants<float>::pi() * normal_distribution(rng) * 2.f
            }
        );
    }
}

AsteroidsArray::AsteroidsArray(gfx::Context& context, Settings settings)
    : AsteroidsArray(context, settings, *std::make_shared<State>(settings))
{
}

 AsteroidsArray::AsteroidsArray(gfx::Context& context, Settings settings, State& state)
    : BaseBuffers(context, state.uber_mesh, "Asteroids Array")
    , m_settings(std::move(settings))
    , m_sp_state(state.shared_from_this())
{
    SetInstanceCount(m_settings.instance_count);

    // Create texture arrays initialized with sub-resources data
    m_unique_textures.reserve(m_settings.textures_count);
    for(const gfx::Resource::SubResources& texture_subresources : m_sp_state->texture_array_subresources)
    {
        m_unique_textures.emplace_back(gfx::Texture::CreateImage(context, m_settings.texture_dimensions, static_cast<uint32_t>(texture_subresources.size()), gfx::PixelFormat::RGBA8Unorm, true));
        m_unique_textures.back()->SetData(texture_subresources);
    }

    // Distribute textures between unique mesh subsets
    for (Asteroid::Parameters& asteroid_parameters : m_sp_state->parameters)
    {
        SetSubsetTexture(m_unique_textures[asteroid_parameters.texture_index], asteroid_parameters.subset_index);
    }
}

bool AsteroidsArray::Update(double elapsed_seconds, double delta_seconds)
{
    gfx::Matrix44f scene_view_matrix, scene_proj_matrix;
    m_settings.view_camera.GetViewProjMatrices(scene_view_matrix, scene_proj_matrix);

    for(Asteroid::Parameters& asteroid_parameters : m_sp_state->parameters)
    {
        asteroid_parameters.spin_angle_rad  += cml::constants<float>::pi() * asteroid_parameters.spin_speed  * static_cast<float>(delta_seconds);
        asteroid_parameters.orbit_angle_rad += cml::constants<float>::pi() * asteroid_parameters.orbit_speed * static_cast<float>(delta_seconds);

        gfx::Matrix44f spin_rotation_matrix;
        cml::matrix_rotation_axis_angle(spin_rotation_matrix, asteroid_parameters.spin_axis, asteroid_parameters.spin_angle_rad);

        gfx::Matrix44f orbit_rotation_matrix;
        cml::matrix_rotation_world_y(orbit_rotation_matrix, asteroid_parameters.orbit_angle_rad);

        gfx::Matrix44f model_matrix = asteroid_parameters.scale_matrix * spin_rotation_matrix * asteroid_parameters.translation_matrix * orbit_rotation_matrix;

        SetFinalPassUniforms(
            AsteroidUniforms
            {
                std::move(model_matrix),
                model_matrix * scene_view_matrix * scene_proj_matrix,
                asteroid_parameters.colors.deep,
                asteroid_parameters.colors.shallow,
                asteroid_parameters.depth_range
            },
            asteroid_parameters.index
        );
    }

    return true;
}

uint32_t AsteroidsArray::GetSubsetByInstanceIndex(uint32_t instance_index) const
{
    assert(!!m_sp_state);
    assert(instance_index < m_sp_state->parameters.size());
    return m_sp_state->parameters[instance_index].subset_index;
}

} // namespace Methane::Samples
