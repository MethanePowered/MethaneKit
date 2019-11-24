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

#include <atomic>

namespace Methane::Samples
{

constexpr size_t g_asteroid_color_schema_size = 6u;
using AsteroidColorSchema = std::array<gfx::Color3f, g_asteroid_color_schema_size>;

static gfx::Color3f TransformSRGBToLinear(const gfx::Color3f& srgb_color)
{
    gfx::Color3f linear_color = {};
    for(int c = 0; c < 3; ++c)
    {
        linear_color[c] = std::powf(srgb_color[c] / 255.f, 2.2f);
    }
    return linear_color;
}

static AsteroidColorSchema TransformSRGBToLinear(const AsteroidColorSchema& srgb_color_schema)
{
    AsteroidColorSchema linear_color_schema = {};
    for(size_t i = 0; i < srgb_color_schema.size(); ++i)
    {
        linear_color_schema[i] = TransformSRGBToLinear(srgb_color_schema[i]);
    }
    return linear_color_schema;
}

static Asteroid::Colors GetAsteroidRockColors(uint32_t deep_color_index, uint32_t shallow_color_index)
{
    static const AsteroidColorSchema s_srgb_deep_rock_colors = { {
        {  55.f,  49.f,  40.f },
        {  58.f,  38.f,  14.f },
        {  98.f, 101.f, 104.f },
        { 205.f, 197.f, 178.f },
        {  88.f,  88.f,  88.f },
        { 148.f, 108.f, 102.f },
    } };
    static const AsteroidColorSchema s_linear_deep_rock_colors = TransformSRGBToLinear(s_srgb_deep_rock_colors);

    static const AsteroidColorSchema s_srgb_shallow_rock_colors = { {
        { 156.f, 139.f, 113.f },
        { 198.f, 188.f, 137.f },
        { 239.f, 222.f, 191.f },
        { 239.f, 213.f, 198.f },
        { 153.f, 146.f, 136.f },
        { 189.f, 181.f, 164.f },
    } };
    static const AsteroidColorSchema s_linear_shallow_rock_colors = TransformSRGBToLinear(s_srgb_shallow_rock_colors);

    if (deep_color_index >= s_linear_deep_rock_colors.size() ||
        shallow_color_index >= s_linear_shallow_rock_colors.size())
        throw std::invalid_argument("Deep or shallow color indices are out of boundaries for asteroids color schema.");

    return Asteroid::Colors{ s_linear_deep_rock_colors[deep_color_index], s_linear_shallow_rock_colors[shallow_color_index] };
}

static Asteroid::Colors GetAsteroidIceColors(uint32_t deep_color_index, uint32_t shallow_color_index)
{
    static const AsteroidColorSchema s_srgb_deep_ice_colors = { {
        {   8.f,  57.f,  72.f },
        {  35.f,  79.f, 116.f },
        {   7.f,  25.f,  27.f },
        {  55.f, 116.f, 161.f },
        {  16.f,  66.f,  66.f },
        {  48.f, 103.f, 147.f }
    } };
    static const AsteroidColorSchema s_linear_deep_ice_colors = TransformSRGBToLinear(s_srgb_deep_ice_colors);

    static const AsteroidColorSchema s_srgb_shallow_ice_colors = { {
        { 199.f, 212.f, 244.f },
        { 196.f, 227.f, 239.f },
        { 133.f, 177.f, 222.f },
        { 133.f, 186.f, 230.f },
        { 167.f, 212.f, 239.f },
        { 200.f, 221.f, 252.f }
    } };
    static const AsteroidColorSchema s_linear_shallow_ice_colors = TransformSRGBToLinear(s_srgb_shallow_ice_colors);

    if (deep_color_index >= s_linear_deep_ice_colors.size() ||
        shallow_color_index >= s_linear_shallow_ice_colors.size())
        throw std::invalid_argument("Deep or shallow color indices are out of boundaries for asteroids color schema.");

    return Asteroid::Colors{ s_linear_deep_ice_colors[deep_color_index], s_linear_shallow_ice_colors[shallow_color_index] };
}

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

            m_depth_ranges.emplace_back(asteroid_mesh.GetDepthRange());
            AddSubMesh(asteroid_mesh, true);
        }
    }
}

