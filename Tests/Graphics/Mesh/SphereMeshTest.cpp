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
        const std::vector<MeshVertex> mesh_vertices = {
            { // 0
                .position = {0, 3, 0},
                .normal   = {0, 1, 0},
                .texcoord = {0, 0}
            },
            { // 1
                .position = {-0, 3, 0},
                .normal   = {-0, 1, 0},
                .texcoord = {0.33333334, 0}
            },
            { // 2
                .position = {-0, 3, -0},
                .normal   = {-0, 1, -0},
                .texcoord = {0.6666667, 0}
            },
            { // 3
                .position = {0, 3, -0},
                .normal   = {0, 1, -0},
                .texcoord = {1, 0}
            },
            { // 4
                .position = {3, 1.8369703e-16, 0},
                .normal   = {1, 6.123234e-17, 0},
                .texcoord = {0, 0.25}
            },
            { // 5
                .position = {-1.5000002, 1.8369703e-16, 2.598076},
                .normal   = {-0.50000006, 6.123234e-17, 0.8660254},
                .texcoord = {0.33333334, 0.25}
            },
            { // 6
                .position = {-1.4999996, 1.8369703e-16, -2.5980763},
                .normal   = {-0.49999988, 6.123234e-17, -0.86602545},
                .texcoord = {0.6666667, 0.25}
            },
            { // 7
                .position = {3, 1.8369703e-16, -7.347881e-16},
                .normal   = {1, 6.123234e-17, -2.4492937e-16},
                .texcoord = {1, 0.25}
            },
            { // 8
                .position = {3.6739406e-16, -3, 0},
                .normal   = {1.2246469e-16, -1, 0},
                .texcoord = {0, 0.5}
            },
            { // 9
                .position = {-1.8369704e-16, -3, 3.1817255e-16},
                .normal   = {-6.123235e-17, -1, 1.0605752e-16},
                .texcoord = {0.33333334, 0.5}
            },
            { // 10
                .position = {-1.8369699e-16, -3, -3.181726e-16},
                .normal   = {-6.123233e-17, -1, -1.0605753e-16},
                .texcoord = {0.6666667, 0.5}
            },
            { // 11
                .position = {3.6739406e-16, -3, -8.9985585e-32},
                .normal   = {1.2246469e-16, -1, -2.9995195e-32},
                .texcoord = {1, 0.5}
            }
        };
        CHECK(mesh.GetVertices() == mesh_vertices);

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
