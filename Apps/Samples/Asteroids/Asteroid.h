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

FILE: Asteroid.h
Random generated asteroid model with mesh and texture ready for rendering.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RenderContext.h>
#include <Methane/Graphics/MathTypes.h>
#include <Methane/Graphics/MeshBuffers.hpp>
#include <Methane/Graphics/Mesh/IcosahedronMesh.hpp>

namespace Methane::Samples
{

namespace gfx = Graphics;

struct SHADER_STRUCT_ALIGN AsteroidUniforms
{
    SHADER_FIELD_ALIGN gfx::Matrix44f model_matrix;
    SHADER_FIELD_ALIGN gfx::Matrix44f mvp_matrix;
    SHADER_FIELD_ALIGN gfx::Color3f   deep_color;
    SHADER_FIELD_ALIGN gfx::Color3f   shallow_color;
    SHADER_FIELD_ALIGN gfx::Vector2f  depth_range;
    SHADER_FIELD_ALIGN uint32_t       texture_index;
};

class Asteroid final : public gfx::TexturedMeshBuffers<AsteroidUniforms>
{
public:
    using BaseBuffers = gfx::TexturedMeshBuffers<AsteroidUniforms>;
    
    struct Vertex
    {
        gfx::Mesh::Position position;
        gfx::Mesh::Normal   normal;

        inline static const gfx::Mesh::VertexLayout layout = {
            gfx::Mesh::VertexField::Position,
            gfx::Mesh::VertexField::Normal,
        };
    };

    class Mesh : public gfx::IcosahedronMesh<Vertex>
    {
    public:
        Mesh(uint32_t subdivisions_count, bool randomize);

        void Randomize(uint32_t random_seed = 1337);

        const gfx::Vector2f& GetDepthRange() const { return m_depth_range; }

    private:
        gfx::Vector2f m_depth_range;
    };

    struct Colors
    {
        gfx::Color3f deep;
        gfx::Color3f shallow;
    };

    struct Parameters
    {
        const uint32_t       index;
        const uint32_t       mesh_instance_index;
        const uint32_t       texture_index;
        const Colors         colors;
        const gfx::Matrix44f scale_translate_matrix;
        const gfx::Point3f   spin_axis;
        const float          scale;
        const float          orbit_speed;
        const float          spin_speed;
        const float          spin_angle_rad;
        const float          orbit_angle_rad;
    };

    struct TextureNoiseParameters
    {
        uint32_t random_seed = 0;
        float    persistence = 0.9f;
        float    scale       = 0.5f;
        float    strength    = 1.5f;
    };

    explicit Asteroid(gfx::RenderContext& context);
    
    static Ptr<gfx::Texture> GenerateTextureArray(gfx::RenderContext& context, const gfx::Dimensions& dimensions, uint32_t array_size, bool mipmapped, const TextureNoiseParameters& noise_parameters);
    static gfx::Resource::SubResources GenerateTextureArraySubresources(const gfx::Dimensions& dimensions, uint32_t array_size, const TextureNoiseParameters& noise_parameters);

    static constexpr size_t color_schema_size = 6u;
    static Colors GetAsteroidRockColors(uint32_t deep_color_index, uint32_t shallow_color_index);
    static Colors GetAsteroidIceColors(uint32_t deep_color_index, uint32_t shallow_color_index);
    static Colors GetAsteroidLodColors(uint32_t lod_index);
    
private:
    static void FillPerlinNoiseToTexture(Data::Bytes& texture_data, const gfx::Dimensions& dimensions, uint32_t row_stride,
                                         float random_seed, float persistence, float noise_scale, float noise_strength);
};

} // namespace Methane::Samples
