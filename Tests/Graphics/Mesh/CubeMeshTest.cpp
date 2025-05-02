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

FILE: Test/CubeMeshTest.cpp
Cube mesh generator unit tests

******************************************************************************/

#include <Methane/Graphics/CubeMesh.hpp>
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
constexpr uint32_t mesh_vertex_count = 24U;
constexpr uint32_t mesh_index_size   = 2U;
constexpr uint32_t mesh_index_count  = 36U;

TEST_CASE("Colored Cube Mesh Generator", "[mesh]")
{
    const CubeMesh<MeshVertex> mesh(MeshVertex::layout, 6.F, 4.F, 2.F);

    SECTION("Mesh Parameters")
    {
        CHECK(mesh.GetType() == Mesh::Type::Cube);
        CHECK(mesh.GetVertexLayout() == MeshVertex::layout);
        CHECK(mesh.GetWidth()  == 6.f);
        CHECK(mesh.GetHeight() == 4.f);
        CHECK(mesh.GetDepth()  == 2.f);
        CHECK(mesh.GetVertexCount() == mesh_vertex_count);
        CHECK(mesh.GetVertexSize() == mesh_vertex_size);
        CHECK(mesh.GetVertexDataSize() == mesh_vertex_count * mesh_vertex_size );
        CHECK(mesh.GetIndexCount() == mesh_index_count);
        CHECK(mesh.GetIndexDataSize() == mesh_index_count * mesh_index_size);
    }

    SECTION("Mesh Data")
    {
        const std::vector<MeshVertex> mesh_vertices = {
            { // 0
                .position = {-3, -2, 1},
                .normal   = {0, 0, 1},
                .color    = {1, 0, 0},
                .texcoord = {0, 1}
            },
            { // 1
                .position = {-3, 2, 1},
                .normal   = {0, 0, 1},
                .color    = {1, 0, 0},
                .texcoord = {0, 0}
            },
            { // 2
                .position = {3, 2, 1},
                .normal   = {0, 0, 1},
                .color    = {1, 0, 0},
                .texcoord = {1, 0}
            },
            { // 3
                .position = {3, -2, 1},
                .normal   = {0, 0, 1},
                .color    = {1, 0, 0},
                .texcoord = {1, 1}
            },
            { // 4
                .position = {-3, -2, -1},
                .normal   = {0, 0, -1},
                .color    = {0, 1, 0},
                .texcoord = {0, 1}
            },
            { // 5
                .position = {-3, 2, -1},
                .normal   = {0, 0, -1},
                .color    = {0, 1, 0},
                .texcoord = {0, 0}
            },
            { // 6
                .position = {3, 2, -1},
                .normal   = {0, 0, -1},
                .color    = {0, 1, 0},
                .texcoord = {1, 0}
            },
            { // 7
                .position = {3, -2, -1},
                .normal   = {0, 0, -1},
                .color    = {0, 1, 0},
                .texcoord = {1, 1}
            },
            { // 8
                .position = {-3, 2, -1},
                .normal   = {0, 1, 0},
                .color    = {0, 0, 1},
                .texcoord = {0, 1}
            },
            { // 9
                .position = {-3, 2, 1},
                .normal   = {0, 1, 0},
                .color    = {0, 0, 1},
                .texcoord = {0, 0}
            },
            { // 10
                .position = {3, 2, 1},
                .normal   = {0, 1, 0},
                .color    = {0, 0, 1},
                .texcoord = {1, 0}
            },
            { // 11
                .position = {3, 2, -1},
                .normal   = {0, 1, 0},
                .color    = {0, 0, 1},
                .texcoord = {1, 1}
            },
            { // 12
                .position = {-3, -2, -1},
                .normal   = {0, -1, 0},
                .color    = {1, 0, 1},
                .texcoord = {0, 1}
            },
            { // 13
                .position = {-3, -2, 1},
                .normal   = {0, -1, 0},
                .color    = {1, 0, 1},
                .texcoord = {0, 0}
            },
            { // 14
                .position = {3, -2, 1},
                .normal   = {0, -1, 0},
                .color    = {1, 0, 1},
                .texcoord = {1, 0}
            },
            { // 15
                .position = {3, -2, -1},
                .normal   = {0, -1, 0},
                .color    = {1, 0, 1},
                .texcoord = {1, 1}
            },
            { // 16
                .position = {3, -2, -1},
                .normal   = {1, 0, 0},
                .color    = {1, 1, 0},
                .texcoord = {0, 1}
            },
            { // 17
                .position = {3, 2, -1},
                .normal   = {1, 0, 0},
                .color    = {1, 1, 0},
                .texcoord = {0, 0}
            },
            { // 18
                .position = {3, 2, 1},
                .normal   = {1, 0, 0},
                .color    = {1, 1, 0},
                .texcoord = {1, 0}
            },
            { // 19
                .position = {3, -2, 1},
                .normal   = {1, 0, 0},
                .color    = {1, 1, 0},
                .texcoord = {1, 1}
            },
            { // 20
                .position = {-3, -2, -1},
                .normal   = {-1, 0, 0},
                .color    = {0, 1, 1},
                .texcoord = {0, 1}
            },
            { // 21
                .position = {-3, 2, -1},
                .normal   = {-1, 0, 0},
                .color    = {0, 1, 1},
                .texcoord = {0, 0}
            },
            { // 22
                .position = {-3, 2, 1},
                .normal   = {-1, 0, 0},
                .color    = {0, 1, 1},
                .texcoord = {1, 0}
            },
            { // 23
                .position = {-3, -2, 1},
                .normal   = {-1, 0, 0},
                .color    = {0, 1, 1},
                .texcoord = {1, 1}
            }
        };
        CHECK(mesh.GetVertices() == mesh_vertices);

        const Mesh::Indices mesh_indices = {
            // Front
            3, 2, 0,
            2, 1, 0,

            4, 5, 6,
            4, 6, 7,

            8, 9, 10,
            8, 10, 11,

            15, 14, 12,
            14, 13, 12,

            16, 17, 18,
            16, 18, 19,

            23, 22, 20,
            22, 21, 20
        };
        CHECK(mesh.GetIndices() == mesh_indices);
    }
}
