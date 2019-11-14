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

AsteroidsArray::AsteroidsArray(gfx::Context& context, Settings settings)
    : BaseBuffers(context, UberMesh(settings.instance_count, settings.subdivisions_count, settings.random_seed), "Asteroid Array")
    , m_settings(std::move(settings))
{
    std::mt19937 rng(settings.random_seed);
    
    // Generated sub-resources for texture arrays filled with random perlin-noise
    std::vector<gfx::Resource::SubResources> texture_subresources_array;
    texture_subresources_array.resize(m_settings.textures_count);
    Data::ParallelFor(texture_subresources_array.begin(), texture_subresources_array.end(),
        [this, &rng](gfx::Resource::SubResources& sub_resources)
        {
            sub_resources = Asteroid::GenerateTextureArraySubresources(m_settings.texture_dimensions, m_settings.subdivisions_count, false, rng());
        });

    // Create texture arrays initialized with sub-resources data
    m_unique_textures.reserve(m_settings.textures_count);
    for(const gfx::Resource::SubResources& texture_subresources : texture_subresources_array)
    {
        m_unique_textures.emplace_back(gfx::Texture::CreateImage(context, m_settings.texture_dimensions, static_cast<uint32_t>(texture_subresources.size()), gfx::PixelFormat::RGBA8Unorm, false));
        m_unique_textures.back()->SetData(texture_subresources);
    }
    
    // Calculate initial asteroid positions and rotation parameters
    const uint32_t asteroids_count = GetSubsetsCount();
    const uint32_t asteroids_grid_size_x = static_cast<uint32_t>(std::sqrt(asteroids_count));
    const uint32_t asteroids_grid_size_y = static_cast<uint32_t>(std::ceil(asteroids_count / asteroids_grid_size_x));

    const float asteroid_offset_step = m_settings.scale * 2.f;
    const float grid_origin_x = asteroid_offset_step * (asteroids_grid_size_x - 1) / -2.f;
    const float grid_origin_y = asteroid_offset_step * (asteroids_grid_size_y - 1) / -2.f;

    std::normal_distribution<float> normal_distribution;
    std::uniform_int_distribution<uint32_t> textures_distribution(0u, m_settings.textures_count - 1);
    //std::uniform_int_distribution<uint32_t> texture_array_distribution(0u, m_settings.subdivisions_count - 1);

    m_parameters.reserve(asteroids_count);
    for (uint32_t asteroid_index = 0; asteroid_index < asteroids_count; ++asteroid_index)
    {
        const uint32_t texture_index    = textures_distribution(rng);
        const uint32_t asteroid_index_x = asteroid_index % asteroids_grid_size_x;
        const uint32_t asteroid_index_y = asteroid_index / asteroids_grid_size_x;
        
        SetTexture(m_unique_textures[texture_index], asteroid_index);

        m_parameters.emplace_back(Asteroid::Parameters{
            asteroid_index,
            0, //texture_array_distribution(rng),
            {
                grid_origin_x + asteroid_index_x * asteroid_offset_step, 0.f,
                grid_origin_y + asteroid_index_y * asteroid_offset_step
            },
            GetRandomDirection(rng),
            cml::constants<float>::pi() * normal_distribution(rng),
            0.1f + normal_distribution(rng) * 0.1f
        });
    }
}

bool AsteroidsArray::Update(double elapsed_seconds, double delta_seconds)
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
                model_matrix * scene_view_matrix * scene_proj_matrix,
                asteroid_parameters.texture_index
            },
            asteroid_parameters.index
        );
    }

    return true;
}

} // namespace Methane::Samples