const gfx::Vector2f& AsteroidsArray::UberMesh::GetSubMeshDepthRange(uint32_t sub_mesh_index) const
{
    if (sub_mesh_index >= m_depth_ranges.size())
        std::invalid_argument("Sub-mesh index is out of range.");

    return m_depth_ranges[sub_mesh_index];
}

AsteroidsArray::AsteroidsArray(gfx::Context& context, Settings settings)
    : AsteroidsArray(context, std::move(settings), UberMesh(settings.instance_count, settings.subdivisions_count, settings.random_seed))
{
}

 AsteroidsArray::AsteroidsArray(gfx::Context& context, Settings&& settings, const UberMesh& uber_mesh)
    : BaseBuffers(context, uber_mesh, "Asteroids Array")
    , m_settings(std::move(settings))
{
    std::mt19937 rng(settings.random_seed);
    std::normal_distribution<float>       noise_persistence_distribution(0.9f, 0.2f);
    std::uniform_real_distribution<float> noise_scale_distribution(0.05f, 0.1f);
    
    // Parallel generation sub-resources for texture arrays filled with random perlin-noise
    std::vector<gfx::Resource::SubResources> texture_subresources_array;
    texture_subresources_array.resize(m_settings.textures_count);
    Data::ParallelFor(texture_subresources_array.begin(), texture_subresources_array.end(),
        [this, &rng, &noise_scale_distribution, &noise_persistence_distribution](gfx::Resource::SubResources& sub_resources)
        {
            Asteroid::TextureNoiseParameters noise_parameters = {
                rng(),
                noise_persistence_distribution(rng),
                noise_scale_distribution(rng),
                1.5f
            };
            sub_resources = Asteroid::GenerateTextureArraySubresources(m_settings.texture_dimensions, 3, noise_parameters);
        });

    // Create texture arrays initialized with sub-resources data
    m_unique_textures.reserve(m_settings.textures_count);
    for(const gfx::Resource::SubResources& texture_subresources : texture_subresources_array)
    {
        m_unique_textures.emplace_back(gfx::Texture::CreateImage(context, m_settings.texture_dimensions, static_cast<uint32_t>(texture_subresources.size()), gfx::PixelFormat::RGBA8Unorm, true));
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
    std::uniform_int_distribution<uint32_t> colors_distribution(0, static_cast<uint32_t>(g_asteroid_color_schema_size - 1));
    std::uniform_real_distribution<float>   scale_distribution(0.7f, 1.3f);

    m_parameters.reserve(asteroids_count);
    for (uint32_t asteroid_index = 0; asteroid_index < asteroids_count; ++asteroid_index)
    {
        const uint32_t texture_index    = textures_distribution(rng);
        const uint32_t asteroid_index_x = asteroid_index % asteroids_grid_size_x;
        const uint32_t asteroid_index_y = asteroid_index / asteroids_grid_size_x;
        
        SetTexture(m_unique_textures[texture_index], asteroid_index);

        gfx::Matrix44f scale_matrix;
        cml::matrix_scale(scale_matrix,
                          m_settings.scale * scale_distribution(rng),
                          m_settings.scale * scale_distribution(rng),
                          m_settings.scale * scale_distribution(rng));

        Asteroid::Colors asteroid_colors = normal_distribution(rng) <= 1.f
                                         ? GetAsteroidIceColors(colors_distribution(rng), colors_distribution(rng))
                                         : GetAsteroidRockColors(colors_distribution(rng), colors_distribution(rng));

        m_parameters.emplace_back(Asteroid::Parameters{
            asteroid_index,
            std::move(asteroid_colors),
            uber_mesh.GetSubMeshDepthRange(asteroid_index),
            std::move(scale_matrix),
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

    for(Asteroid::Parameters& asteroid_parameters : m_parameters)
    {
        asteroid_parameters.rotation_angle_rad += cml::constants<float>::pi() * asteroid_parameters.rotation_speed * static_cast<float>(delta_seconds);

        gfx::Matrix44f rotation_matrix;
        cml::matrix_rotation_axis_angle(rotation_matrix, asteroid_parameters.rotation_axis, asteroid_parameters.rotation_angle_rad);

        gfx::Matrix44f postion_matrix;
        cml::matrix_translation(postion_matrix, asteroid_parameters.position);

        gfx::Matrix44f model_matrix = asteroid_parameters.scale_matrix * rotation_matrix * postion_matrix;

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

} // namespace Methane::Samples
