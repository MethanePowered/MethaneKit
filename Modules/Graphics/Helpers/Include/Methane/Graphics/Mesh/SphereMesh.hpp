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

    explicit SphereMesh(const Mesh::VertexLayout& vertex_layout, float radius = 1.f, uint32_t lat_lines_count = 10, uint32_t long_lines_count = 16)
        : BaseMeshT(Mesh::Type::Sphere, vertex_layout)
        , m_radius(radius)
        , m_lat_lines_count(lat_lines_count)
        , m_long_lines_count(long_lines_count)
    {
        ITT_FUNCTION_TASK();

        if (Mesh::HasVertexField(Mesh::VertexField::Color))
        {
            throw std::invalid_argument("Colored vertices are not supported for sphere mesh.");
        }
        if (m_lat_lines_count < 3)
        {
            throw std::invalid_argument("Latitude lines count should not be less than 3.");
        }
        if (m_long_lines_count < 3)
        {
            throw std::invalid_argument("Longitude lines count should not be less than 3.");
        }

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
        ITT_FUNCTION_TASK();

        // In case of textured sphere mesh,
        // an additional ending longitude line of vertices is added (with same positions as for the first line),
        // required to complete the texture projection on sphere

        const bool     has_texcoord = Mesh::HasVertexField(Mesh::VertexField::TexCoord);
        const bool     has_normals  = Mesh::HasVertexField(Mesh::VertexField::Normal);
        const uint32_t actual_long_lines_count = GetActualLongLinesCount();
        const uint32_t cap_vertex_count = 2 * (has_texcoord ? actual_long_lines_count : 1);

        BaseMeshT::m_vertices.resize((m_lat_lines_count - 2) * actual_long_lines_count + cap_vertex_count, {});

        if (!has_texcoord)
        {
            Mesh::Position& first_vertex_position = BaseMeshT::template GetVertexField<Mesh::Position>(BaseMeshT::m_vertices.front(), Mesh::VertexField::Position);
            Mesh::Position& last_vertex_position = BaseMeshT::template GetVertexField<Mesh::Position>(BaseMeshT::m_vertices.back(), Mesh::VertexField::Position);

            first_vertex_position = Mesh::Position(0.f, m_radius, 0.f);
            last_vertex_position = Mesh::Position(0.f, -m_radius, 0.f);

            if (has_normals)
            {
                Mesh::Normal& first_vertex_normal = BaseMeshT::template GetVertexField<Mesh::Normal>(BaseMeshT::m_vertices.front(), Mesh::VertexField::Normal);
                Mesh::Normal& last_vertex_normal = BaseMeshT::template GetVertexField<Mesh::Normal>(BaseMeshT::m_vertices.back(), Mesh::VertexField::Normal);

                first_vertex_normal = Mesh::Normal(0.f, 1.f, 0.f);
                last_vertex_normal = Mesh::Normal(0.f, -1.f, 0.f);
            }
        }

        const float texcoord_long_spacing = 1.f / (actual_long_lines_count - 1);
        const float texcoord_lat_spacing  = 1.f / (m_lat_lines_count + 1);

        Matrix33f pitch_step_matrix = { }, yaw_step_matrix = { };
        cml::matrix_rotation_world_x(pitch_step_matrix, -cml::constants<float>::pi() / (m_lat_lines_count - 1));
        cml::matrix_rotation_world_y(yaw_step_matrix, -2.0 * cml::constants<float>::pi() / m_long_lines_count);

        Matrix33f pitch_matrix = { }, yaw_matrix = { };;
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

                VType& vertex = BaseMeshT::m_vertices[vertex_index];
                {
                    Mesh::Position& vertex_position = BaseMeshT::template GetVertexField<Mesh::Position>(vertex, Mesh::VertexField::Position);
                    vertex_position = Mesh::Position(0.f, m_radius, 0.f) * rotation_matrix;
                }
                if (has_normals)
                {
                    Mesh::Normal& vertex_normal = BaseMeshT::template GetVertexField<Mesh::Normal>(vertex, Mesh::VertexField::Normal);
                    vertex_normal = Mesh::Normal(0.f, 1.f, 0.f) * rotation_matrix;
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
        ITT_FUNCTION_TASK();

        const bool     has_texcoord            = Mesh::HasVertexField(Mesh::VertexField::TexCoord);
        const uint32_t actual_long_lines_count = GetActualLongLinesCount();
        const uint32_t sphere_faces_count      = GetSphereFacesCount();

        BaseMeshT::m_indices.resize(sphere_faces_count * 3, 0);

        uint32_t index_offset = 0;

        if (!has_texcoord)
        {
            // Top cap triangles reuse single pole vertex

            for (Mesh::Index long_line_index = 0; long_line_index < actual_long_lines_count - 1; ++long_line_index)
            {
                BaseMeshT::m_indices[index_offset]     = 0;
                BaseMeshT::m_indices[index_offset + 1] = long_line_index + 2;
                BaseMeshT::m_indices[index_offset + 2] = long_line_index + 1;

                index_offset += 3;
            }

            BaseMeshT::m_indices[index_offset]     = 0;
            BaseMeshT::m_indices[index_offset + 1] = 1;
            BaseMeshT::m_indices[index_offset + 2] = m_long_lines_count;

            index_offset += 3;
        }

        const uint32_t vertices_count         = static_cast<uint32_t>(BaseMeshT::m_vertices.size());
        const uint32_t index_lat_lines_count  = has_texcoord ? m_lat_lines_count - 1 : m_lat_lines_count - 3;
        const uint32_t index_long_lines_count = has_texcoord ? m_long_lines_count : m_long_lines_count - 1;
        const uint32_t first_vertex_index     = has_texcoord ? 0 : 1;

        for (uint32_t lat_line_index = 0; lat_line_index < index_lat_lines_count; ++lat_line_index)
        {
            for (uint32_t long_line_index = 0; long_line_index < index_long_lines_count; ++long_line_index)
            {
                BaseMeshT::m_indices[index_offset]     = (lat_line_index      * actual_long_lines_count) + long_line_index + first_vertex_index;
                BaseMeshT::m_indices[index_offset + 1] = (lat_line_index      * actual_long_lines_count) + long_line_index + first_vertex_index + 1;
                BaseMeshT::m_indices[index_offset + 2] = (lat_line_index + 1) * actual_long_lines_count  + long_line_index + first_vertex_index;

                BaseMeshT::m_indices[index_offset + 3] = (lat_line_index + 1) * actual_long_lines_count  + long_line_index + first_vertex_index;
                BaseMeshT::m_indices[index_offset + 4] = (lat_line_index      * actual_long_lines_count) + long_line_index + first_vertex_index + 1;
                BaseMeshT::m_indices[index_offset + 5] = (lat_line_index + 1) * actual_long_lines_count  + long_line_index + first_vertex_index + 1;

                index_offset += 6;
            }

            if (!has_texcoord)
            {
                BaseMeshT::m_indices[index_offset]     = (lat_line_index      * actual_long_lines_count) + actual_long_lines_count;
                BaseMeshT::m_indices[index_offset + 1] = (lat_line_index      * actual_long_lines_count) + 1;
                BaseMeshT::m_indices[index_offset + 2] = (lat_line_index + 1) * actual_long_lines_count  + actual_long_lines_count;

                BaseMeshT::m_indices[index_offset + 3] = (lat_line_index + 1) * actual_long_lines_count  + actual_long_lines_count;
                BaseMeshT::m_indices[index_offset + 4] = (lat_line_index      * actual_long_lines_count) + 1;
                BaseMeshT::m_indices[index_offset + 5] = (lat_line_index + 1) * actual_long_lines_count  + 1;

                index_offset += 6;
            }
        }

        if (!has_texcoord)
        {
            // Bottom cap triangles reuse single pole vertex

            for (uint32_t long_line_index = 0; long_line_index < index_long_lines_count; ++long_line_index)
            {
                BaseMeshT::m_indices[index_offset]     = (vertices_count - 1);
                BaseMeshT::m_indices[index_offset + 1] = (vertices_count - 1) - (long_line_index + 2);
                BaseMeshT::m_indices[index_offset + 2] = (vertices_count - 1) - (long_line_index + 1);

                index_offset += 3;
            }

            BaseMeshT::m_indices[index_offset]     = (vertices_count - 1);
            BaseMeshT::m_indices[index_offset + 1] = (vertices_count - 2);
            BaseMeshT::m_indices[index_offset + 2] = (vertices_count - 1) - actual_long_lines_count;
        }
    }

    const float    m_radius;
    const uint32_t m_lat_lines_count;
    const uint32_t m_long_lines_count;
};

} // namespace Methane::Graphics
