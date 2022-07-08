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
#include "MultiOctavePerlinNoise.h"

#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>
#include <FastNoise/FastNoise.h>

#include <cmath>

namespace Methane::Samples
{

using AsteroidColorSchema = std::array<gfx::Color3F, Asteroid::color_schema_size>;

static gfx::Color3F TransformSrgbToLinear(const gfx::Color3F& srgb_color)
{
    META_FUNCTION_TASK();

    gfx::Color3F linear_color{};
    for (size_t c = 0U; c < gfx::Color3F::Size; ++c)
    {
        linear_color.Set(c, std::pow(srgb_color[c], 2.233333333F));
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

    std::mt19937 rng(random_seed); // NOSONAR - using pseudorandom generator is safe here

    auto                         random_persistence = std::normal_distribution<float>(0.95F, 0.04F);
    const MultiOctavePerlinNoise perlin_noise(random_persistence(rng), 4, random_seed);

    auto  random_noise = std::uniform_real_distribution<float>(0.0F, 10000.0F);
    const float noise = random_noise(rng);

    m_depth_range.first = std::numeric_limits<float>::max();
    m_depth_range.second = std::numeric_limits<float>::min();

    for (size_t vertex_index = 0; vertex_index < GetVertexCount(); ++vertex_index)
    {
        Vertex& vertex = GetMutableVertex(vertex_index);
        vertex.position *= perlin_noise(Data::RawVector4F(vertex.position * noise_scale, noise)) * radius_scale + radius_bias;

        const float vertex_depth = vertex.position.GetLength();
        m_depth_range.first = std::min(m_depth_range.first, vertex_depth);
        m_depth_range.second = std::max(m_depth_range.second, vertex_depth);
    }

    ComputeAverageNormals();
}

Asteroid::Asteroid(gfx::CommandQueue& render_cmd_queue)
    : BaseBuffers(render_cmd_queue, Mesh(3, true), "Asteroid")
{
    META_FUNCTION_TASK();
    SetTexture(GenerateTextureArray(render_cmd_queue, gfx::Dimensions(256, 256), 1, true, TextureNoiseParameters()));
}

Ptr<gfx::Texture> Asteroid::GenerateTextureArray(gfx::CommandQueue& render_cmd_queue, const gfx::Dimensions& dimensions, uint32_t array_size, bool mipmapped,
                                                 const TextureNoiseParameters& noise_parameters)
{
    META_FUNCTION_TASK();
    const gfx::Resource::SubResources sub_resources = GenerateTextureArraySubresources(dimensions, array_size, noise_parameters);
    Ptr<gfx::Texture> texture_array_ptr = gfx::Texture::CreateImage(render_cmd_queue.GetContext(), dimensions, array_size, gfx::PixelFormat::RGBA8Unorm, mipmapped);
    texture_array_ptr->SetData(sub_resources, render_cmd_queue);
    return texture_array_ptr;
}

gfx::Resource::SubResources Asteroid::GenerateTextureArraySubresources(const gfx::Dimensions& dimensions, uint32_t array_size,
                                                                       const TextureNoiseParameters& noise_parameters)
{
    META_FUNCTION_TASK();
    const gfx::PixelFormat pixel_format = gfx::PixelFormat::RGBA8Unorm;
    const uint32_t         pixel_size   = gfx::GetPixelSize(pixel_format);
    const uint32_t         pixels_count = dimensions.GetPixelsCount();
    const uint32_t         row_stride   = pixel_size * dimensions.GetWidth();

    gfx::Resource::SubResources sub_resources;
    sub_resources.reserve(array_size);

    std::mt19937 rng(noise_parameters.random_seed); // NOSONAR - using pseudorandom generator is safe here
    std::uniform_int_distribution<int> noise_seed_distribution(0, 10000);

    for (uint32_t array_index = 0; array_index < array_size; ++array_index)
    {
        Data::Bytes sub_resource_data(static_cast<size_t>(pixels_count) * pixel_size, std::byte(255));
        FillPerlinNoiseToTexture(sub_resource_data, dimensions, row_stride, noise_parameters);
        sub_resources.emplace_back(std::move(sub_resource_data), gfx::Resource::SubResource::Index{ 0, array_index });
    }

    return sub_resources;
}

Asteroid::Colors Asteroid::GetAsteroidRockColors(uint32_t deep_color_index, uint32_t shallow_color_index)
{
    META_FUNCTION_TASK();

    static const AsteroidColorSchema s_srgb_deep_rock_colors{ {
        { uint8_t( 55), uint8_t( 49), uint8_t( 40) },
        { uint8_t( 58), uint8_t( 38), uint8_t( 14) },
        { uint8_t( 98), uint8_t(101), uint8_t(104) },
        { uint8_t(172), uint8_t(158), uint8_t(122) },
        { uint8_t( 88), uint8_t( 88), uint8_t( 88) },
        { uint8_t(148), uint8_t(108), uint8_t(102) },
    } };
    static const AsteroidColorSchema s_linear_deep_rock_colors = TransformSrgbToLinear(s_srgb_deep_rock_colors);

    static const AsteroidColorSchema s_srgb_shallow_rock_colors{ {
        { uint8_t(140), uint8_t(109), uint8_t( 61) },
        { uint8_t(172), uint8_t(154), uint8_t( 58) },
        { uint8_t(204), uint8_t(177), uint8_t(119) },
        { uint8_t(204), uint8_t(164), uint8_t(136) },
        { uint8_t(130), uint8_t(117), uint8_t( 98) },
        { uint8_t(160), uint8_t(145), uint8_t(114) },
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
        { uint8_t(22), uint8_t( 51), uint8_t( 59) },
        { uint8_t(45), uint8_t( 72), uint8_t( 93) },
        { uint8_t(14), uint8_t( 25), uint8_t( 27) },
        { uint8_t(68), uint8_t(103), uint8_t(129) },
        { uint8_t(29), uint8_t( 59), uint8_t( 59) },
        { uint8_t(59), uint8_t( 92), uint8_t(118) }
    } };
    static const AsteroidColorSchema s_linear_deep_ice_colors = TransformSrgbToLinear(s_srgb_deep_ice_colors);

    static const AsteroidColorSchema s_srgb_shallow_ice_colors{ {
        { uint8_t(144), uint8_t(163), uint8_t(188) },
        { uint8_t(133), uint8_t(179), uint8_t(189) },
        { uint8_t( 74), uint8_t(135), uint8_t(178) },
        { uint8_t( 69), uint8_t(143), uint8_t(177) },
        { uint8_t(104), uint8_t(168), uint8_t(185) },
        { uint8_t(140), uint8_t(170), uint8_t(186) }
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
        {  uint8_t(  0), uint8_t(128), uint8_t(  0) }, // LOD-0: green
        {  uint8_t(  0), uint8_t( 64), uint8_t(128) }, // LOD-1: blue
        {  uint8_t( 96), uint8_t(  0), uint8_t(128) }, // LOD-2: purple
        {  uint8_t(128), uint8_t(  0), uint8_t(  0) }, // LOD-3: red
        {  uint8_t(128), uint8_t(128), uint8_t(  0) }, // LOD-4: yellow
        {  uint8_t(128), uint8_t( 64), uint8_t(  0) }, // LOD-5: orange
    } };
    static const AsteroidColorSchema s_linear_lod_deep_colors = TransformSrgbToLinear(s_srgb_lod_deep_colors);

    static const AsteroidColorSchema s_srgb_lod_shallow_colors{ {
        {  uint8_t(  0), uint8_t(255), uint8_t(  0) }, // LOD-0: green
        {  uint8_t(  0), uint8_t(128), uint8_t(255) }, // LOD-1: blue
        {  uint8_t(196), uint8_t(  0), uint8_t(255) }, // LOD-2: purple
        {  uint8_t(255), uint8_t(  0), uint8_t(  0) }, // LOD-3: red
        {  uint8_t(255), uint8_t(255), uint8_t(  0) }, // LOD-4: yellow
        {  uint8_t(255), uint8_t(128), uint8_t(  0) }, // LOD-5: orange
    } };
    static const AsteroidColorSchema s_linear_lod_shallow_colors = TransformSrgbToLinear(s_srgb_lod_shallow_colors);

    META_CHECK_ARG_LESS(lod_index, s_linear_lod_deep_colors.size());
    META_CHECK_ARG_LESS(lod_index, s_linear_lod_shallow_colors.size());
    return Asteroid::Colors{ s_linear_lod_deep_colors[lod_index], s_linear_lod_shallow_colors[lod_index] };
}

void Asteroid::FillPerlinNoiseToTexture(Data::Bytes& texture_data, const gfx::Dimensions& dimensions, uint32_t row_stride,
                                        const TextureNoiseParameters& noise_parameters)
{
    META_FUNCTION_TASK();
    static const auto fractal_noise = [noise_parameters]() {
        auto noise = FastNoise::New<FastNoise::FractalFBm>();
        noise->SetSource(FastNoise::New<FastNoise::Simplex>());
        noise->SetGain(noise_parameters.gain);
        noise->SetWeightedStrength(noise_parameters.fractal_weight);
        noise->SetOctaveCount(4);
        noise->SetLacunarity(noise_parameters.lacunarity);
        return noise;
    }();

    std::vector<float>            noise_values(dimensions.GetPixelsCount());
    const FastNoise::OutputMinMax noise_range = fractal_noise->GenTileable2D(noise_values.data(), dimensions.GetWidth(), dimensions.GetHeight(),
                                                                             noise_parameters.scale, noise_parameters.random_seed);
    const float                   noise_scale = noise_range.max - noise_range.min;

    for (size_t row = 0; row < dimensions.GetHeight(); ++row)
    {
        auto row_data = reinterpret_cast<uint32_t*>(texture_data.data() + row * row_stride); // NOSONAR
        for (size_t col = 0; col < dimensions.GetWidth(); ++col)
        {
            //const float noise_intensity = fractal_noise->GenSingle2D(noise_parameters.scale * static_cast<float>(row),
            //                                                         noise_parameters.scale * static_cast<float>(col),
            //                                                         noise_parameters.random_seed);
            const float noise_value = noise_values[col + row * dimensions.GetWidth()];
            const float noise_intensity = (noise_value - noise_range.min) * noise_parameters.strength / noise_scale;

            auto texel_data = reinterpret_cast<std::byte*>(&row_data[col]); // NOSONAR
            for (size_t channel = 0; channel < 3; ++channel)
            {
                texel_data[channel] = static_cast<std::byte>(255.F * noise_intensity);
            }
        }
    }
}

} // namespace Methane::Samples
