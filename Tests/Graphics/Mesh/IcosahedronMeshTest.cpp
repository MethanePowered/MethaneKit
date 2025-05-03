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

FILE: Test/IcosahedronMeshTest.cpp
Icosahedron mesh generator unit tests

******************************************************************************/

#include <Methane/Graphics/IcosahedronMesh.hpp>
#include <Methane/Data/TypeFormatters.hpp>

#define MESH_VERTEX_POSITION
#define MESH_VERTEX_NORMAL
#define MESH_VERTEX_TEXCOORD
#include "MeshTestHelpers.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <fmt/format.h>
#include <string>

using namespace Methane;
using namespace Methane::Graphics;

constexpr uint32_t mesh_vertex_size  = 8U * 4U;
constexpr uint32_t mesh_vertex_count = 12U;
constexpr uint32_t mesh_index_size   = 2U;
constexpr uint32_t mesh_index_count  = 60U;

TEST_CASE("Icosahedron Mesh Generator", "[mesh]")
{
    IcosahedronMesh<MeshVertex> mesh(MeshVertex::layout, 3.F);

    SECTION("Mesh Parameters")
    {
        CHECK(mesh.GetType() == Mesh::Type::Icosahedron);
        CHECK(mesh.GetVertexLayout() == MeshVertex::layout);
        CHECK(mesh.GetRadius() == 3.f);
        CHECK(mesh.GetVertexCount() == mesh_vertex_count);
        CHECK(mesh.GetVertexSize() == mesh_vertex_size);
        CHECK(mesh.GetVertexDataSize() == mesh_vertex_count * mesh_vertex_size );
        CHECK(mesh.GetIndexCount() == mesh_index_count);
        CHECK(mesh.GetIndexDataSize() == mesh_index_count * mesh_index_size);
    }

    SECTION("Mesh Data")
    {
        const std::vector<MeshVertex> reference_vertices = {
            { // 0
                .position = {-3.F, 3.436F, 0.F},
                .normal   = {-0.658F, 0.753F, 0.F},
                .texcoord = {1.F, 0.771F}
            },
            { // 1
                .position = {3.F, 3.436F, 0.F},
                .normal   = {0.658F, 0.753F, 0.F},
                .texcoord = {0.5F, 0.771F}
            },
            { // 2
                .position = {-3.F, -3.436F, 0.F},
                .normal   = {-0.658F, -0.753F, 0.F},
                .texcoord = {1.F, 0.228F}
            },
            { // 3
                .position = {3.F, -3.436F, 0.F},
                .normal   = {0.658F, -0.753F, 0.F},
                .texcoord = {0.5F, 0.228F}
            },
            { // 4
                .position = {0.F, -3.F, 3.436F},
                .normal   = {0.F, -0.658F, 0.753F},
                .texcoord = {0.75F, 0.271F}
            },
            { // 5
                .position = {0.F, 3.F, 3.436F},
                .normal   = {0.F, 0.658F, 0.753F},
                .texcoord = {0.75F, 0.728F}
            },
            { // 6
                .position = {0.F, -3.F, -3.436F},
                .normal   = {0.F, -0.658F, -0.753F},
                .texcoord = {0.25F, 0.271F}
            },
            { // 7
                .position = {0.F, 3.F, -3.436F},
                .normal   = {0.F, 0.658F, -0.753F},
                .texcoord = {0.25F, 0.728F}
            },
            { // 8
                .position = {3.436F, 0.F, -3.F},
                .normal   = {0.753F, 0.F, -0.658F},
                .texcoord = {0.386F, 0.5F}
            }, {
                .position = {3.436F, 0.F, 3.F},
                .normal   = {0.753F, 0.F, 0.658F},
                .texcoord = {0.614F, 0.5F}
            }, {
                .position = {-3.436F, 0.F, -3.F},
                .normal   = {-0.753F, 0.F, -0.658F},
                .texcoord = {0.1142F, 0.5F}
            }, {
                .position = {-3.436F, 0.F, 3.F},
                .normal   = {-0.753F, 0.F, 0.658F},
                .texcoord = {0.886F, 0.5F}
            }
        };
        CheckMeshVerticesApproxEquals(mesh.GetVertices(), reference_vertices);

        const Mesh::Indices mesh_indices = {
            5, 0, 11,
            1, 0, 5,
            7, 0, 1,
            10, 0, 7,
            11, 0, 10,
            9, 1, 5,
            4, 5, 11,
            2, 11, 10,
            6, 10, 7,
            8, 7, 1,
            4, 3, 9,
            2, 3, 4,
            6, 3, 2,
            8, 3, 6,
            9, 3, 8,
            5, 4, 9,
            11, 2, 4,
            10, 6, 2,
            7, 8, 6,
            1, 9, 8
        };
        CHECK(mesh.GetIndices() == mesh_indices);
    }

    SECTION("Subdivide")
    {
        CHECK(mesh.GetVertexCount() == mesh_vertex_count);
        CHECK(mesh.GetIndexCount() == mesh_index_count);

        REQUIRE_NOTHROW(mesh.Subdivide());

        CHECK(mesh.GetVertexCount() == mesh_vertex_count + mesh_index_count / 2);
        CHECK(mesh.GetIndexCount() == 4U * mesh_index_count);
    }

    SECTION("Spherify")
    {
        REQUIRE_NOTHROW(mesh.Spherify());

        for(const MeshVertex& mesh_vertex : mesh.GetVertices())
        {
            CHECK(mesh_vertex.position.GetLength() == mesh.GetRadius());
            CHECK(mesh_vertex.normal == Mesh::Normal(hlslpp::normalize(mesh_vertex.position.AsHlsl())));
        }
    }
}
