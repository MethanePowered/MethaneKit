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

namespace Methane::Graphics
{

template<typename VType>
class SphereMesh : public BaseMesh<VType>
{
public:
    using BaseMeshT = BaseMesh<VType>;

    explicit SphereMesh(const Mesh::VertexLayout& vertex_layout, float radius = 1.F, uint32_t lat_lines_count = 10, uint32_t long_lines_count = 16)
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

    float    GetRadius() const noexcept         { return m_radius; }
    uint32_t GetLongLinesCount() const noexcept { return m_long_lines_count; }
    uint32_t GetLatLinesCount() const noexcept  { return m_lat_lines_count; }

private:
    uint32_t GetActualLongLinesCount() const noexcept { return  Mesh::HasVertexField(Mesh::VertexField::TexCoord) ? m_long_lines_count + 1 : m_long_lines_count; }
    uint32_t GetSphereFacesCount() const noexcept     { return (Mesh::HasVertexField(Mesh::VertexField::TexCoord) ? m_lat_lines_count : m_lat_lines_count - 2) * m_long_lines_count * 2; }

    void GenerateSphereVertices()
    {
        META_FUNCTION_TASK();

        // In case of textured sphere mesh,
        // an additional ending longitude line of vertices is added (with same positions as for the first line),
        // required to complete the texture projection on sphere

        const bool     has_texcoord = Mesh::HasVertexField(Mesh::VertexField::TexCoord);
        const bool     has_normals  = Mesh::HasVertexField(Mesh::VertexField::Normal);
        const uint32_t actual_long_lines_count = GetActualLongLinesCount();
        const uint32_t cap_vertex_count = 2 * (has_texcoord ? actual_long_lines_count : 1);

        BaseMeshT::ResizeVertices((m_lat_lines_count - 2) * actual_long_lines_count + cap_vertex_count);

        if (!has_texcoord)
        {
            Mesh::Position& first_vertex_position = BaseMeshT::template GetVertexField<Mesh::Position>(BaseMeshT::GetMutableFirstVertex(), Mesh::VertexField::Position);
            Mesh::Position& last_vertex_position = BaseMeshT::template GetVertexField<Mesh::Position>(BaseMeshT::GetMutableLastVertex(), Mesh::VertexField::Position);

            first_vertex_position = Mesh::Position(0.F,  m_radius, 0.F);
            last_vertex_position  = Mesh::Position(0.F, -m_radius, 0.F);

            if (has_normals)
            {
                Mesh::Normal& first_vertex_normal = BaseMeshT::template GetVertexField<Mesh::Normal>(BaseMeshT::GetMutableFirstVertex(), Mesh::VertexField::Normal);
                Mesh::Normal& last_vertex_normal = BaseMeshT::template GetVertexField<Mesh::Normal>(BaseMeshT::GetMutableLastVertex(), Mesh::VertexField::Normal);

                first_vertex_normal = Mesh::Normal(0.F,  1.F, 0.F);
                last_vertex_normal  = Mesh::Normal(0.F, -1.F, 0.F);
            }
        }

        const float texcoord_long_spacing = 1.F / (actual_long_lines_count - 1);
        const float texcoord_lat_spacing  = 1.F / (m_lat_lines_count + 1);

        Matrix33f pitch_step_matrix{ };
        Matrix33f yaw_step_matrix{ };
        cml::matrix_rotation_world_x(pitch_step_matrix, -cml::constants<float>::pi() / (m_lat_lines_count - 1));
        cml::matrix_rotation_world_y(yaw_step_matrix, -2.0 * cml::constants<float>::pi() / m_long_lines_count);

        Matrix33f pitch_matrix{ };
        Matrix33f yaw_matrix{ };
        pitch_matrix.identity();

        if (!has_texcoord)
            pitch_matrix = pitch_step_matrix;

        const uint32_t actual_lat_lines_count = has_texcoord ? m_lat_lines_count : m_lat_lines_count - 1;
        const uint32_t first_lat_line_index   = has_texcoord ? 0 : 1;
        const uint32_t first_vertex_index     = has_texcoord ? 0 : 1;

        for (uint32_t lat_line_index = first_lat_line_index; lat_line_index < actual_lat_lines_count; ++lat_line_index)
        {
            yaw_matrix.identity();

            for(uint32_t long_line_index = 0; long_line_index < actual_long_lines_count; ++long_line_index)
            {
                const Matrix33f rotation_matrix = pitch_matrix * yaw_matrix;
                const uint32_t  vertex_index    = (lat_line_index - first_lat_line_index) * actual_long_lines_count + long_line_index + first_vertex_index;

                VType& vertex = BaseMeshT::GetMutableVertex(vertex_index);
                {
                    Mesh::Position& vertex_position = BaseMeshT::template GetVertexField<Mesh::Position>(vertex, Mesh::VertexField::Position);
                    vertex_position = Mesh::Position(0.F, m_radius, 0.F) * rotation_matrix;
                }
                if (has_normals)
                {
                    Mesh::Normal& vertex_normal = BaseMeshT::template GetVertexField<Mesh::Normal>(vertex, Mesh::VertexField::Normal);
                    vertex_normal = Mesh::Normal(0.F, 1.F, 0.F) * rotation_matrix;
                }
                if (has_texcoord)
                {
                    Mesh::TexCoord& vertex_texcoord = BaseMeshT::template GetVertexField<Mesh::TexCoord>(vertex, Mesh::VertexField::TexCoord);
                    vertex_texcoord = Mesh::TexCoord(texcoord_long_spacing * long_line_index, texcoord_lat_spacing * lat_line_index);
                }

                yaw_matrix = yaw_matrix * yaw_step_matrix;
            }

            pitch_matrix = pitch_matrix * pitch_step_matrix;
        }
    }

