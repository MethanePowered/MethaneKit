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

FILE: Methane/Graphics/QuadMesh.hpp
Quad mesh generator with customizable vertex type

******************************************************************************/

#pragma once

#include "BaseMesh.hpp"

namespace Methane::Graphics
{

template<typename VType>
class QuadMesh : public BaseMesh<VType>
{
public:
    using BaseMeshT = BaseMesh<VType>;

    enum class FaceType
    {
        XY,
        XZ,
        YZ,
    };

    explicit QuadMesh(const Mesh::VertexLayout& vertex_layout, float width = 1.f, float height = 1.f, float depth_pos = 0.f, size_t color_index = 0, FaceType face_type = FaceType::XY, Mesh::Type type = Mesh::Type::Rect)
        : BaseMeshT(type, vertex_layout)
        , m_width(width)
        , m_height(height)
        , m_depth_pos(depth_pos)
    {
        ITT_FUNCTION_TASK();

        const bool has_colors   = Mesh::HasVertexField(Mesh::VertexField::Color);
        const bool has_normals  = Mesh::HasVertexField(Mesh::VertexField::Normal);
        const bool has_texcoord = Mesh::HasVertexField(Mesh::VertexField::TexCoord);

        for (size_t face_vertex_idx = 0; face_vertex_idx < Mesh::g_face_positions_2d.size(); ++face_vertex_idx)
        {
            VType vertex = {};
            {
                const Mesh::Position2D& pos_2d = Mesh::g_face_positions_2d[face_vertex_idx];
                Mesh::Position& vertex_position = BaseMeshT::template GetVertexField<Mesh::Position>(vertex, Mesh::VertexField::Position);
                switch (face_type)
                {
                case FaceType::XY: vertex_position = Mesh::Position(pos_2d[0] * m_width, pos_2d[1] * m_height, m_depth_pos); break;
                case FaceType::XZ: vertex_position = Mesh::Position(pos_2d[0] * m_width, m_depth_pos, pos_2d[1] * m_height); break;
                case FaceType::YZ: vertex_position = Mesh::Position(m_depth_pos, pos_2d[1] * m_width, pos_2d[0] * m_height); break;
                }
            }
            if (has_normals)
            {
                Mesh::Normal& vertex_normal = BaseMeshT::template GetVertexField<Mesh::Normal>(vertex, Mesh::VertexField::Normal);
                const float depth_norm = m_depth_pos ? m_depth_pos / std::abs(m_depth_pos) : 1.f;
                switch (face_type)
                {
                case FaceType::XY: vertex_normal = Mesh::Normal(0.f, 0.f, depth_norm); break;
                case FaceType::XZ: vertex_normal = Mesh::Normal(0.f, depth_norm, 0.f); break;
                case FaceType::YZ: vertex_normal = Mesh::Normal(depth_norm, 0.f, 0.f); break;
                }
            }
            if (has_colors)
            {
                Mesh::Color& vertex_color = BaseMeshT::template GetVertexField<Mesh::Color>(vertex, Mesh::VertexField::Color);
                vertex_color = Mesh::g_colors[color_index % Mesh::g_colors.size()];
            }
            if (has_texcoord)
            {
                Mesh::TexCoord& vertex_texcoord = BaseMeshT::template GetVertexField<Mesh::TexCoord>(vertex, Mesh::VertexField::TexCoord);
                vertex_texcoord = Mesh::g_face_texcoords[face_vertex_idx];
            }
            QuadMesh::m_vertices.push_back(vertex);
        }

        Mesh::m_indices = Mesh::g_face_indices;
        if ( (g_axis_orientation == cml::AxisOrientation::left_handed && ((face_type == FaceType::XY && m_depth_pos >= 0) || ((face_type == FaceType::XZ || face_type == FaceType::YZ) && m_depth_pos < 0))) ||
             (g_axis_orientation == cml::AxisOrientation::right_handed && ((face_type == FaceType::XY && m_depth_pos < 0) || ((face_type == FaceType::XZ || face_type == FaceType::YZ) && m_depth_pos >= 0))) )
        {
            std::reverse(Mesh::m_indices.begin(), Mesh::m_indices.end());
        }
    }

    const float GetWidth() const noexcept    { return m_width; }
    const float GetHeight() const noexcept   { return m_height; }
    const float GetDepthPos() const noexcept { return m_depth_pos; }

protected:
    const float m_width;
    const float m_height;
    const float m_depth_pos;
};

} // namespace Methane::Graphics
