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

FILE: Asteroid.cpp
Random generated asteroid model with mesh and texture ready for rendering

******************************************************************************/

#include "Asteroid.h"

#include <Methane/Graphics/PerlinNoise.h>
#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>

#include <cmath>

namespace Methane::Samples
{

using AsteroidColorSchema = std::array<gfx::Color3f, Asteroid::color_schema_size>;

static gfx::Color3f TransformSrgbToLinear(const gfx::Color3f& srgb_color)
{
    META_FUNCTION_TASK();

    gfx::Color3f linear_color{};
    for (Data::Index c = 0U; c < gfx::Color3f::Size; ++c)
    {
        linear_color.Set(c, std::pow(srgb_color[c] / 255.F, 2.233333333F));
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
    : gfx::IcosahedronMesh<Vertex>(Mesh::VertexLayout(Vertex::layout), 0.5F, subdivisions_count, true)
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

    const float noise_scale = 0.5F;
    const float radius_scale = 1.8F;
    const float radius_bias = 0.3F;

    std::mt19937 rng(random_seed);

    auto random_persistence = std::normal_distribution<float>(0.95F, 0.04F);
    const gfx::PerlinNoise perlin_noise(random_persistence(rng));

    auto  random_noise = std::uniform_real_distribution<float>(0.0F, 10000.0F);
    const float noise = random_noise(rng);

    m_depth_range.first = std::numeric_limits<float>::max();
    m_depth_range.second = std::numeric_limits<float>::min();

    for (size_t vertex_index = 0; vertex_index < GetVertexCount(); ++vertex_index)
    {
        Vertex& vertex = GetMutableVertex(vertex_index);
        vertex.position *= perlin_noise(cml::vector4f(vertex.position * noise_scale, noise)) * radius_scale + radius_bias;

        const float vertex_depth = vertex.position.length();
        m_depth_range.first = std::min(m_depth_range.first, vertex_depth);
        m_depth_range.second = std::max(m_depth_range.second, vertex_depth);
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
    Ptr<gfx::Texture> texture_array_ptr = gfx::Texture::CreateImage(context, dimensions, array_size, gfx::PixelFormat::RGBA8Unorm, mipmapped);
    texture_array_ptr->SetData(sub_resources);
    return texture_array_ptr;
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
    std::uniform_real_distribution<float> noise_seed_distribution(0.F, 10000.F);

    for (uint32_t array_index = 0; array_index < array_size; ++array_index)
    {
        Data::Bytes sub_resource_data(pixels_count * pixel_size, std::byte(255));
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
        {  55.F,  49.F,  40.F },
        {  58.F,  38.F,  14.F },
        {  98.F, 101.F, 104.F },
        { 172.F, 158.F, 122.F },
        {  88.F,  88.F,  88.F },
        { 148.F, 108.F, 102.F },
    } };
    static const AsteroidColorSchema s_linear_deep_rock_colors = TransformSrgbToLinear(s_srgb_deep_rock_colors);

    static const AsteroidColorSchema s_srgb_shallow_rock_colors{ {
        { 140.F, 109.F,  61.F },
        { 172.F, 154.F,  58.F },
        { 204.F, 177.F, 119.F },
        { 204.F, 164.F, 136.F },
        { 130.F, 117.F,  98.F },
        { 160.F, 145.F, 114.F },
    } };
    static const AsteroidColorSchema s_linear_shallow_rock_colors = TransformSrgbToLinear(s_srgb_shallow_rock_colors);

    META_CHECK_ARG_LESS(deep_color_index, s_linear_deep_rock_colors.size());
    META_CHECK_ARG_LESS(shallow_color_index, s_linear_shallow_rock_colors.size());
    return Asteroid::Colors{ s_linear_deep_rock_colors[deep_color_index], s_linear_shallow_rock_colors[shallow_color_index] };
}

Asteroid::Colors Asteroid::GetAsteroidIceColors(uint32_t deep_color_index, uint32_t shallow_color_index)
{
    META_FUNCTION_TASK();

    static const AsteroidColorSchema s_srgb_deep_ice_colors{ {
        {  22.F,  51.F,  59.F },
        {  45.F,  72.F,  93.F },
        {  14.F,  25.F,  27.F },
        {  68.F, 103.F, 129.F },
        {  29.F,  59.F,  59.F },
        {  59.F,  92.F, 118.F }
    } };
    static const AsteroidColorSchema s_linear_deep_ice_colors = TransformSrgbToLinear(s_srgb_deep_ice_colors);

    static const AsteroidColorSchema s_srgb_shallow_ice_colors{ {
        { 144.F, 163.F, 188.F },
        { 133.F, 179.F, 189.F },
        {  74.F, 135.F, 178.F },
        {  69.F, 143.F, 177.F },
        { 104.F, 168.F, 185.F },
        { 140.F, 170.F, 186.F }
    } };
    static const AsteroidColorSchema s_linear_shallow_ice_colors = TransformSrgbToLinear(s_srgb_shallow_ice_colors);

    META_CHECK_ARG_LESS(deep_color_index, s_linear_deep_ice_colors.size());
    META_CHECK_ARG_LESS(shallow_color_index, s_linear_shallow_ice_colors.size());
    return Asteroid::Colors{ s_linear_deep_ice_colors[deep_color_index], s_linear_shallow_ice_colors[shallow_color_index] };
}

Asteroid::Colors Asteroid::GetAsteroidLodColors(uint32_t lod_index)
{
    META_FUNCTION_TASK();
    static const AsteroidColorSchema s_srgb_lod_deep_colors{ {
        {    0.F, 128.F,   0.F }, // LOD-0: green
        {    0.F,  64.F, 128.F }, // LOD-1: blue
        {   96.F,   0.F, 128.F }, // LOD-2: purple
        {  128.F,   0.F,   0.F }, // LOD-3: red
        {  128.F, 128.F,   0.F }, // LOD-4: yellow
        {  128.F,  64.F,   0.F }, // LOD-5: orange
    } };
    static const AsteroidColorSchema s_linear_lod_deep_colors = TransformSrgbToLinear(s_srgb_lod_deep_colors);

    static const AsteroidColorSchema s_srgb_lod_shallow_colors{ {
        {    0.F, 255.F,   0.F }, // LOD-0: green
        {    0.F, 128.F, 255.F }, // LOD-1: blue
        {  196.F,   0.F, 255.F }, // LOD-2: purple
        {  255.F,   0.F,   0.F }, // LOD-3: red
        {  255.F, 255.F,   0.F }, // LOD-4: yellow
        {  255.F, 128.F,   0.F }, // LOD-5: orange
    } };
    static const AsteroidColorSchema s_linear_lod_shallow_colors = TransformSrgbToLinear(s_srgb_lod_shallow_colors);

    META_CHECK_ARG_LESS(lod_index, s_linear_lod_deep_colors.size());
    META_CHECK_ARG_LESS(lod_index, s_linear_lod_shallow_colors.size());
    return Asteroid::Colors{ s_linear_lod_deep_colors[lod_index], s_linear_lod_shallow_colors[lod_index] };
}

void Asteroid::FillPerlinNoiseToTexture(Data::Bytes& texture_data, const gfx::Dimensions& dimensions, uint32_t row_stride,
                                        float random_seed, float persistence, float noise_scale, float noise_strength)
{
    META_FUNCTION_TASK();

    const gfx::PerlinNoise perlin_noise(persistence);

    for (size_t row = 0; row < dimensions.height; ++row)
    {
        auto row_data = reinterpret_cast<uint32_t*>(texture_data.data() + row * row_stride);
        
        for (size_t col = 0; col < dimensions.width; ++col)
        {
            const gfx::Vector3f noise_coordinates(noise_scale * static_cast<float>(row), noise_scale * static_cast<float>(col), random_seed);
            const float         noise_intensity = std::max(0.0F, std::min(1.0F, (perlin_noise(noise_coordinates) - 0.5F) * noise_strength + 0.5F));

            auto texel_data = reinterpret_cast<std::byte*>(&row_data[col]);
            for (size_t channel = 0; channel < 3; ++channel)
            {
                texel_data[channel] = static_cast<std::byte>(255.F * noise_intensity);
            }
        }
    }
}

} // namespace Methane::Samples
