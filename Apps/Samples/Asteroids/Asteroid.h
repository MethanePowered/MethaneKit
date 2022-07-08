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

FILE: Asteroid.h
Random generated asteroid model with mesh and texture ready for rendering.

******************************************************************************/

#pragma once

#include <Methane/Graphics/CommandQueue.h>
#include <Methane/Graphics/MeshBuffers.hpp>
#include <Methane/Graphics/IcosahedronMesh.hpp>

namespace hlslpp // NOSONAR
{
#pragma pack(push, 16)
#include "Shaders/AsteroidUniforms.h" // NOSONAR
#pragma pack(pop)
}

#include <utility>

namespace Methane::Samples
{

namespace gfx = Graphics;

class Asteroid final : public gfx::TexturedMeshBuffers<hlslpp::AsteroidUniforms>
{
public:
    using BaseBuffers = gfx::TexturedMeshBuffers<hlslpp::AsteroidUniforms>;
    
    struct Vertex
    {
        gfx::Mesh::Position position;
        gfx::Mesh::Normal   normal;

        inline static const gfx::Mesh::VertexLayout layout{
            gfx::Mesh::VertexField::Position,
            gfx::Mesh::VertexField::Normal,
        };
    };

    class Mesh : public gfx::IcosahedronMesh<Vertex>
    {
    public:
        using DepthRange = std::pair<float, float>;

        Mesh(uint32_t subdivisions_count, bool randomize);

        void Randomize(uint32_t random_seed = 1337);

        [[nodiscard]] const DepthRange& GetDepthRange() const { return m_depth_range; }

    private:
        DepthRange m_depth_range;
    };

    struct Colors
    {
        gfx::Color3F deep;
        gfx::Color3F shallow;
    };

    struct Parameters
    {
        const uint32_t         index;
        const uint32_t         mesh_instance_index;
        const uint32_t         texture_index;
        const Colors           colors;
        const hlslpp::float4x4 scale_translate_matrix;
        const hlslpp::float3   spin_axis;
        const float            scale;
        const float            orbit_speed;
        const float            spin_speed;
        const float            spin_angle_rad;
        const float            orbit_angle_rad;
    };

    struct TextureNoiseParameters
    {
        uint32_t random_seed    = 0;
        float    gain           = 0.5F;
        float    fractal_weight = 0.5F;
        float    lacunarity     = 2.0F;
        float    scale          = 0.5F;
        float    strength       = 0.8F;
    };

    explicit Asteroid(gfx::CommandQueue& render_cmd_queue);
    
    static Ptr<gfx::Texture> GenerateTextureArray(gfx::CommandQueue& render_cmd_queue, const gfx::Dimensions& dimensions,
                                                  uint32_t array_size, bool mipmapped, const TextureNoiseParameters& noise_parameters);
    static gfx::Resource::SubResources GenerateTextureArraySubresources(const gfx::Dimensions& dimensions, uint32_t array_size,
                                                                        const TextureNoiseParameters& noise_parameters);

    static constexpr size_t color_schema_size = 6U;
    static Colors GetAsteroidRockColors(uint32_t deep_color_index, uint32_t shallow_color_index);
    static Colors GetAsteroidIceColors(uint32_t deep_color_index, uint32_t shallow_color_index);
    static Colors GetAsteroidLodColors(uint32_t lod_index);
    
private:
    static void FillPerlinNoiseToTexture(Data::Bytes& texture_data, const gfx::Dimensions& dimensions, uint32_t row_stride,
                                         const TextureNoiseParameters& noise_parameters);
};

} // namespace Methane::Samples
