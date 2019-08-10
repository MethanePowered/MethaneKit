/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Mesh.h
Procedural mesh generators, including rect, box, etc.

******************************************************************************/

#pragma once

#include "MathTypes.h"

#include <vector>
#include <array>
#include <algorithm>
#include <cassert>

namespace Methane
{
namespace Graphics
{

class Mesh
{
public:
    using Position  = Vector3f;
    using Normal    = Vector3f;
    using Color     = Vector4f;
    using TexCoord  = Vector2f;
    using Index     = uint32_t;
    using Indices   = std::vector<Index>;

    enum class Type
    {
        Rect,
        Box,
    };

    enum class VertexField : size_t
    {
        Position = 0,
        Normal,
        TexCoord,
        Color,

        Count
    };

    using VertexLayout = std::vector<VertexField>;

    template<std::size_t N>
    static VertexLayout VertexLayoutFromArray(const std::array<VertexField, N>& layout_array)
    {
        return VertexLayout(layout_array.begin(), layout_array.end());
    }

    Mesh(Type type, const VertexLayout& vertex_layout);

    Type                GetType() const noexcept            { return m_type; }
    const VertexLayout& GetVertexLayout() const noexcept    { return m_vertex_layout; }
    size_t              GetVertexSize() const noexcept      { return m_vertex_size; }
    const Indices&      GetIndices() const noexcept         { return m_indices; }
    size_t              GetIndexDataSize() const noexcept   { return m_indices.size() * sizeof(Index); }

protected:
    using VertexFieldOffsets = std::array<int32_t, static_cast<size_t>(VertexField::Count)>;
    using VertexFieldSizes   = std::array<size_t, static_cast<size_t>(VertexField::Count)>;

    bool HasVertexField(VertexField field) const noexcept;

    static VertexFieldOffsets GetVertexFieldOffsets(const VertexLayout& vertex_layout);
    static size_t             GetVertexSize(const VertexLayout& vertex_layout) noexcept;

    const Type                  m_type;
    const VertexLayout          m_vertex_layout;
    const VertexFieldOffsets    m_vertex_field_offsets;
    const size_t                m_vertex_size;
    Indices                     m_indices;

    using Position2D  = Vector2f;
    using Positions2D = std::vector<Position2D>;
    using TexCoords   = std::vector<TexCoord>;
    using Colors      = std::vector<Color>;

    static const VertexFieldSizes   g_vertex_field_sizes;
    static const Positions2D        g_face_positions_2d;
    static const TexCoords          g_face_texcoords;
    static const Indices            g_face_indices;
    static const Colors             g_colors;
};

template<typename VType>
class BaseMesh : public Mesh
{
public:
    using Vertices = std::vector<VType>;

    BaseMesh(Type type, const VertexLayout& vertex_layout)
        : Mesh(type, vertex_layout)
    {
        if (sizeof(VType) != m_vertex_size)
        {
            throw std::invalid_argument("Size of vertex structure differs from vertex size calculated by vertex layout.");
        }
    }

    const Vertices& GetVertices() const noexcept       { return m_vertices; }
    size_t          GetVertexDataSize() const noexcept { return m_vertices.size() * m_vertex_size; }

protected:
    template<typename FType>
    FType& GetVertexField(VType& vertex, VertexField field) noexcept
    {
        const int32_t field_offset = m_vertex_field_offsets[static_cast<size_t>(field)];
        assert(field_offset >= 0);
        return *reinterpret_cast<FType*>(reinterpret_cast<char*>(&vertex) + field_offset);
    }

    Vertices m_vertices;
};

template<typename VType>
class RectMesh : public BaseMesh<VType>
{
public:
    enum class FaceType
    {
        XY,
        XZ,
        YZ,
    };

