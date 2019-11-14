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

FILE: Asteroid.cpp
Random generated asteroid model with mesh and texture ready for rendering

******************************************************************************/

#include "Asteroid.h"

#include <Methane/Graphics/Noise.hpp>

namespace Methane::Samples
{

Asteroid::Mesh::Mesh(uint32_t subdivisions_count, bool randomize)
    : gfx::IcosahedronMesh<Vertex>(VertexLayoutFromArray(Vertex::layout), 0.5f, subdivisions_count, true)
{
    if (randomize)
    {
        Randomize();
    }
}

void Asteroid::Mesh::Randomize(uint32_t random_seed)
{
    const float noise_scale = 0.5f;
    const float radius_scale = 1.8f;
    const float radius_bias = 0.3f;

    std::mt19937 rng(random_seed);

    auto random_persistence = std::normal_distribution<float>(0.95f, 0.04f);
    const gfx::NoiseOctaves<4> perlin_noise(random_persistence(rng));

    auto  random_noise = std::uniform_real_distribution<float>(0.0f, 10000.0f);
    const float noise = random_noise(rng);

    for (Vertex& vertex : m_vertices)
    {
        vertex.position *= perlin_noise(gfx::Vector4f(vertex.position * noise_scale, noise)) * radius_scale + radius_bias;
    }

    ComputeAverageNormals();
}

Asteroid::Asteroid(gfx::Context& context)
    : BaseBuffers(context, Mesh(3, true), "Asteroid")
{
    SetTexture(GenerateTextureArray(context, gfx::Dimensions(256, 256), 1, true, 0), 0);
}

gfx::Texture::Ptr Asteroid::GenerateTextureArray(gfx::Context& context, const gfx::Dimensions& dimensions, uint32_t array_size, bool mipmapped, uint32_t random_seed)
{
    const gfx::Resource::SubResources sub_resources = GenerateTextureArraySubresources(dimensions, array_size, mipmapped, random_seed);
    gfx::Texture::Ptr sp_texture_array = gfx::Texture::CreateImage(context, dimensions, array_size, gfx::PixelFormat::RGBA8Unorm, mipmapped);
    sp_texture_array->SetData(sub_resources);
    return sp_texture_array;
}

gfx::Resource::SubResources Asteroid::GenerateTextureArraySubresources(const gfx::Dimensions& dimensions, uint32_t array_size, bool mipmapped, uint32_t random_seed)
{
    const gfx::PixelFormat pixel_format = gfx::PixelFormat::RGBA8Unorm;
    const uint32_t pixel_size = gfx::GetPixelSize(pixel_format);
    const uint32_t pixels_count = dimensions.GetPixelsCount();
    const uint32_t row_stide = pixel_size * dimensions.width;

    gfx::Resource::SubResources sub_resources;
    sub_resources.reserve(array_size);

    std::mt19937 rng(random_seed);
    std::uniform_real_distribution<float> noise_seed_distribution(0.f, 10000.f);
    std::uniform_real_distribution<float> noise_scale_distribution(10.f, 30.f);
    std::normal_distribution<float>       persistence_distribution(0.9f, 0.2f);

    for (uint32_t array_index = 0; array_index < array_size; ++array_index)
    {
        const gfx::Color3f base_color(
#if 1
            255.f, 255.f, 255.f
#else
            array_index & 1 ? 255.f : 0.f,
            array_index & 2 ? 255.f : 0.f,
            array_index & 4 ? 255.f : 0.f
#endif
        );

        Data::Bytes sub_resource_data(pixels_count * pixel_size, 0);
        FillRandomNoiseToTexture(sub_resource_data, dimensions, pixel_size, row_stide, base_color,
            noise_seed_distribution(rng),
            persistence_distribution(rng),
            noise_scale_distribution(rng),
            1.5f);

        sub_resources.emplace_back(std::move(sub_resource_data), 0, array_index);
    }

    return sub_resources;
}

void Asteroid::FillRandomNoiseToTexture(Data::Bytes& texture_data, const gfx::Dimensions& dimensions, uint32_t pixel_size, uint32_t row_stride,
                                        const gfx::Color3f& base_color, float random_seed, float persistence, float noise_scale, float noise_strength)
{
    const gfx::NoiseOctaves<4> perlin_noise(persistence);
    
    for (size_t row = 0; row < dimensions.height; ++row)
    {
        uint32_t* row_data = reinterpret_cast<uint32_t*>(texture_data.data() + row * row_stride);
        
        for (size_t col = 0; col < dimensions.width; ++col)
        {
            const gfx::Vector3f noise_coordinates(noise_scale * row, noise_scale * col, random_seed);
            const float intensity = std::max(0.0f, std::min(1.0f, (perlin_noise(noise_coordinates) - 0.5f) * noise_strength + 0.5f));
            const gfx::Color3f texel_color = base_color * intensity;
            
            row_data[col] = (static_cast<uint32_t>(texel_color.r()) << 16u) |
                            (static_cast<uint32_t>(texel_color.g()) << 8u)  |
                            (static_cast<uint32_t>(texel_color.b()) << 0u);
        }
    }
}

} // namespace Methane::Samples
