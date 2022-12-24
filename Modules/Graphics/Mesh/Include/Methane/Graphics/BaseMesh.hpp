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

FILE: Methane/Graphics/BaseMesh.hpp
Base mesh implementation with customizable vertex types

******************************************************************************/

#pragma once

#include <Methane/Graphics/Mesh.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics
{

template<typename VType>
class BaseMesh
    : public Mesh
{
public:
    using Vertices = std::vector<VType>;

    BaseMesh(Type type, const VertexLayout& vertex_layout)
        : Mesh(type, vertex_layout)
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_EQUAL_DESCR(GetVertexSize(), sizeof(VType), "size of vertex structure differs from vertex size calculated by vertex layout");
    }

    [[nodiscard]] const Vertices&   GetVertices() const noexcept             { return m_vertices; }
    [[nodiscard]] Data::Size        GetVertexCount() const noexcept final    { return static_cast<Data::Size>(m_vertices.size()); }
    [[nodiscard]] Data::Size        GetVertexDataSize() const noexcept final { return static_cast<Data::Size>(m_vertices.size() * GetVertexSize()); }
    [[nodiscard]] Data::ConstRawPtr GetVertexData() const noexcept final     { return reinterpret_cast<Data::ConstRawPtr>(m_vertices.data()); }

protected:
    template<typename FType>
    [[nodiscard]] FType& GetVertexField(VType& vertex, VertexField field) noexcept
    {
        META_FUNCTION_TASK();
        const int32_t field_offset = GetVertexFieldOffset(field);
        return *reinterpret_cast<FType*>(reinterpret_cast<std::byte*>(&vertex) + field_offset); // NOSONAR
    }

    template<typename FType>
    [[nodiscard]] const FType& GetVertexField(const VType& vertex, VertexField field) noexcept
    {
        META_FUNCTION_TASK();
        const int32_t field_offset = GetVertexFieldOffset(field);
        return *reinterpret_cast<const FType*>(reinterpret_cast<const std::byte*>(&vertex) + field_offset); // NOSONAR
    }

    using EdgeMidpoints = std::map<Mesh::Edge, Mesh::Index>;
    Index AddEdgeMidpoint(const Edge& edge, EdgeMidpoints& edge_midpoints)
    {
        META_FUNCTION_TASK();
        if (const auto edge_midpoint_it = edge_midpoints.find(edge);
            edge_midpoint_it != edge_midpoints.end())
            return edge_midpoint_it->second;

        const VType& v1 = m_vertices[edge.first_index];
        const VType& v2 = m_vertices[edge.second_index];
        VType  v_mid{ };

        const HlslPosition v1_position = GetVertexField<Mesh::Position>(v1, Mesh::VertexField::Position).AsHlsl();
        const HlslPosition v2_position = GetVertexField<Mesh::Position>(v2, Mesh::VertexField::Position).AsHlsl();
        Mesh::Position& v_mid_position = GetVertexField<Mesh::Position>(v_mid, Mesh::VertexField::Position);
        v_mid_position = Mesh::Position((v1_position + v2_position) / 2.F);

        if (Mesh::HasVertexField(Mesh::VertexField::Normal))
        {
            const HlslNormal v1_normal = GetVertexField<Mesh::Normal>(v1, Mesh::VertexField::Normal).AsHlsl();
            const HlslNormal v2_normal = GetVertexField<Mesh::Normal>(v2, Mesh::VertexField::Normal).AsHlsl();
            Mesh::Normal& v_mid_normal = GetVertexField<Mesh::Normal>(v_mid, Mesh::VertexField::Normal);
            v_mid_normal = Mesh::Normal(hlslpp::normalize(v1_normal + v2_normal));
        }

        if (Mesh::HasVertexField(Mesh::VertexField::Color))
        {
            const HlslColor v1_color = GetVertexField<Mesh::Color>(v1, Mesh::VertexField::Color).AsHlsl();
            const HlslColor v2_color = GetVertexField<Mesh::Color>(v2, Mesh::VertexField::Color).AsHlsl();
            Mesh::Color& v_mid_color = GetVertexField<Mesh::Color>(v_mid, Mesh::VertexField::Color);
            v_mid_color = Mesh::Color((v1_color + v2_color) / 2.F);
        }

        if (Mesh::HasVertexField(Mesh::VertexField::TexCoord))
        {
            const HlslTexCoord v1_texcoord = GetVertexField<Mesh::TexCoord>(v1,    Mesh::VertexField::TexCoord).AsHlsl();
            const HlslTexCoord v2_texcoord = GetVertexField<Mesh::TexCoord>(v2,    Mesh::VertexField::TexCoord).AsHlsl();
            Mesh::TexCoord& v_mid_texcoord = GetVertexField<Mesh::TexCoord>(v_mid, Mesh::VertexField::TexCoord);
            v_mid_texcoord = Mesh::TexCoord((v1_texcoord + v2_texcoord) / 2.F);
        }

        const auto v_mid_index = static_cast<Mesh::Index>(m_vertices.size());
        edge_midpoints.try_emplace(edge, v_mid_index);
        m_vertices.push_back(v_mid);
        return v_mid_index;
    }

    void ComputeAverageNormals()
    {
        META_FUNCTION_TASK();
        CheckLayoutHasVertexField(VertexField::Normal);
        META_CHECK_ARG_DESCR(BaseMesh::GetIndexCount(), BaseMesh::GetIndexCount() % 3 == 0,
                             "mesh indices count should be a multiple of three representing triangles list");

        for (VType& vertex : m_vertices)
        {
            Mesh::Normal& vertex_normal = GetVertexField<Mesh::Normal>(vertex, Mesh::VertexField::Normal);
            vertex_normal = { 0.F, 0.F, 0.F };
        }

        const Data::Size triangles_count = BaseMesh::GetIndexCount() / 3;
        for (Data::Index triangle_index = 0; triangle_index < triangles_count; ++triangle_index)
        {
            VType& v1 = GetMutableVertex(GetIndex(triangle_index * 3));
            VType& v2 = GetMutableVertex(GetIndex(triangle_index * 3 + 1));
            VType& v3 = GetMutableVertex(GetIndex(triangle_index * 3 + 2));

            const Mesh::HlslPosition p1 = GetVertexField<Mesh::Position>(v1, Mesh::VertexField::Position).AsHlsl();
            const Mesh::HlslPosition p2 = GetVertexField<Mesh::Position>(v2, Mesh::VertexField::Position).AsHlsl();
            const Mesh::HlslPosition p3 = GetVertexField<Mesh::Position>(v3, Mesh::VertexField::Position).AsHlsl();

            const Mesh::HlslPosition u = p2 - p1;
            const Mesh::HlslPosition v = p3 - p1;
            const Mesh::HlslNormal   n = hlslpp::cross(u, v);

            // NOTE: weight average by contributing face area
            Mesh::Normal& n1 = GetVertexField<Mesh::Normal>(v1, Mesh::VertexField::Normal);
            n1 = static_cast<Mesh::Normal>(n1.AsHlsl() + n);

            Mesh::Normal& n2 = GetVertexField<Mesh::Normal>(v2, Mesh::VertexField::Normal);
            n2 = static_cast<Mesh::Normal>(n2.AsHlsl() + n);

            Mesh::Normal& n3 = GetVertexField<Mesh::Normal>(v3, Mesh::VertexField::Normal);
            n3 = static_cast<Mesh::Normal>(n3.AsHlsl() + n);
        }

        for (VType& vertex : m_vertices)
        {
            Mesh::Normal& vertex_normal = GetVertexField<Mesh::Normal>(vertex, Mesh::VertexField::Normal);
            vertex_normal = Mesh::Normal(hlslpp::normalize(vertex_normal.AsHlsl()));
        }
    }

    void ValidateMeshData()
    {
        for(size_t index = 0; index < GetIndexCount(); ++index)
        {
            const Index vertex_index = GetIndex(index);
            META_CHECK_ARG_LESS_DESCR(vertex_index, m_vertices.size(),
                                      "mesh index buffer value at position {} is greater is out of vertex buffer bounds", index);
        }
    }

    void   ResizeVertices(size_t vertex_count) noexcept  { m_vertices.resize(vertex_count, {}); }
    void   ReserveVertices(size_t vertex_count) noexcept { m_vertices.reserve(vertex_count); }
    VType& GetMutableVertex(size_t vertex_index)         { return m_vertices[vertex_index]; }
    VType& GetMutableFirstVertex()                       { return m_vertices.front(); }
    VType& GetMutableLastVertex()                        { return m_vertices.back(); }
    void   AddVertex(VType&& vertex) noexcept            { m_vertices.emplace_back(std::move(vertex)); }
    void   AppendVertices(const Vertices& vertices)      { m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end()); }

private:
    Vertices m_vertices;
};

} // namespace Methane::Graphics
