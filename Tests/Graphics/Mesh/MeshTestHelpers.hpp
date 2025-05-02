/******************************************************************************

Copyright 2025 Evgeny Gorodetskiy

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

FILE: Tests/Graphics/Mesh/MeshTestHelpers.hpp
Mesh test helpers

******************************************************************************/

#pragma once

#include <Methane/Graphics/Mesh.h>

#include <catch2/catch_test_macros.hpp>

namespace // anonymous
{
struct MeshVertex
{
#ifdef MESH_VERTEX_POSITION
    Methane::Graphics::Mesh::Position position;
#endif
#ifdef MESH_VERTEX_NORMAL
    Methane::Graphics::Mesh::Normal   normal;
#endif
#ifdef MESH_VERTEX_COLOR
    Methane::Graphics::Mesh::Color    color;
#endif
#ifdef MESH_VERTEX_TEXCOORD
    Methane::Graphics::Mesh::TexCoord texcoord;
#endif

    bool operator==(const MeshVertex& other) const = default;

    inline static const Methane::Graphics::Mesh::VertexLayout layout{
#ifdef MESH_VERTEX_POSITION
        Methane::Graphics::Mesh::VertexField::Position,
#endif
#ifdef MESH_VERTEX_NORMAL
        Methane::Graphics::Mesh::VertexField::Normal,
#endif
#ifdef MESH_VERTEX_COLOR
        Methane::Graphics::Mesh::VertexField::Color,
#endif
#ifdef MESH_VERTEX_TEXCOORD
        Methane::Graphics::Mesh::VertexField::TexCoord
#endif
    };
};
} // anonymous namespace

template<>
struct Catch::StringMaker<MeshVertex>
{
    static std::string convert(const MeshVertex& v)
    {
        return fmt::format(
            "{{\n"
#ifdef MESH_VERTEX_POSITION
            "    .position = {},\n"
#endif
#ifdef MESH_VERTEX_NORMAL
            "    .normal   = {},\n"
#endif
#ifdef MESH_VERTEX_COLOR
            "    .color    = {},\n"
#endif
#ifdef MESH_VERTEX_TEXCOORD
            "    .texcoord = {}\n"
#endif
            "}}",
#ifdef MESH_VERTEX_POSITION
            v.position,
#endif
#ifdef MESH_VERTEX_NORMAL
            v.normal,
#endif
#ifdef MESH_VERTEX_COLOR
            v.color,
#endif
#ifdef MESH_VERTEX_TEXCOORD
            v.texcoord
#endif
            );
    }
};
