/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/BaseMesh.hpp
Base mesh implementation with customizable vertex types

******************************************************************************/

#pragma once

#include <Methane/Graphics/Mesh.h>
#include <Methane/Instrumentation.h>

#include <cassert>
#include <cml/mathlib/mathlib.h>

namespace Methane::Graphics
{

template<typename VType>
class BaseMesh : public Mesh
{
public:
    using Vertices = std::vector<VType>;

    BaseMesh(Type type, const VertexLayout& vertex_layout)
        : Mesh(type, vertex_layout)
    {
        ITT_FUNCTION_TASK();
        if (sizeof(VType) != m_vertex_size)
        {
            throw std::invalid_argument("Size of vertex structure differs from vertex size calculated by vertex layout.");
        }
    }

    const Vertices& GetVertices() const noexcept       { return m_vertices; }
    Data::Size      GetVertexCount() const noexcept    { return static_cast<Data::Size>(m_vertices.size()); }
    Data::Size      GetVertexDataSize() const noexcept { return static_cast<Data::Size>(m_vertices.size() * m_vertex_size); }

protected:
    template<typename FType>
    FType& GetVertexField(VType& vertex, VertexField field) noexcept
    {
        ITT_FUNCTION_TASK();
        const int32_t field_offset = m_vertex_field_offsets[static_cast<size_t>(field)];
        assert(field_offset >= 0);
        return *reinterpret_cast<FType*>(reinterpret_cast<char*>(&vertex) + field_offset);
    }

    template<typename FType>
    const FType& GetVertexField(const VType& vertex, VertexField field) noexcept
    {
        ITT_FUNCTION_TASK();
        const int32_t field_offset = m_vertex_field_offsets[static_cast<size_t>(field)];
        assert(field_offset >= 0);
        return *reinterpret_cast<const FType*>(reinterpret_cast<const char*>(&vertex) + field_offset);
    }

    using EdgeMidpoints = std::map<Mesh::Edge, Mesh::Index>;
    Index AddEdgeMidpoint(const Edge& edge, EdgeMidpoints& edge_midpoints)
    {
        ITT_FUNCTION_TASK();
        const auto edge_midpoint_it = edge_midpoints.find(edge);
        if (edge_midpoint_it != edge_midpoints.end())
            return edge_midpoint_it->second;

        const VType& v1 = m_vertices[edge.first_index];
        const VType& v2 = m_vertices[edge.second_index];
        VType  v_mid = { };

        const Mesh::Position& v1_position = GetVertexField<Mesh::Position>(v1,    Mesh::VertexField::Position);
        const Mesh::Position& v2_position = GetVertexField<Mesh::Position>(v2,    Mesh::VertexField::Position);
        Mesh::Position&    v_mid_position = GetVertexField<Mesh::Position>(v_mid, Mesh::VertexField::Position);
        v_mid_position = (v1_position + v2_position) / 2.f;

        if (Mesh::HasVertexField(Mesh::VertexField::Normal))
        {
            const Mesh::Normal& v1_normal = GetVertexField<Mesh::Normal>(v1,    Mesh::VertexField::Normal);
            const Mesh::Normal& v2_normal = GetVertexField<Mesh::Normal>(v2,    Mesh::VertexField::Normal);
            Mesh::Normal&    v_mid_normal = GetVertexField<Mesh::Normal>(v_mid, Mesh::VertexField::Normal);
            v_mid_normal = cml::normalize(v1_normal + v2_normal);
        }

        if (Mesh::HasVertexField(Mesh::VertexField::Color))
        {
            const Mesh::Color& v1_color = GetVertexField<Mesh::Color>(v1,    Mesh::VertexField::Color);
            const Mesh::Color& v2_color = GetVertexField<Mesh::Color>(v2,    Mesh::VertexField::Color);
            Mesh::Color&    v_mid_color = GetVertexField<Mesh::Color>(v_mid, Mesh::VertexField::Color);
            v_mid_color = (v1_color + v2_color) / 2.f;
        }

        if (Mesh::HasVertexField(Mesh::VertexField::TexCoord))
        {
            const Mesh::TexCoord& v1_texcoord = GetVertexField<Mesh::TexCoord>(v1,    Mesh::VertexField::TexCoord);
            const Mesh::TexCoord& v2_texcoord = GetVertexField<Mesh::TexCoord>(v2,    Mesh::VertexField::TexCoord);
            Mesh::TexCoord&    v_mid_texcoord = GetVertexField<Mesh::TexCoord>(v_mid, Mesh::VertexField::TexCoord);
            v_mid_texcoord = (v1_texcoord + v2_texcoord) / 2.f;
        }

        const Mesh::Index v_mid_index = static_cast<Mesh::Index>(m_vertices.size());
        edge_midpoints.emplace(edge, v_mid_index);
        m_vertices.push_back(v_mid);
        return v_mid_index;
    }

    void ComputeAverageNormals()
    {
        ITT_FUNCTION_TASK();
        if (!Mesh::HasVertexField(Mesh::VertexField::Normal))
            throw std::logic_error("Mesh should contain normals.");

        if (BaseMesh::m_indices.size() % 3 != 0)
            throw std::logic_error("Mesh indices count should be a multiple of three representing triangles list.");

        for (VType& vertex : m_vertices)
        {
            Mesh::Normal& vertex_normal = GetVertexField<Mesh::Normal>(vertex, Mesh::VertexField::Normal);
            vertex_normal = { 0.f, 0.f, 0.f };
        }

        const size_t triangles_count = BaseMesh::m_indices.size() / 3;
        for (size_t triangle_index = 0; triangle_index < triangles_count; ++triangle_index)
        {
            VType& v1 = m_vertices[m_indices[triangle_index * 3]];
            VType& v2 = m_vertices[m_indices[triangle_index * 3 + 1]];
            VType& v3 = m_vertices[m_indices[triangle_index * 3 + 2]];

            const Mesh::Position& p1 = GetVertexField<Mesh::Position>(v1, Mesh::VertexField::Position);
            const Mesh::Position& p2 = GetVertexField<Mesh::Position>(v2, Mesh::VertexField::Position);
            const Mesh::Position& p3 = GetVertexField<Mesh::Position>(v3, Mesh::VertexField::Position);

            const Mesh::Position u = p2 - p1;
            const Mesh::Position v = p3 - p1;
            const Mesh::Normal   n = cml::cross(u, v);

            // NOTE: weight average by contributing face area
            Mesh::Normal& n1 = GetVertexField<Mesh::Normal>(v1, Mesh::VertexField::Normal);
            n1 += n;

            Mesh::Normal& n2 = GetVertexField<Mesh::Normal>(v2, Mesh::VertexField::Normal);
            n2 += n;

            Mesh::Normal& n3 = GetVertexField<Mesh::Normal>(v3, Mesh::VertexField::Normal);
            n3 += n;
        }

        for (VType& vertex : m_vertices)
        {
            Mesh::Normal& vertex_normal = GetVertexField<Mesh::Normal>(vertex, Mesh::VertexField::Normal);
            vertex_normal.normalize();
        }
    }

    void ValidateMeshData()
    {
        for(size_t index = 0; index < m_indices.size(); ++index)
        {
            const Index vertex_index = m_indices[index];
            if (vertex_index >= m_vertices.size())
                throw std::logic_error("Mesh index buffer value " + std::to_string(vertex_index) +
                                       " at position " + std::to_string(index) +
                                       " is out of vertex buffer size " + std::to_string(m_vertices.size()));
        }
    }

    Vertices m_vertices;
};

} // namespace Methane::Graphics
