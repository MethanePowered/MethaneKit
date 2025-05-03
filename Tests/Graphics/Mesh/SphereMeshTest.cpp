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

FILE: Test/SphereMeshTest.cpp
Sphere mesh generator unit tests

******************************************************************************/

#include <Methane/Graphics/SphereMesh.hpp>
#include <Methane/Data/TypeFormatters.hpp>

#define MESH_VERTEX_POSITION
#define MESH_VERTEX_NORMAL
#define MESH_VERTEX_TEXCOORD
#include "MeshTestHelpers.hpp"

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <string>

using namespace Methane;
using namespace Methane::Graphics;

constexpr uint32_t mesh_vertex_size  = 8U * 4U;
constexpr uint32_t mesh_vertex_count = 12U;
constexpr uint32_t mesh_index_size   = 2U;
constexpr uint32_t mesh_index_count  = 36U;

TEST_CASE("Sphere Mesh Generator", "[mesh]")
{
    const SphereMesh<MeshVertex> mesh(MeshVertex::layout, 3.F, 3U, 3U);

    SECTION("Mesh Parameters")
    {
        CHECK(mesh.GetType() == Mesh::Type::Sphere);
        CHECK(mesh.GetVertexLayout() == MeshVertex::layout);
        CHECK(mesh.GetRadius() == 3.f);
        CHECK(mesh.GetLatLinesCount() == 3U);
        CHECK(mesh.GetLongLinesCount()  == 3U);
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
                .position = {0.F, 3.F, 0.F},
                .normal   = {0.F, 1.F, 0.F},
                .texcoord = {0.F, 0.F}
            },
            { // 1
                .position = {0.F, 3.F, 0.F},
                .normal   = {0.F, 1.F, 0.F},
                .texcoord = {0.333F, 0.F}
            },
            { // 2
                .position = {0.F, 3.F, 0.F},
                .normal   = {0.F, 1.F, 0.F},
                .texcoord = {0.667F, 0.F}
            },
            { // 3
                .position = {0.F, 3.F, 0.F},
                .normal   = {0.F, 1.F, 0.F},
                .texcoord = {1.F, 0.F}
            },
            { // 4
                .position = {3.F, 0.F, 0.F},
                .normal   = {1.F, 0.F, 0.F},
                .texcoord = {0.F, 0.25F}
            },
            { // 5
                .position = {-1.5F, 0.F, 2.598F},
                .normal   = {-0.5F, 0.F, 0.866F},
                .texcoord = {0.333F, 0.25F}
            },
            { // 6
                .position = {-1.5F, 0.F, -2.598F},
                .normal   = {-0.5F, 0.F, -0.866F},
                .texcoord = {0.667F, 0.25F}
            },
            { // 7
                .position = {3.F, 0.F, 0.F},
                .normal   = {1.F, 0.F, 0.F},
                .texcoord = {1.F, 0.25F}
            },
            { // 8
                .position = {0.F, -3.F, 0.F},
                .normal   = {0.F, -1.F, 0.F},
                .texcoord = {0.F, 0.5F}
            },
            { // 9
                .position = {0.F, -3.F, 0.F},
                .normal   = {0.F, -1.F, 0.F},
                .texcoord = {0.333F, 0.5F}
            },
            { // 10
                .position = {0.F, -3.F, 0.F},
                .normal   = {0.F, -1.F, 0.F},
                .texcoord = {0.667F, 0.5F}
            },
            { // 11
                .position = {0.F, -3.F, 0.F},
                .normal   = {0.F, -1.F, 0.F},
                .texcoord = {1.F, 0.5F}
            }
        };
        CheckMeshVerticesApproxEquals(mesh.GetVertices(), reference_vertices);

        const Mesh::Indices mesh_indices = {
            0, 1, 4,
            4, 1, 5,
            1, 2, 5,
            5, 2, 6,
            2, 3, 6,
            6, 3, 7,
            4, 5, 8,
            8, 5, 9,
            5, 6, 9,
            9, 6, 10,
            6, 7, 10,
            10, 7, 11
        };
        CHECK(mesh.GetIndices() == mesh_indices);
    }
}