    RectMesh(const Mesh::VertexLayout& vertex_layout, float width = 1.f, float height = 1.f, float depth_pos = 0.f, size_t color_index = 0, FaceType face_type = FaceType::XY, Mesh::Type type = Mesh::Type::Rect)
        : BaseMesh<VType>(type, vertex_layout)
        , m_width(width)
        , m_height(height)
        , m_depth_pos(depth_pos)
    {
        for (size_t face_vertex_idx = 0; face_vertex_idx < Mesh::g_face_positions_2d.size(); ++face_vertex_idx)
        {
            VType vertex = {};
            {
                const Mesh::Position2D& pos_2d = Mesh::g_face_positions_2d[face_vertex_idx];
                Mesh::Position& vertex_position = BaseMesh<VType>::template GetVertexField<Mesh::Position>(vertex, Mesh::VertexField::Position);
                switch (face_type)
                {
                case FaceType::XY: vertex_position = Mesh::Position(pos_2d[0] * m_width, pos_2d[1] * m_height, m_depth_pos); break;
                case FaceType::XZ: vertex_position = Mesh::Position(pos_2d[0] * m_width, m_depth_pos, pos_2d[1] * m_height); break;
                case FaceType::YZ: vertex_position = Mesh::Position(m_depth_pos, pos_2d[1] * m_width, pos_2d[0] * m_height); break;
                }
            }
            if (Mesh::HasVertexField(Mesh::VertexField::Normal))
            {
                Mesh::Normal& vertex_normal = BaseMesh<VType>::template GetVertexField<Mesh::Normal>(vertex, Mesh::VertexField::Normal);
                const float depth_norm = m_depth_pos ? m_depth_pos / std::abs(m_depth_pos) : 1.f;
                switch (face_type)
                {
                    case FaceType::XY: vertex_normal = Mesh::Normal(0.f, 0.f, depth_norm); break;
                    case FaceType::XZ: vertex_normal = Mesh::Normal(0.f, depth_norm, 0.f); break;
                    case FaceType::YZ: vertex_normal = Mesh::Normal(depth_norm, 0.f, 0.f); break;
                }
            }
            if (Mesh::HasVertexField(Mesh::VertexField::Color))
            {
                Mesh::Color& vertex_color = BaseMesh<VType>::template GetVertexField<Mesh::Color>(vertex, Mesh::VertexField::Color);
                vertex_color = Mesh::g_colors[color_index % Mesh::g_colors.size()];
            }
            if (Mesh::HasVertexField(Mesh::VertexField::TexCoord))
            {
                Mesh::TexCoord& vertex_texcoord = BaseMesh<VType>::template GetVertexField<Mesh::TexCoord>(vertex, Mesh::VertexField::TexCoord);
                vertex_texcoord = Mesh::g_face_texcoords[face_vertex_idx];
            }
            RectMesh::m_vertices.push_back(vertex);
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

template<typename VType>
class BoxMesh : public RectMesh<VType>
{
    using Positions = std::vector<Mesh::Position>;

public:
    BoxMesh(const Mesh::VertexLayout& vertex_layout, float width = 1.f, float height = 1.f, float depth = 1.f)
        : RectMesh<VType>(vertex_layout, width, height, depth / 2.f, 0, RectMesh<VType>::FaceType::XY, Mesh::Type::Box)
        , m_depth(depth)
    {
        AddFace(RectMesh<VType>(vertex_layout, width,  height, -depth  / 2.f, 1, RectMesh<VType>::FaceType::XY));
        AddFace(RectMesh<VType>(vertex_layout, width,  depth,   height / 2.f, 2, RectMesh<VType>::FaceType::XZ));
        AddFace(RectMesh<VType>(vertex_layout, width,  depth,  -height / 2.f, 3, RectMesh<VType>::FaceType::XZ));
        AddFace(RectMesh<VType>(vertex_layout, height, depth,   width  / 2.f, 4, RectMesh<VType>::FaceType::YZ));
        AddFace(RectMesh<VType>(vertex_layout, height, depth,  -width  / 2.f, 5, RectMesh<VType>::FaceType::YZ));
    }

    float GetDepth() const noexcept { return m_depth; }

protected:
    void AddFace(const RectMesh<VType>& face_mesh) noexcept
    {
        const size_t initial_vertices_count = BaseMesh<VType>::m_vertices.size();

        const typename BaseMesh<VType>::Vertices& face_vertices = face_mesh.GetVertices();
        BoxMesh::m_vertices.insert(BaseMesh<VType>::m_vertices.end(), face_vertices.begin(), face_vertices.end());

        const Mesh::Indices& face_indices = face_mesh.GetIndices();
        std::transform(face_indices.begin(), face_indices.end(), std::back_inserter(Mesh::m_indices),
                       [initial_vertices_count](const Mesh::Index& index)
                       { return static_cast<Mesh::Index>(initial_vertices_count + index); });
    }

    const float m_depth;
};

} // namespace Graphics
} // namespace Methane
