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

FILE: Methane/Graphics/SphereMesh.hpp
Sphere mesh generator with customizable vertex type

******************************************************************************/

#pragma once

#include "BaseMesh.hpp"

#include <Methane/Data/Constants.hpp>

namespace Methane::Graphics
{

template<typename VType>
class SphereMesh : public BaseMesh<VType>
{
public:
    using BaseMeshT = BaseMesh<VType>;

    explicit SphereMesh(const Mesh::VertexLayout& vertex_layout, float radius = 1.F, Mesh::Index lat_lines_count = 10, Mesh::Index long_lines_count = 16)
        : BaseMeshT(Mesh::Type::Sphere, vertex_layout)
        , m_radius(radius)
        , m_lat_lines_count(lat_lines_count)
        , m_long_lines_count(long_lines_count)
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_NAME_DESCR("vertex_layout", !Mesh::HasVertexField(Mesh::VertexField::Color), "colored vertices are not supported by sphere mesh");
        META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(m_lat_lines_count,  3, "latitude lines count should not be less than 3");
        META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(m_long_lines_count, 3, "longitude lines count should not be less than 3");

        GenerateSphereVertices();
        GenerateSphereIndices();
    }

    float       GetRadius() const noexcept         { return m_radius; }
    Mesh::Index GetLongLinesCount() const noexcept { return m_long_lines_count; }
    Mesh::Index GetLatLinesCount() const noexcept  { return m_lat_lines_count; }

private:
    Mesh::Index GetActualLongLinesCount() const noexcept
    {
        return Mesh::HasVertexField(Mesh::VertexField::TexCoord)
             ? m_long_lines_count + 1
             : m_long_lines_count;
    }
    Mesh::Index GetSphereFacesCount() const noexcept
    {
        return (Mesh::HasVertexField(Mesh::VertexField::TexCoord)
             ? m_lat_lines_count
             : m_lat_lines_count - 2) * m_long_lines_count * 2;
    }

    void GenerateSphereVertices()
    {
        META_FUNCTION_TASK();

        // In case of textured sphere mesh,
        // an additional ending longitude line of vertices is added (with same positions as for the first line),
        // required to complete the texture projection on sphere

        const bool        has_texcoord = BaseMeshT::HasVertexField(Mesh::VertexField::TexCoord);
        const bool        has_normals  = BaseMeshT::HasVertexField(Mesh::VertexField::Normal);
        const Mesh::Index actual_long_lines_count = GetActualLongLinesCount();
        const Mesh::Index cap_vertex_count = 2 * (has_texcoord ? actual_long_lines_count : 1);

        BaseMeshT::ResizeVertices((m_lat_lines_count - 2) * actual_long_lines_count + cap_vertex_count);

        if (!has_texcoord)
        {
            Mesh::Position& first_vertex_position = BaseMeshT::template GetVertexField<Mesh::Position>(BaseMeshT::GetMutableFirstVertex(), Mesh::VertexField::Position);
            Mesh::Position& last_vertex_position  = BaseMeshT::template GetVertexField<Mesh::Position>(BaseMeshT::GetMutableLastVertex(), Mesh::VertexField::Position);

            first_vertex_position = Mesh::Position(0.F,  m_radius, 0.F);
            last_vertex_position  = Mesh::Position(0.F, -m_radius, 0.F);

            if (has_normals)
            {
                Mesh::Normal& first_vertex_normal = BaseMeshT::template GetVertexField<Mesh::Normal>(BaseMeshT::GetMutableFirstVertex(), Mesh::VertexField::Normal);
                Mesh::Normal& last_vertex_normal  = BaseMeshT::template GetVertexField<Mesh::Normal>(BaseMeshT::GetMutableLastVertex(), Mesh::VertexField::Normal);

                first_vertex_normal = Mesh::Normal(0.F,  1.F, 0.F);
                last_vertex_normal  = Mesh::Normal(0.F, -1.F, 0.F);
            }
        }

        const float texcoord_long_spacing = 1.F / (actual_long_lines_count - 1);
        const float texcoord_lat_spacing  = 1.F / (m_lat_lines_count + 1);

        const uint32_t actual_lat_lines_count = has_texcoord ? m_lat_lines_count : m_lat_lines_count - 1;
        const uint32_t first_lat_line_index   = has_texcoord ? 0 : 1;
        const uint32_t first_vertex_index     = has_texcoord ? 0 : 1;

        for (uint32_t lat_line_index = first_lat_line_index; lat_line_index < actual_lat_lines_count; ++lat_line_index)
        {
            const float lat_ratio = static_cast<float>(lat_line_index) / static_cast<float>(actual_lat_lines_count - 1);

            for(uint32_t long_line_index = 0; long_line_index < actual_long_lines_count; ++long_line_index)
            {
                const uint32_t vertex_index = (lat_line_index - first_lat_line_index) * actual_long_lines_count + long_line_index + first_vertex_index;
                const float    long_ratio   = static_cast<float>(long_line_index) / static_cast<float>(actual_long_lines_count - 1);

                VType& vertex = BaseMeshT::GetMutableVertex(vertex_index);

                Mesh::Position& vertex_position = BaseMeshT::template GetVertexField<Mesh::Position>(vertex, Mesh::VertexField::Position);
                vertex_position.SetX(std::sin(ConstFloat::Pi * lat_ratio) * std::cos(ConstFloat::TwoPi * long_ratio));
                vertex_position.SetZ(std::sin(ConstFloat::Pi * lat_ratio) * std::sin(ConstFloat::TwoPi * long_ratio));
                vertex_position.SetY(std::cos(ConstFloat::Pi * lat_ratio));

                if (has_normals)
                {
                    Mesh::Normal& vertex_normal = BaseMeshT::template GetVertexField<Mesh::Normal>(vertex, Mesh::VertexField::Normal);
                    vertex_normal = vertex_position;
                }

                vertex_position *= m_radius;

                if (has_texcoord)
                {
                    Mesh::TexCoord& vertex_texcoord = BaseMeshT::template GetVertexField<Mesh::TexCoord>(vertex, Mesh::VertexField::TexCoord);
                    vertex_texcoord.SetX(texcoord_long_spacing * long_line_index);
                    vertex_texcoord.SetY(texcoord_lat_spacing * lat_line_index);
                }
            }
        }
    }