    void GenerateSphereIndices()
    {
        META_FUNCTION_TASK();

        const bool     has_texcoord            = Mesh::HasVertexField(Mesh::VertexField::TexCoord);
        const uint32_t actual_long_lines_count = GetActualLongLinesCount();
        const uint32_t sphere_faces_count      = GetSphereFacesCount();
        uint32_t       index_offset            = 0;

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
            Mesh::SetIndex(index_offset + 2, static_cast<Mesh::Index>(m_long_lines_count));

            index_offset += 3;
        }

        const auto     vertices_count         = static_cast<uint32_t>(BaseMeshT::GetVertexCount());
        const uint32_t index_lat_lines_count  = has_texcoord ? m_lat_lines_count - 1 : m_lat_lines_count - 3;
        const uint32_t index_long_lines_count = has_texcoord ? m_long_lines_count : m_long_lines_count - 1;
        const uint32_t first_vertex_index     = has_texcoord ? 0 : 1;

        for (uint32_t lat_line_index = 0; lat_line_index < index_lat_lines_count; ++lat_line_index)
        {
            for (uint32_t long_line_index = 0; long_line_index < index_long_lines_count; ++long_line_index)
            {
                Mesh::SetIndex(index_offset, static_cast<Mesh::Index>((lat_line_index * actual_long_lines_count) + long_line_index + first_vertex_index));
                Mesh::SetIndex(index_offset + 1,
                               static_cast<Mesh::Index>((lat_line_index * actual_long_lines_count) + long_line_index + first_vertex_index + 1));
                Mesh::SetIndex(index_offset + 2,
                               static_cast<Mesh::Index>((lat_line_index + 1) * actual_long_lines_count + long_line_index + first_vertex_index));

                Mesh::SetIndex(index_offset + 3,
                               static_cast<Mesh::Index>((lat_line_index + 1) * actual_long_lines_count + long_line_index + first_vertex_index));
                Mesh::SetIndex(index_offset + 4,
                               static_cast<Mesh::Index>((lat_line_index * actual_long_lines_count) + long_line_index + first_vertex_index + 1));
                Mesh::SetIndex(index_offset + 5,
                               static_cast<Mesh::Index>((lat_line_index + 1) * actual_long_lines_count + long_line_index + first_vertex_index + 1));

                index_offset += 6;
            }

            if (!has_texcoord)
            {
                Mesh::SetIndex(index_offset, static_cast<Mesh::Index>((lat_line_index * actual_long_lines_count) + actual_long_lines_count));
                Mesh::SetIndex(index_offset + 1, static_cast<Mesh::Index>((lat_line_index * actual_long_lines_count) + 1));
                Mesh::SetIndex(index_offset + 2, static_cast<Mesh::Index>((lat_line_index + 1) * actual_long_lines_count + actual_long_lines_count));

                Mesh::SetIndex(index_offset + 3, static_cast<Mesh::Index>((lat_line_index + 1) * actual_long_lines_count + actual_long_lines_count));
                Mesh::SetIndex(index_offset + 4, static_cast<Mesh::Index>((lat_line_index * actual_long_lines_count) + 1));
                Mesh::SetIndex(index_offset + 5, static_cast<Mesh::Index>((lat_line_index + 1) * actual_long_lines_count + 1));

                index_offset += 6;
            }
        }

        if (!has_texcoord)
        {
            // Bottom cap triangles reuse single pole vertex

            for (uint32_t long_line_index = 0; long_line_index < index_long_lines_count; ++long_line_index)
            {
                Mesh::SetIndex(index_offset, static_cast<Mesh::Index>(vertices_count - 1));
                Mesh::SetIndex(index_offset + 1, static_cast<Mesh::Index>((vertices_count - 1) - (long_line_index + 2)));
                Mesh::SetIndex(index_offset + 2, static_cast<Mesh::Index>((vertices_count - 1) - (long_line_index + 1)));

                index_offset += 3;
            }

            Mesh::SetIndex(index_offset, static_cast<Mesh::Index>(vertices_count - 1));
            Mesh::SetIndex(index_offset + 1, static_cast<Mesh::Index>(vertices_count - 2));
            Mesh::SetIndex(index_offset + 2, static_cast<Mesh::Index>((vertices_count - 1) - actual_long_lines_count));
        }
    }

    const float    m_radius;
    const uint32_t m_lat_lines_count;
    const uint32_t m_long_lines_count;
};

} // namespace Methane::Graphics
