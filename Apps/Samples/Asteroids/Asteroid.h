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

#include <Methane/Graphics/Mesh.h>

namespace Methane::Samples
{

namespace gfx = Graphics;

class Asteroid
{
public:
    struct Vertex
    {
        gfx::Mesh::Position position;
        gfx::Mesh::Normal   normal;

        static constexpr const std::array<gfx::Mesh::VertexField, 2> layout = {
            gfx::Mesh::VertexField::Position,
            gfx::Mesh::VertexField::Normal,
        };
    };

    class Mesh : public gfx::IcosahedronMesh<Vertex>
    {
    public:
        Mesh();

    private:
    };

    Asteroid();

private:
};

} // namespace Methane::Samples
