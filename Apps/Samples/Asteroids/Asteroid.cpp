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

#include <sstream>

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

Asteroid::Asteroid(gfx::Context& context)
    : BaseBuffers(context, Mesh(3, true), "Asteroid")
{
    SetTexture(GenerateTextureArray(context, gfx::Dimensions(256, 256), 1, true, TextureNoiseParameters()), 0);
}

gfx::Texture::Ptr Asteroid::GenerateTextureArray(gfx::Context& context, const gfx::Dimensions& dimensions, uint32_t array_size, bool mipmapped, 
                                                 const TextureNoiseParameters& noise_parameters)
{
    const gfx::Resource::SubResources sub_resources = GenerateTextureArraySubresources(dimensions, array_size, noise_parameters);
    gfx::Texture::Ptr sp_texture_array = gfx::Texture::CreateImage(context, dimensions, array_size, gfx::PixelFormat::RGBA8Unorm, mipmapped);
    sp_texture_array->SetData(sub_resources);
    return sp_texture_array;
}

gfx::Resource::SubResources Asteroid::GenerateTextureArraySubresources(const gfx::Dimensions& dimensions, uint32_t array_size, const TextureNoiseParameters& noise_parameters)
{
    const gfx::PixelFormat pixel_format = gfx::PixelFormat::RGBA8Unorm;
    const uint32_t pixel_size = gfx::GetPixelSize(pixel_format);
    const uint32_t pixels_count = dimensions.GetPixelsCount();
    const uint32_t row_stide = pixel_size * dimensions.width;

    gfx::Resource::SubResources sub_resources;
    sub_resources.reserve(array_size);

    std::mt19937 rng(noise_parameters.random_seed);
    std::uniform_real_distribution<float> noise_seed_distribution(0.f, 10000.f);

    for (uint32_t array_index = 0; array_index < array_size; ++array_index)
    {
        Data::Bytes sub_resource_data(pixels_count * pixel_size, 255u);
        FillPerlinNoiseToTexture(sub_resource_data, dimensions,
                                 pixel_size, row_stide,
                                 noise_seed_distribution(rng),
                                 noise_parameters.persistence,
                                 noise_parameters.scale,
                                 noise_parameters.strength,
                                 array_index);

        sub_resources.emplace_back(std::move(sub_resource_data), gfx::Resource::SubResource::Index{ 0, array_index });
    }

    return sub_resources;
}

void Asteroid::FillPerlinNoiseToTexture(Data::Bytes& texture_data, const gfx::Dimensions& dimensions, uint32_t pixel_size, uint32_t row_stride,
                                        float random_seed, float persistence, float noise_scale, float noise_strength, uint32_t array_index)
{
    const gfx::NoiseOctaves<4> perlin_noise(persistence);
    
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
#if 1
                const float channel_value = 255.f;
#else
                const float channel_value = array_index % 3 == channel ? 255.f : 125.f;
#endif
                texel_data[channel] = static_cast<uint8_t>(channel_value * noise_intensity);
            }
            
        }
    }
}

} // namespace Methane::Samples
