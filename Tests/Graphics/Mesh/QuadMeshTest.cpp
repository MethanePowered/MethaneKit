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

FILE: Test/QuadMeshTest.cpp
Quad mesh generator unit tests

******************************************************************************/

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

constexpr uint32_t mesh_vertex_size  = 11U * 4U;
constexpr uint32_t mesh_vertex_count = 4U;
constexpr uint32_t mesh_index_size   = 2U;
constexpr uint32_t mesh_index_count  = 6U;

TEST_CASE("Quad Mesh Generator", "[mesh]")
{
    const QuadMesh<MeshVertex> mesh_xy(MeshVertex::layout, 6.F, 4.F, -1.F, 0U, QuadFaceType::XY);

    SECTION("Quad-XY Mesh Parameters")
    {
        CHECK(mesh_xy.GetType() == Mesh::Type::Quad);
        CHECK(mesh_xy.GetVertexLayout() == MeshVertex::layout);
        CHECK(mesh_xy.GetWidth()  == 6.f);
        CHECK(mesh_xy.GetHeight() == 4.f);
        CHECK(mesh_xy.GetVertexCount() == mesh_vertex_count);
        CHECK(mesh_xy.GetVertexSize() == mesh_vertex_size);
        CHECK(mesh_xy.GetVertexDataSize() == mesh_vertex_count * mesh_vertex_size );
        CHECK(mesh_xy.GetIndexCount() == mesh_index_count);
        CHECK(mesh_xy.GetIndexDataSize() == mesh_index_count * mesh_index_size);
    }

    SECTION("Quad-XY Mesh Data")
    {
        const std::vector<MeshVertex> reference_vertices = {
            { // 0
                .position = {-3.F, -2.f, -1.F},
                .normal   = { 0.F,  0.F, -1.F},
                .color    = { 1.F,  0.F,  0.F},
                .texcoord = { 0.F,  1.F}
             },
            { // 1
                .position = {-3.F, 2.F, -1.F},
                .normal   = { 0.F, 0.F, -1.F},
                .color    = { 1.F, 0.F,  0.F},
                .texcoord = { 0.F, 0.F}
            },
            { // 2
                .position = {3.F, 2.F, -1.F},
                .normal   = {0.F, 0.F, -1.F},
                .color    = {1.F, 0.F,  0.F},
                .texcoord = {1.F, 0.F}
            },
            { // 3
                .position = {3.F, -2.F, -1.F},
                .normal   = {0.F,  0.F, -1.F},
                .color    = {1.F,  0.F,  0.F},
                .texcoord = {1.F,  1.F}
            }
        };
        CheckMeshVerticesApproxEquals(mesh_xy.GetVertices(), reference_vertices);

        const Mesh::Indices mesh_indices = {
            0, 1, 2,
            0, 2, 3
        };
        CHECK(mesh_xy.GetIndices() == mesh_indices);
    }

    const QuadMesh<MeshVertex> mesh_xz(MeshVertex::layout, 6.F, 2.F, -2.F, 1U, QuadFaceType::XZ);

    SECTION("Quad-XZ Mesh Parameters")
    {
        CHECK(mesh_xz.GetType() == Mesh::Type::Quad);
        CHECK(mesh_xz.GetVertexLayout() == MeshVertex::layout);
        CHECK(mesh_xz.GetWidth()  == 6.f);
        CHECK(mesh_xz.GetHeight() == 2.f);
        CHECK(mesh_xz.GetVertexCount() == mesh_vertex_count);
        CHECK(mesh_xz.GetVertexSize() == mesh_vertex_size);
        CHECK(mesh_xz.GetVertexDataSize() == mesh_vertex_count * mesh_vertex_size );
        CHECK(mesh_xz.GetIndexCount() == mesh_index_count);
        CHECK(mesh_xz.GetIndexDataSize() == mesh_index_count * mesh_index_size);
    }

    SECTION("Quad-XZ Mesh Data")
    {
        const std::vector<MeshVertex> reference_vertices = {
            { // 0
                .position = {-3.F, -2.f, -1.F},
                .normal   = { 0.F, -1.F,  0.F},
                .color    = { 0.F,  1.F,  0.F},
                .texcoord = { 0.F,  1.F}
            },
           { // 1
               .position = {-3.F, -2.F, 1.F},
               .normal   = { 0.F, -1.F, 0.F},
               .color    = { 0.F,  1.F, 0.F},
               .texcoord = { 0.F,  0.F}
           },
           { // 2
               .position = {3.F, -2.F, 1.F},
               .normal   = {0.F, -1.F, 0.F},
               .color    = {0.F,  1.F, 0.F},
               .texcoord = {1.F,  0.F}
           },
           { // 3
               .position = {3.F, -2.F, -1.F},
               .normal   = {0.F, -1.F,  0.F},
               .color    = {0.F,  1.F,  0.F},
               .texcoord = {1.F,  1.F}
           }
        };
        CheckMeshVerticesApproxEquals(mesh_xz.GetVertices(), reference_vertices);

        const Mesh::Indices mesh_indices = {
            3, 2, 0,
            2, 1, 0
        };
        CHECK(mesh_xz.GetIndices() == mesh_indices);
    }

    const QuadMesh<MeshVertex> mesh_yz(MeshVertex::layout, 4.F, 2.F, -3.F, 2U, QuadFaceType::YZ);

    SECTION("Quad-YZ Mesh Parameters")
    {
        CHECK(mesh_yz.GetType() == Mesh::Type::Quad);
        CHECK(mesh_yz.GetVertexLayout() == MeshVertex::layout);
        CHECK(mesh_yz.GetWidth()  == 4.f);
        CHECK(mesh_yz.GetHeight() == 2.f);
        CHECK(mesh_yz.GetVertexCount() == mesh_vertex_count);
        CHECK(mesh_yz.GetVertexSize() == mesh_vertex_size);
        CHECK(mesh_yz.GetVertexDataSize() == mesh_vertex_count * mesh_vertex_size );
        CHECK(mesh_yz.GetIndexCount() == mesh_index_count);
        CHECK(mesh_yz.GetIndexDataSize() == mesh_index_count * mesh_index_size);
    }

    SECTION("Quad-YZ Mesh Data")
    {
        const std::vector<MeshVertex> reference_vertices = {
            { // 0
                .position = {-3.F, -2.f, -1.F},
                .normal   = {-1.F,  0.F,  0.F},
                .color    = { 0.F,  0.F,  1.F},
                .texcoord = { 0.F,  1.F}
            },
           { // 1
               .position = {-3.F, 2.F, -1.F},
               .normal   = {-1.F, 0.F,  0.F},
               .color    = { 0.F, 0.F,  1.F},
               .texcoord = { 0.F, 0.F}
           },
           { // 2
               .position = {-3.F, 2.F, 1.F},
               .normal   = {-1.F, 0.F, 0.F},
               .color    = {0.F,  0.F, 1.F},
               .texcoord = {1.F,  0.F}
           },
           { // 3
               .position = {-3.F, -2.F, 1.F},
               .normal   = {-1.F,  0.F, 0.F},
               .color    = {0.F,  0.F,  1.F},
               .texcoord = {1.F,  1.F}
           }
        };
        CheckMeshVerticesApproxEquals(mesh_yz.GetVertices(), reference_vertices);

        const Mesh::Indices mesh_indices = {
            3, 2, 0,
            2, 1, 0
        };
        CHECK(mesh_yz.GetIndices() == mesh_indices);
    }
}
