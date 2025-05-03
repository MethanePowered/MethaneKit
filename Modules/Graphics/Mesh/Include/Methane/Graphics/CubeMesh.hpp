/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/CubeMesh.hpp
Cube mesh generator with customizable vertex type

******************************************************************************/

#pragma once

#include "QuadMesh.hpp"

#include <algorithm>

namespace Methane::Graphics
{

template<typename VType>
class CubeMesh : public QuadMesh<VType>
{
    using Positions = std::vector<Mesh::Position>;
    using BaseMeshT = BaseMesh<VType>;
    using QuadMeshT = QuadMesh<VType>;

public:
    explicit CubeMesh(const Mesh::VertexLayout& vertex_layout, float width = 1.F, float height = 1.F, float depth = 1.F)
        : QuadMeshT(vertex_layout, width, height, depth / 2.F, 0, QuadMeshT::FaceType::XY, Mesh::Type::Cube)
        , m_depth(depth)
    {
        META_FUNCTION_TASK();
        AddFace(QuadMeshT(vertex_layout, width,  height, -depth  / 2.F, 1, QuadMeshT::FaceType::XY));
        AddFace(QuadMeshT(vertex_layout, width,  depth,   height / 2.F, 2, QuadMeshT::FaceType::XZ));
        AddFace(QuadMeshT(vertex_layout, width,  depth,  -height / 2.F, 3, QuadMeshT::FaceType::XZ));
        AddFace(QuadMeshT(vertex_layout, height, depth,   width  / 2.F, 4, QuadMeshT::FaceType::YZ));
        AddFace(QuadMeshT(vertex_layout, height, depth,  -width  / 2.F, 5, QuadMeshT::FaceType::YZ));
    }

    float GetDepth() const noexcept { return m_depth; }

protected:
    void AddFace(const QuadMeshT& face_mesh) noexcept
    {
        META_FUNCTION_TASK();
        const Data::Size initial_vertices_count = BaseMeshT::GetVertexCount();

        BaseMeshT::AppendVertices(face_mesh.GetVertices());

        const Mesh::Indices& face_indices = face_mesh.GetIndices();
        std::transform(face_indices.begin(), face_indices.end(), Mesh::GetIndicesBackInserter(),
            [initial_vertices_count](const Mesh::Index& index)
            {
                return static_cast<Mesh::Index>(initial_vertices_count + index);
            }
       );
    }

private:
    const float m_depth;
};

} // namespace Methane::Graphics
