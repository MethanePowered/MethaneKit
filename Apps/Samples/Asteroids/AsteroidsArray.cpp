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

AsteroidArray::UberMesh::UberMesh(uint32_t instance_count, uint32_t subdivisions_count, uint32_t random_seed)
    : gfx::UberMesh<Asteroid::Vertex>(gfx::Mesh::VertexLayoutFromArray(Asteroid::Vertex::layout))
{
    std::mt19937 rng(random_seed);

    for(uint32_t instance_index = 0; instance_index < instance_count; ++instance_index)
    {
        const uint32_t instance_seed = rng();
        Asteroid::Mesh base_mesh(0, false);

        for (uint32_t subdivision_index = 0; subdivision_index < subdivisions_count; ++subdivision_index)
        {
            base_mesh.Subdivide();

            Asteroid::Mesh asteroid_mesh(base_mesh);
            asteroid_mesh.Spherify();
            asteroid_mesh.Randomize(instance_seed);

            AddSubMesh(asteroid_mesh, true);
        }
    }
}

AsteroidArray::AsteroidArray(gfx::Context& context, Settings settings)
    : BaseBuffers(context, UberMesh(settings.instance_count, settings.subdivisions_count, settings.random_seed), "Asteroid Array")
    , m_settings(std::move(settings))
{
    // Calculate initial asteroid positions and rotation parameters
    const uint32_t asteroids_count = GetSubsetsCount();
    const uint32_t asteroids_grid_size_x = static_cast<uint32_t>(std::sqrt(asteroids_count));
    const uint32_t asteroids_grid_size_y = static_cast<uint32_t>(std::ceil(asteroids_count / asteroids_grid_size_x));

    const float asteroid_offset_step = m_settings.scale * 2.f;
    const float grid_origin_x = asteroid_offset_step * (asteroids_grid_size_x - 1) / -2.f;
    const float grid_origin_y = asteroid_offset_step * (asteroids_grid_size_y - 1) / -2.f;

    std::mt19937 rng(settings.random_seed);
    std::normal_distribution<float> distribution;

    m_parameters.reserve(asteroids_count);
    for (uint32_t asteroid_index = 0; asteroid_index < asteroids_count; ++asteroid_index)
    {
        const uint32_t asteroid_index_x = asteroid_index % asteroids_grid_size_x;
        const uint32_t asteroid_index_y = asteroid_index / asteroids_grid_size_x;

        m_parameters.emplace_back(Asteroid::Parameters{
            asteroid_index,
            {
                grid_origin_x + asteroid_index_x * asteroid_offset_step, 0.f,
                grid_origin_y + asteroid_index_y * asteroid_offset_step
            },
            GetRandomDirection(rng),
            cml::constants<float>::pi() * distribution(rng),
            0.1f + distribution(rng) * 0.1f
        });
    }
}

bool AsteroidArray::Update(double elapsed_seconds, double delta_seconds)
{
    gfx::Matrix44f scene_view_matrix, scene_proj_matrix;
    m_settings.view_camera.GetViewProjMatrices(scene_view_matrix, scene_proj_matrix);

    gfx::Matrix44f scale_matrix;
    cml::matrix_uniform_scale(scale_matrix, m_settings.scale);
    
    for (Asteroid::Parameters& asteroid_parameters : m_parameters)
    {
        asteroid_parameters.rotation_angle_rad += cml::constants<float>::pi() * asteroid_parameters.rotation_speed * static_cast<float>(delta_seconds);

        gfx::Matrix44f rotation_matrix;
        cml::matrix_rotation_axis_angle(rotation_matrix, asteroid_parameters.rotation_axis, asteroid_parameters.rotation_angle_rad);

        gfx::Matrix44f postion_matrix;
        cml::matrix_translation(postion_matrix, asteroid_parameters.position);

        gfx::Matrix44f model_matrix = scale_matrix * rotation_matrix * postion_matrix;

        SetFinalPassUniforms(
            AsteroidUniforms
            {
                model_matrix,
                model_matrix * scene_view_matrix * scene_proj_matrix
            },
            asteroid_parameters.index
        );
    }

    return true;
}

} // namespace Methane::Samples
