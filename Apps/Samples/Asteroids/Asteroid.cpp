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

FILE: Asteroid.cpp
Random generated asteroid model with mesh and texture ready for rendering

******************************************************************************/

#include "Asteroid.h"

#include <Methane/Graphics/PerlinNoise.h>
#include <Methane/Instrumentation.h>

#include <cmath>

namespace Methane::Samples
{

using AsteroidColorSchema = std::array<gfx::Color3f, Asteroid::color_schema_size>;

static gfx::Color3f TransformSrgbToLinear(const gfx::Color3f& srgb_color)
{
    META_FUNCTION_TASK();

    gfx::Color3f linear_color{};
    for (int c = 0; c < 3; ++c)
    {
        linear_color[c] = std::pow(srgb_color[c] / 255.f, 2.233333333f);
    }
    return linear_color;
}

static AsteroidColorSchema TransformSrgbToLinear(const AsteroidColorSchema& srgb_color_schema)
{
    META_FUNCTION_TASK();

    AsteroidColorSchema linear_color_schema{};
    for (size_t i = 0; i < srgb_color_schema.size(); ++i)
    {
        linear_color_schema[i] = TransformSrgbToLinear(srgb_color_schema[i]);
    }
    return linear_color_schema;
}

Asteroid::Mesh::Mesh(uint32_t subdivisions_count, bool randomize)
    : gfx::IcosahedronMesh<Vertex>(Mesh::VertexLayout(Vertex::layout), 0.5f, subdivisions_count, true)
{
    META_FUNCTION_TASK();

    if (randomize)
    {
        Randomize();
    }
}

void Asteroid::Mesh::Randomize(uint32_t random_seed)
{
    META_FUNCTION_TASK();

    const float noise_scale = 0.5f;
    const float radius_scale = 1.8f;
    const float radius_bias = 0.3f;

    std::mt19937 rng(random_seed);

    auto random_persistence = std::normal_distribution<float>(0.95f, 0.04f);
    const gfx::PerlinNoise perlin_noise(random_persistence(rng));

    auto  random_noise = std::uniform_real_distribution<float>(0.0f, 10000.0f);
    const float noise = random_noise(rng);

    m_depth_range[0] = std::numeric_limits<float>::max();
    m_depth_range[1] = std::numeric_limits<float>::min();

    for (Vertex& vertex : m_vertices)
    {
        vertex.position *= perlin_noise(gfx::Vector4f(vertex.position * noise_scale, noise)) * radius_scale + radius_bias;

        const float vertex_depth = vertex.position.length();
        m_depth_range[0] = std::min(m_depth_range[0], vertex_depth);
        m_depth_range[1] = std::max(m_depth_range[1], vertex_depth);
    }

    ComputeAverageNormals();
}

Asteroid::Asteroid(gfx::RenderContext& context)
    : BaseBuffers(context, Mesh(3, true), "Asteroid")
{
    META_FUNCTION_TASK();

    SetTexture(GenerateTextureArray(context, gfx::Dimensions(256, 256), 1, true, TextureNoiseParameters()));
}

Ptr<gfx::Texture> Asteroid::GenerateTextureArray(gfx::RenderContext& context, const gfx::Dimensions& dimensions, uint32_t array_size, bool mipmapped,
                                                 const TextureNoiseParameters& noise_parameters)
{
    META_FUNCTION_TASK();

    const gfx::Resource::SubResources sub_resources = GenerateTextureArraySubresources(dimensions, array_size, noise_parameters);
    Ptr<gfx::Texture> sp_texture_array = gfx::Texture::CreateImage(context, dimensions, array_size, gfx::PixelFormat::RGBA8Unorm, mipmapped);
    sp_texture_array->SetData(sub_resources);
    return sp_texture_array;
}

gfx::Resource::SubResources Asteroid::GenerateTextureArraySubresources(const gfx::Dimensions& dimensions, uint32_t array_size, const TextureNoiseParameters& noise_parameters)
{
    META_FUNCTION_TASK();

    const gfx::PixelFormat pixel_format = gfx::PixelFormat::RGBA8Unorm;
    const uint32_t         pixel_size   = gfx::GetPixelSize(pixel_format);
    const uint32_t         pixels_count = dimensions.GetPixelsCount();
    const uint32_t         row_stride   = pixel_size * dimensions.width;

    gfx::Resource::SubResources sub_resources;
    sub_resources.reserve(array_size);

    std::mt19937 rng(noise_parameters.random_seed);
    std::uniform_real_distribution<float> noise_seed_distribution(0.f, 10000.f);

    for (uint32_t array_index = 0; array_index < array_size; ++array_index)
    {
        Data::Bytes sub_resource_data(pixels_count * pixel_size, 255u);
        FillPerlinNoiseToTexture(sub_resource_data, dimensions, row_stride,
                                 noise_seed_distribution(rng),
                                 noise_parameters.persistence,
                                 noise_parameters.scale,
                                 noise_parameters.strength);

        sub_resources.emplace_back(std::move(sub_resource_data), gfx::Resource::SubResource::Index{ 0, array_index });
    }

    return sub_resources;
}

Asteroid::Colors Asteroid::GetAsteroidRockColors(uint32_t deep_color_index, uint32_t shallow_color_index)
{
    META_FUNCTION_TASK();

    static const AsteroidColorSchema s_srgb_deep_rock_colors{ {
        {  55.f,  49.f,  40.f },
        {  58.f,  38.f,  14.f },
        {  98.f, 101.f, 104.f },
        { 172.f, 158.f, 122.f },
        {  88.f,  88.f,  88.f },
        { 148.f, 108.f, 102.f },
    } };
    static const AsteroidColorSchema s_linear_deep_rock_colors = TransformSrgbToLinear(s_srgb_deep_rock_colors);

    static const AsteroidColorSchema s_srgb_shallow_rock_colors{ {
        { 140.f, 109.f,  61.f },
        { 172.f, 154.f,  58.f },
        { 204.f, 177.f, 119.f },
        { 204.f, 164.f, 136.f },
        { 130.f, 117.f,  98.f },
        { 160.f, 145.f, 114.f },
    } };
    static const AsteroidColorSchema s_linear_shallow_rock_colors = TransformSrgbToLinear(s_srgb_shallow_rock_colors);

    if (deep_color_index >= s_linear_deep_rock_colors.size() || shallow_color_index >= s_linear_shallow_rock_colors.size())
        throw std::invalid_argument("Deep or shallow color indices are out of boundaries for asteroids color schema.");

    return Asteroid::Colors{ s_linear_deep_rock_colors[deep_color_index], s_linear_shallow_rock_colors[shallow_color_index] };
}

Asteroid::Colors Asteroid::GetAsteroidIceColors(uint32_t deep_color_index, uint32_t shallow_color_index)
{
    META_FUNCTION_TASK();

    static const AsteroidColorSchema s_srgb_deep_ice_colors{ {
        {  22.f,  51.f,  59.f },
        {  45.f,  72.f,  93.f },
        {  14.f,  25.f,  27.f },
        {  68.f, 103.f, 129.f },
        {  29.f,  59.f,  59.f },
        {  59.f,  92.f, 118.f }
    } };
    static const AsteroidColorSchema s_linear_deep_ice_colors = TransformSrgbToLinear(s_srgb_deep_ice_colors);

    static const AsteroidColorSchema s_srgb_shallow_ice_colors{ {
        { 144.f, 163.f, 188.f },
        { 133.f, 179.f, 189.f },
        {  74.f, 135.f, 178.f },
        {  69.f, 143.f, 177.f },
        { 104.f, 168.f, 185.f },
        { 140.f, 170.f, 186.f }
    } };
    static const AsteroidColorSchema s_linear_shallow_ice_colors = TransformSrgbToLinear(s_srgb_shallow_ice_colors);

    if (deep_color_index >= s_linear_deep_ice_colors.size() || shallow_color_index >= s_linear_shallow_ice_colors.size())
        throw std::invalid_argument("Deep or shallow color indices are out of boundaries for asteroids color schema.");

    return Asteroid::Colors{ s_linear_deep_ice_colors[deep_color_index], s_linear_shallow_ice_colors[shallow_color_index] };
}

Asteroid::Colors Asteroid::GetAsteroidLodColors(uint32_t lod_index)
{
    META_FUNCTION_TASK();
    static const AsteroidColorSchema s_srgb_lod_deep_colors{ {
        {    0.f, 128.f,   0.f }, // LOD-0: green
        {    0.f,  64.f, 128.f }, // LOD-1: blue
        {   96.f,   0.f, 128.f }, // LOD-2: purple
        {  128.f,   0.f,   0.f }, // LOD-3: red
        {  128.f, 128.f,   0.f }, // LOD-4: yellow
        {  128.f,  64.f,   0.f }, // LOD-5: orange
    } };
    static const AsteroidColorSchema s_linear_lod_deep_colors = TransformSrgbToLinear(s_srgb_lod_deep_colors);

    static const AsteroidColorSchema s_srgb_lod_shallow_colors{ {
        {    0.f, 255.f,   0.f }, // LOD-0: green
        {    0.f, 128.f, 255.f }, // LOD-1: blue
        {  196.f,   0.f, 255.f }, // LOD-2: purple
        {  255.f,   0.f,   0.f }, // LOD-3: red
        {  255.f, 255.f,   0.f }, // LOD-4: yellow
        {  255.f, 128.f,   0.f }, // LOD-5: orange
    } };
    static const AsteroidColorSchema s_linear_lod_shallow_colors = TransformSrgbToLinear(s_srgb_lod_shallow_colors);

    if (lod_index >= s_linear_lod_deep_colors.size() || lod_index >= s_linear_lod_shallow_colors.size())
        throw std::invalid_argument("LOD index is out of boundaries for asteroids color schema.");

    return Asteroid::Colors{ s_linear_lod_deep_colors[lod_index], s_linear_lod_shallow_colors[lod_index] };
}

void Asteroid::FillPerlinNoiseToTexture(Data::Bytes& texture_data, const gfx::Dimensions& dimensions, uint32_t row_stride,
                                        float random_seed, float persistence, float noise_scale, float noise_strength)
{
    META_FUNCTION_TASK();

    const gfx::PerlinNoise perlin_noise(persistence);

    for (size_t row = 0; row < dimensions.height; ++row)
    {
        uint32_t* row_data = reinterpret_cast<uint32_t*>(texture_data.data() + row * row_stride);
        
        for (size_t col = 0; col < dimensions.width; ++col)
        {
            const gfx::Vector3f noise_coordinates(noise_scale * row, noise_scale * col, random_seed);
            const float         noise_intensity = std::max(0.0f, std::min(1.0f, (perlin_noise(noise_coordinates) - 0.5f) * noise_strength + 0.5f));

            uint8_t* texel_data = reinterpret_cast<uint8_t*>(&row_data[col]);
            for (size_t channel = 0; channel < 3; ++channel)
            {
                texel_data[channel] = static_cast<uint8_t>(255.f * noise_intensity);
            }
        }
    }
}

} // namespace Methane::Samples
