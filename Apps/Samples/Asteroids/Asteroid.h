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

FILE: Asteroid.h
Random generated asteroid model with mesh and texture ready for rendering.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Context.h>
#include <Methane/Graphics/MathTypes.h>
#include <Methane/Graphics/Mesh.h>
#include <Methane/Graphics/MeshBuffers.hpp>

namespace Methane::Samples
{

namespace gfx = Graphics;

struct SHADER_STRUCT_ALIGN AsteroidUniforms
{
    SHADER_FIELD_ALIGN gfx::Matrix44f model_matrix;
    SHADER_FIELD_ALIGN gfx::Matrix44f mvp_matrix;
};

class Asteroid : public gfx::TexturedMeshBuffers<AsteroidUniforms>
{
public:
    using Ptr = std::unique_ptr<Asteroid>;
    using BaseBuffers = gfx::TexturedMeshBuffers<AsteroidUniforms>;
    
    struct Vertex
    {
        gfx::Mesh::Position position;
        gfx::Mesh::Normal   normal;
        gfx::Mesh::TexCoord texcoord;

        static constexpr const std::array<gfx::Mesh::VertexField, 3> layout = {
            gfx::Mesh::VertexField::Position,
            gfx::Mesh::VertexField::Normal,
            gfx::Mesh::VertexField::TexCoord,
        };
    };

    class Mesh : public gfx::IcosahedronMesh<Vertex>
    {
    public:
        Mesh();

    private:
    };

    Asteroid(gfx::Context& context);

private:
};

} // namespace Methane::Samples
