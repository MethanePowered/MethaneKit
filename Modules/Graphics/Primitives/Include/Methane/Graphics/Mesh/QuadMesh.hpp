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

    explicit QuadMesh(const Mesh::VertexLayout& vertex_layout,
                      float width = 1.F, float height = 1.F, float depth_pos = 0.F, size_t color_index = 0,
                      FaceType face_type = FaceType::XY, Mesh::Type type = Mesh::Type::Rect)
        : BaseMeshT(type, vertex_layout)
        , m_width(width)
        , m_height(height)
        , m_depth_pos(depth_pos)
    {
        META_FUNCTION_TASK();

        const bool has_colors   = Mesh::HasVertexField(Mesh::VertexField::Color);
        const bool has_normals  = Mesh::HasVertexField(Mesh::VertexField::Normal);
        const bool has_texcoord = Mesh::HasVertexField(Mesh::VertexField::TexCoord);

        for (size_t face_vertex_idx = 0; face_vertex_idx < Mesh::Mesh::GetFacePositionCount(); ++face_vertex_idx)
        {
            VType vertex{};
            InitVertexPosition(face_type, face_vertex_idx, vertex);

            if (has_normals)
                InitVertexNormal(face_type, vertex);

            if (has_colors)
                InitVertexColor(color_index, vertex);

            if (has_texcoord)
                InitVertexTexCoord(face_vertex_idx, vertex);

            BaseMeshT::AddVertex(std::move(vertex));
        }

#if defined(HLSLPP_COORDINATES) && HLSLPP_COORDINATES == 0 // HLSLPP_COORDINATES_LEFT_HANDED
        const bool reverse_indices = (face_type == FaceType::XY && m_depth_pos >= 0) ||
                                     ((face_type == FaceType::XZ || face_type == FaceType::YZ) && m_depth_pos < 0);
#else
        const bool reverse_indices = (face_type == FaceType::XY && m_depth_pos < 0) ||
                                     ((face_type == FaceType::XZ || face_type == FaceType::YZ) && m_depth_pos >= 0);
#endif

        const size_t face_indices_count = Mesh::GetFaceIndicesCount();
        Mesh::ResizeIndices(face_indices_count);
        for(size_t index = 0; index < face_indices_count; ++index)
        {
            Mesh::SetIndex(reverse_indices ? face_indices_count - index - 1 : index, Mesh::GetFaceIndex(index));
        }
    }

    float GetWidth() const noexcept    { return m_width; }
    float GetHeight() const noexcept   { return m_height; }
    float GetDepthPos() const noexcept { return m_depth_pos; }

private:
    void InitVertexPosition(const FaceType& face_type, size_t face_vertex_idx, VType& vertex)
    {
        const Mesh::Position2D& pos_2d = Mesh::GetFacePosition2D(face_vertex_idx);
        Mesh::Position& vertex_position = BaseMeshT::template GetVertexField<Mesh::Position>(vertex, Mesh::VertexField::Position);
        switch (face_type)
        {
        case FaceType::XY: vertex_position = Mesh::Position(pos_2d[0] * m_width, pos_2d[1] * m_height, m_depth_pos); break;
        case FaceType::XZ: vertex_position = Mesh::Position(pos_2d[0] * m_width, m_depth_pos, pos_2d[1] * m_height); break;
        case FaceType::YZ: vertex_position = Mesh::Position(m_depth_pos, pos_2d[1] * m_width, pos_2d[0] * m_height); break;
        default:           META_UNEXPECTED_ENUM_ARG(face_type);
        }
    }

    void InitVertexNormal(const FaceType& face_type, VType& vertex)
    {
        Mesh::Normal& vertex_normal = BaseMeshT::template GetVertexField<Mesh::Normal>(vertex, Mesh::VertexField::Normal);
        const float depth_norm      = m_depth_pos ? m_depth_pos / abs(m_depth_pos) : 1.F;
        switch (face_type)
        {
        case FaceType::XY: vertex_normal = Mesh::Normal(0.F, 0.F, depth_norm); break;
        case FaceType::XZ: vertex_normal = Mesh::Normal(0.F, depth_norm, 0.F); break;
        case FaceType::YZ: vertex_normal = Mesh::Normal(depth_norm, 0.F, 0.F); break;
        default:           META_UNEXPECTED_ENUM_ARG(face_type);
        }
    }

    void InitVertexColor(size_t color_index, VType& vertex)
    {
        Mesh::Color& vertex_color = BaseMeshT::template GetVertexField<Mesh::Color>(vertex, Mesh::VertexField::Color);
        vertex_color = Mesh::GetColor(color_index % Mesh::GetColorsCount());
    }

    void InitVertexTexCoord(size_t face_vertex_idx, VType& vertex)
    {
        Mesh::TexCoord& vertex_texcoord = BaseMeshT::template GetVertexField<Mesh::TexCoord>(vertex, Mesh::VertexField::TexCoord);
        vertex_texcoord = Mesh::GetFaceTexCoord(face_vertex_idx);
    }

    const float m_width;
    const float m_height;
    const float m_depth_pos;
};

} // namespace Methane::Graphics
