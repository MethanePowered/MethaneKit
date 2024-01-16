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

FILE: Methane/Graphics/IcosahedronMesh.hpp
Icosahedron mesh generator with customizable vertex type

******************************************************************************/

#pragma once

#include "BaseMesh.hpp"

#include <cassert>

namespace Methane::Graphics
{

template<typename VType>
class IcosahedronMesh : public BaseMesh<VType>
{
public:
    using BaseMeshT = BaseMesh<VType>;

    explicit IcosahedronMesh(const Mesh::VertexLayout& vertex_layout, float radius = 1.F, uint32_t subdivisions_count = 0, bool spherify = false)
        : BaseMeshT(Mesh::Type::Icosahedron, vertex_layout)
        , m_radius(radius)
    {
        META_FUNCTION_TASK();

        const bool has_colors   = BaseMeshT::HasVertexField(Mesh::VertexField::Color);
        const bool has_normals  = BaseMeshT::HasVertexField(Mesh::VertexField::Normal);
        const bool has_texcoord = BaseMeshT::HasVertexField(Mesh::VertexField::TexCoord);

        META_CHECK_ARG_FALSE_DESCR(has_colors, "colored vertices are not supported by icosahedron mesh");

        const float a = (radius + std::sqrt(radius * 5.F)) / 2.F;
        const float b = radius;
        const std::array<Mesh::Position, 12> vertex_positions{ {
            {-b,  a,  0 },
            { b,  a,  0 },
            {-b, -a,  0 },
            { b, -a,  0 },
            { 0, -b,  a },
            { 0,  b,  a },
            { 0, -b, -a },
            { 0,  b, -a },
            { a,  0, -b },
            { a,  0,  b },
            {-a,  0, -b },
            {-a,  0,  b },
        } };

        BaseMeshT::ResizeVertices(vertex_positions.size());
        for(size_t vertex_index = 0; vertex_index < vertex_positions.size(); ++vertex_index)
        {
            VType& vertex = BaseMeshT::GetMutableVertex(vertex_index);

            Mesh::Position& vertex_position = BaseMeshT::template GetVertexField<Mesh::Position>(vertex, Mesh::VertexField::Position);
            vertex_position = vertex_positions[vertex_index];

            if (has_normals)
            {
                Mesh::Normal& vertex_normal = BaseMeshT::template GetVertexField<Mesh::Normal>(vertex, Mesh::VertexField::Normal);
                vertex_normal = Mesh::Normal(hlslpp::normalize(vertex_position.AsHlsl()));
            }

            if (has_texcoord)
            {
                Mesh::TexCoord& vertex_texcoord = BaseMeshT::template GetVertexField<Mesh::TexCoord>(vertex, Mesh::VertexField::TexCoord);
                const Mesh::Position vertex_direction(hlslpp::normalize(vertex_position.AsHlsl()));

                vertex_texcoord.SetX(std::atan2(vertex_direction.GetZ(), vertex_direction.GetX()) / ConstFloat::TwoPi + 0.5F);
                assert(0.F <= vertex_texcoord.GetX() && vertex_texcoord.GetX() <= 1.F);

                vertex_texcoord.SetY(std::asin(vertex_direction.GetY()) / ConstFloat::Pi + 0.5F);
                assert(0.F <= vertex_texcoord.GetY() && vertex_texcoord.GetY() <= 1.F);
            }
        }

        Mesh::SetIndices({
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
                             1, 9, 8,
                         });

        for(uint32_t subdivision = 0; subdivision < subdivisions_count; ++subdivision)
        {
            Subdivide();
        }

        if (spherify)
        {
            Spherify();
        }
    }

    float GetRadius() const noexcept  { return m_radius; }

    void Subdivide()
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_DESCR(Mesh::GetIndexCount(), Mesh::GetIndexCount() % 3 == 0,
                             "icosahedron indices count should be a multiple of three representing triangles list");

        Mesh::Indices new_indices;
        new_indices.reserve(Mesh::GetIndexCount() * 4);
        BaseMeshT::ReserveVertices(BaseMeshT::GetVertexCount() * 2);

        typename BaseMeshT::EdgeMidpoints edge_midpoints;

        const Data::Size triangles_count = Mesh::GetIndexCount() / 3;
        for (Data::Index triangle_index = 0; triangle_index < triangles_count; ++triangle_index)
        {
            const Mesh::Index vi1 = Mesh::GetIndex(triangle_index * 3);
            const Mesh::Index vi2 = Mesh::GetIndex(triangle_index * 3 + 1);
            const Mesh::Index vi3 = Mesh::GetIndex(triangle_index * 3 + 2);

            const Mesh::Index vm1 = BaseMeshT::AddEdgeMidpoint(Mesh::Edge(vi1, vi2), edge_midpoints);
            const Mesh::Index vm2 = BaseMeshT::AddEdgeMidpoint(Mesh::Edge(vi2, vi3), edge_midpoints);
            const Mesh::Index vm3 = BaseMeshT::AddEdgeMidpoint(Mesh::Edge(vi3, vi1), edge_midpoints);

            std::array<Mesh::Index, 3 * 4> indices{
                vi1, vm1, vm3,
                vm1, vi2, vm2,
                vm1, vm2, vm3,
                vm3, vm2, vi3,
            };
            new_indices.insert(new_indices.end(), indices.begin(), indices.end());
        }

        BaseMeshT::SwapIndices(new_indices);
    }

    void Spherify()
    {
        META_FUNCTION_TASK();
        const bool has_normals = BaseMeshT::HasVertexField(Mesh::VertexField::Normal);

        for(size_t vertex_index = 0; vertex_index < BaseMeshT::GetVertexCount(); ++vertex_index)
        {
            VType& vertex = BaseMeshT::GetMutableVertex(vertex_index);
            Mesh::Position& vertex_position = BaseMeshT::template GetVertexField<Mesh::Position>(vertex, Mesh::VertexField::Position);
            const Mesh::HlslPosition vertex_position_norm = hlslpp::normalize(vertex_position.AsHlsl());
            vertex_position = Mesh::Position(vertex_position_norm * m_radius);

            if (has_normals)
            {
                Mesh::Normal& vertex_normal = BaseMeshT::template GetVertexField<Mesh::Normal>(vertex, Mesh::VertexField::Normal);
                vertex_normal = Mesh::Normal(vertex_position_norm);
            }
        }
    }

private:
    const float m_radius;
};

} // namespace Methane::Graphics
