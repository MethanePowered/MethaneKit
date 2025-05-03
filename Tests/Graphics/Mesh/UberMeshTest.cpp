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

FILE: Test/UberMeshTest.cpp
Uber mesh generator unit tests

******************************************************************************/

#include <Methane/Graphics/UberMesh.hpp>
#include <Methane/Graphics/QuadMesh.hpp>
#include <Methane/Data/TypeFormatters.hpp>

#define MESH_VERTEX_POSITION
#define MESH_VERTEX_NORMAL
#define MESH_VERTEX_COLOR
#define MESH_VERTEX_TEXCOORD
#include "MeshTestHelpers.hpp"

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <string>

using namespace Methane;
using namespace Methane::Graphics;

constexpr uint32_t mesh_subsets_count = 3U;
constexpr uint32_t mesh_vertex_size   = 11U * 4U;
constexpr uint32_t mesh_vertex_count  = 4U * mesh_subsets_count;
constexpr uint32_t mesh_index_size    = 2U;
constexpr uint32_t mesh_index_count   = 6U * mesh_subsets_count;

TEST_CASE("Uber Mesh Generator", "[mesh]")
{
    const QuadMesh<MeshVertex> mesh_xy(MeshVertex::layout, 6.F, 4.F, 0.F, 0U, QuadFaceType::XY);
    const QuadMesh<MeshVertex> mesh_xz(MeshVertex::layout, 6.F, 2.F, 0.F, 1U, QuadFaceType::XZ);
    const QuadMesh<MeshVertex> mesh_yz(MeshVertex::layout, 4.F, 2.F, 0.F, 2U, QuadFaceType::YZ);

    UberMesh<MeshVertex> mesh_uber(MeshVertex::layout);
    REQUIRE_NOTHROW(mesh_uber.AddSubMesh(mesh_xy, true));
    REQUIRE_NOTHROW(mesh_uber.AddSubMesh(mesh_xz, true));
    REQUIRE_NOTHROW(mesh_uber.AddSubMesh(mesh_yz, true));

    SECTION("Mesh Parameters")
    {
        CHECK(mesh_uber.GetType() == Mesh::Type::Uber);
        CHECK(mesh_uber.GetVertexLayout() == MeshVertex::layout);
        CHECK(mesh_uber.GetSubsetCount() == 3U);
        CHECK(mesh_uber.GetVertexCount() == mesh_vertex_count);
        CHECK(mesh_uber.GetVertexSize() == mesh_vertex_size);
        CHECK(mesh_uber.GetVertexDataSize() == mesh_vertex_count * mesh_vertex_size );
        CHECK(mesh_uber.GetIndexCount() == mesh_index_count);
        CHECK(mesh_uber.GetIndexDataSize() == mesh_index_count * mesh_index_size);
    }

    SECTION("Mesh Subsets")
    {
        uint32_t indices_offset = 0U;
        uint32_t vertices_offset =0U;

        for (uint32_t i = 0; i < mesh_subsets_count; ++i)
        {
            const Mesh::Subset& subset = mesh_uber.GetSubset(i);
            CHECK(subset.mesh_type == Mesh::Type::Quad);
            CHECK(subset.indices.count == mesh_xy.GetIndexCount());
            CHECK(subset.indices.offset == indices_offset);
            CHECK(subset.vertices.count == mesh_xy.GetVertexCount());
            CHECK(subset.vertices.offset == vertices_offset);
            CHECK(subset.indices_adjusted);

            indices_offset += subset.indices.count;
            vertices_offset += subset.vertices.count;
        }
    }

    SECTION("Mesh Data")
    {
        const std::vector<MeshVertex> reference_vertices = {
            { // 0
                .position = {-3, -2, 0},
                .normal   = {0, 0, 1},
                .color    = {1, 0, 0},
                .texcoord = {0, 1}
            },
            { // 1
                .position = {-3, 2, 0},
                .normal   = {0, 0, 1},
                .color    = {1, 0, 0},
                .texcoord = {0, 0}
            },
            { // 2
                .position = {3, 2, 0},
                .normal   = {0, 0, 1},
                .color    = {1, 0, 0},
                .texcoord = {1, 0}
            },
            { // 3
                .position = {3, -2, 0},
                .normal   = {0, 0, 1},
                .color    = {1, 0, 0},
                .texcoord = {1, 1}
            },
            { // 4
                .position = {-3, 0, -1},
                .normal   = {0, 1, 0},
                .color    = {0, 1, 0},
                .texcoord = {0, 1}
            },
            { // 5
                .position = {-3, 0, 1},
                .normal   = {0, 1, 0},
                .color    = {0, 1, 0},
                .texcoord = {0, 0}
            },
            { // 6
                .position = {3, 0, 1},
                .normal   = {0, 1, 0},
                .color    = {0, 1, 0},
                .texcoord = {1, 0}
            },
            { // 7
                .position = {3, 0, -1},
                .normal   = {0, 1, 0},
                .color    = {0, 1, 0},
                .texcoord = {1, 1}
            },
            { // 8
                .position = {0, -2, -1},
                .normal   = {1, 0, 0},
                .color    = {0, 0, 1},
                .texcoord = {0, 1}
            },
            { // 9
                .position = {0, 2, -1},
                .normal   = {1, 0, 0},
                .color    = {0, 0, 1},
                .texcoord = {0, 0}
            },
            { // 10
                .position = {0, 2, 1},
                .normal   = {1, 0, 0},
                .color    = {0, 0, 1},
                .texcoord = {1, 0}
            },
            { // 11
                .position = {0, -2, 1},
                .normal   = {1, 0, 0},
                .color    = {0, 0, 1},
                .texcoord = {1, 1}
            }
        };
        CheckMeshVerticesApproxEquals(mesh_uber.GetVertices(), reference_vertices);

        const Mesh::Indices mesh_indices = {
            3, 2, 0,
            2, 1, 0,
            4, 5, 6,
            4, 6, 7,
            8, 9, 10,
            8, 10, 11
        };
        CHECK(mesh_uber.GetIndices() == mesh_indices);
    }
}