    void GenerateSphereIndices()
    {
        META_FUNCTION_TASK();
        const bool        has_texcoord            = BaseMeshT::HasVertexField(Mesh::VertexField::TexCoord);
        const Mesh::Index actual_long_lines_count = GetActualLongLinesCount();
        const Mesh::Index sphere_faces_count      = GetSphereFacesCount();
        Data::Index       index_offset            = 0;

        Mesh::ResizeIndices(sphere_faces_count * 3);

        if (!has_texcoord)
        {
            // Top cap triangles reuse single pole vertex

            for (Mesh::Index long_line_index = 0; long_line_index < actual_long_lines_count - 1; ++long_line_index)
            {
                Mesh::SetIndex(index_offset, 0);
                Mesh::SetIndex(index_offset + 1, long_line_index + 2);
                Mesh::SetIndex(index_offset + 2, long_line_index + 1);

                index_offset += 3;
            }

            Mesh::SetIndex(index_offset, 0);
            Mesh::SetIndex(index_offset + 1, 1);
            Mesh::SetIndex(index_offset + 2, m_long_lines_count);

            index_offset += 3;
        }

        const auto        vertices_count         = static_cast<Mesh::Index>(BaseMeshT::GetVertexCount());
        const Mesh::Index index_lat_lines_count  = has_texcoord ? m_lat_lines_count - 1 : m_lat_lines_count - 3;
        const Mesh::Index index_long_lines_count = has_texcoord ? m_long_lines_count    : m_long_lines_count - 1;
        const Mesh::Index first_vertex_index     = has_texcoord ? 0 : 1;

        for (Mesh::Index lat_line_index = 0; lat_line_index < index_lat_lines_count; ++lat_line_index)
        {
            for (Mesh::Index long_line_index = 0; long_line_index < index_long_lines_count; ++long_line_index)
            {
                Mesh::SetIndex(index_offset,     (lat_line_index * actual_long_lines_count) + long_line_index + first_vertex_index);
                Mesh::SetIndex(index_offset + 1, (lat_line_index * actual_long_lines_count) + long_line_index + first_vertex_index + 1);
                Mesh::SetIndex(index_offset + 2, (lat_line_index + 1) * actual_long_lines_count + long_line_index + first_vertex_index);

                Mesh::SetIndex(index_offset + 3, (lat_line_index + 1) * actual_long_lines_count + long_line_index + first_vertex_index);
                Mesh::SetIndex(index_offset + 4, (lat_line_index * actual_long_lines_count) + long_line_index + first_vertex_index + 1);
                Mesh::SetIndex(index_offset + 5, (lat_line_index + 1) * actual_long_lines_count + long_line_index + first_vertex_index + 1);

                index_offset += 6;
            }

            if (!has_texcoord)
            {
                Mesh::SetIndex(index_offset,     (lat_line_index * actual_long_lines_count) + actual_long_lines_count);
                Mesh::SetIndex(index_offset + 1, (lat_line_index * actual_long_lines_count) + 1);
                Mesh::SetIndex(index_offset + 2, (lat_line_index + 1) * actual_long_lines_count + actual_long_lines_count);

                Mesh::SetIndex(index_offset + 3, (lat_line_index + 1) * actual_long_lines_count + actual_long_lines_count);
                Mesh::SetIndex(index_offset + 4, (lat_line_index * actual_long_lines_count) + 1);
                Mesh::SetIndex(index_offset + 5, (lat_line_index + 1) * actual_long_lines_count + 1);

                index_offset += 6;
            }
        }

        if (!has_texcoord)
        {
            // Bottom cap triangles reuse single pole vertex

            for (Mesh::Index long_line_index = 0; long_line_index < index_long_lines_count; ++long_line_index)
            {
                Mesh::SetIndex(index_offset,     vertices_count - 1);
                Mesh::SetIndex(index_offset + 1, vertices_count - 1 - long_line_index + 2);
                Mesh::SetIndex(index_offset + 2, vertices_count - 1 - long_line_index + 1);

                index_offset += 3;
            }

            Mesh::SetIndex(index_offset,     vertices_count - 1);
            Mesh::SetIndex(index_offset + 1, vertices_count - 2);
            Mesh::SetIndex(index_offset + 2, vertices_count - 1 - actual_long_lines_count);
        }
    }

    const float       m_radius;
    const Mesh::Index m_lat_lines_count;
    const Mesh::Index m_long_lines_count;
};

} // namespace Methane::Graphics
