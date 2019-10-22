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

#include <Methane/Instrumentation.h>

#include <vector>
#include <array>
#include <map>
#include <algorithm>
#include <cassert>

#include <cml/mathlib/mathlib.h>

namespace Methane::Graphics
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
        Unknown,
        Rect,
        Box,
        Sphere,
        Icosahedron,
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
        ITT_FUNCTION_TASK();
        return VertexLayout(layout_array.begin(), layout_array.end());
    }

    Mesh(Type type, const VertexLayout& vertex_layout);

    Type                GetType() const noexcept            { return m_type; }
    const VertexLayout& GetVertexLayout() const noexcept    { return m_vertex_layout; }
    size_t              GetVertexSize() const noexcept      { return m_vertex_size; }
    const Indices&      GetIndices() const noexcept         { return m_indices; }
    size_t              GetIndexDataSize() const noexcept   { return m_indices.size() * sizeof(Index); }

protected:
    struct Edge
    {
        const Mesh::Index first_index;
        const Mesh::Index second_index;
        
        Edge(Mesh::Index v1_index, Mesh::Index v2_index);
        
        bool operator<(const Edge& other) const;
    };
    
    using VertexFieldOffsets = std::array<int32_t, static_cast<size_t>(VertexField::Count)>;
    using VertexFieldSizes   = std::array<size_t,  static_cast<size_t>(VertexField::Count)>;

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
        ITT_FUNCTION_TASK();
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
    Index AddEdgeMidpoint(const Edge& edge, EdgeMidpoints& edge_midpoinds)
    {
        const auto edge_midpoint_it = edge_midpoinds.find(edge);
        if (edge_midpoint_it != edge_midpoinds.end())
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
        edge_midpoinds.emplace(edge, v_mid_index);
        m_vertices.push_back(v_mid);
        return v_mid_index;
    }
    
    void ComputeAverageNormals()
    {
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
        ITT_FUNCTION_TASK();
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
        ITT_FUNCTION_TASK();
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
        ITT_FUNCTION_TASK();
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
    
template<typename VType>
class SphereMesh : public BaseMesh<VType>
{
public:
    using BaseMesh = BaseMesh<VType>;
    
    SphereMesh(const Mesh::VertexLayout& vertex_layout, float radius = 1.f, uint32_t lat_lines_count = 10, uint32_t long_lines_count = 16)
        : BaseMesh(Mesh::Type::Sphere, vertex_layout)
        , m_radius(radius)
    {
        ITT_FUNCTION_TASK();

        if (Mesh::HasVertexField(Mesh::VertexField::Color))
        {
            throw std::invalid_argument("Colored vertices are not supported for sphere mesh.");
        }
        if (Mesh::HasVertexField(Mesh::VertexField::TexCoord))
        {
            throw std::invalid_argument("Textured vertices are not supported for sphere mesh.");
        }
        if (lat_lines_count < 3)
        {
            throw std::invalid_argument("Lattitude lines count should not be less than 3.");
        }
        if (long_lines_count < 3)
        {
            throw std::invalid_argument("Longitude lines count should not be less than 3.");
        }
        
        // Generate sphere vertices
        
        BaseMesh::m_vertices.resize((lat_lines_count - 2) * long_lines_count + 2, {});
        
        Mesh::Position& first_vertex_position = BaseMesh::template GetVertexField<Mesh::Position>(BaseMesh::m_vertices.front(), Mesh::VertexField::Position);
        Mesh::Position& last_vertex_position = BaseMesh::template GetVertexField<Mesh::Position>(BaseMesh::m_vertices.back(), Mesh::VertexField::Position);
        
        first_vertex_position = Mesh::Position(0.f, m_radius, 0.f);
        last_vertex_position  = Mesh::Position(0.f, -m_radius, 0.f);
        
        if (Mesh::HasVertexField(Mesh::VertexField::Normal))
        {
            Mesh::Position& first_vertex_normal = BaseMesh::template GetVertexField<Mesh::Normal>(BaseMesh::m_vertices.front(), Mesh::VertexField::Normal);
            Mesh::Position& last_vertex_normal = BaseMesh::template GetVertexField<Mesh::Normal>(BaseMesh::m_vertices.back(), Mesh::VertexField::Normal);
        
            first_vertex_normal = Mesh::Position(0.f,  1.f, 0.f);
            last_vertex_normal  = Mesh::Position(0.f, -1.f, 0.f);
        }
        
        cml::matrix33f pitch_step_matrix = { }, yaw_step_matrix = { };
        cml::matrix_rotation_world_x(pitch_step_matrix, cml::constants<float>::pi() / (lat_lines_count - 1));
        cml::matrix_rotation_world_y(yaw_step_matrix, 2.0 * cml::constants<float>::pi() / long_lines_count);
        
        cml::matrix33f pitch_matrix = cml::identity_3x3(), yaw_matrix = cml::identity_3x3();
        for (uint32_t lat_line_index = 1; lat_line_index < lat_lines_count - 1; ++lat_line_index)
        {
            pitch_matrix = pitch_matrix * pitch_step_matrix;
            
            for(uint32_t long_line_index = 0; long_line_index < long_lines_count; ++long_line_index)
            {
                const cml::matrix33f rotation_matrix = pitch_matrix * yaw_matrix;
                const uint32_t  vertex_index = (lat_line_index - 1) * long_lines_count + long_line_index + 1;
                
                VType& vertex = BaseMesh::m_vertices[vertex_index];
                {
                    Mesh::Position& vertex_position = BaseMesh::template GetVertexField<Mesh::Position>(vertex, Mesh::VertexField::Position);
                    vertex_position = Mesh::Position(0.f, m_radius, 0.f) * rotation_matrix;
                }
                if (Mesh::HasVertexField(Mesh::VertexField::Normal))
                {
                    Mesh::Normal& vertex_normal = BaseMesh::template GetVertexField<Mesh::Normal>(vertex, Mesh::VertexField::Normal);
                    vertex_normal = Mesh::Normal(0.f, 1.f, 0.f) * rotation_matrix;
                }
                
                yaw_matrix = yaw_matrix * yaw_step_matrix;
            }
        }
        
        // Generate sphere indices
        
        const uint32_t sphere_faces_count = (lat_lines_count - 2) * long_lines_count * 2;
        BaseMesh::m_indices.resize(sphere_faces_count * 3, 0);
        
        uint32_t index_offset = 0;
        for(Mesh::Index long_line_index = 0; long_line_index < long_lines_count - 1; ++long_line_index)
        {
            BaseMesh::m_indices[index_offset]     = 0;
            BaseMesh::m_indices[index_offset + 1] = long_line_index + 2;
            BaseMesh::m_indices[index_offset + 2] = long_line_index + 1;
            index_offset += 3;
        }

        BaseMesh::m_indices[index_offset]     = 0;
        BaseMesh::m_indices[index_offset + 1] = 1;
        BaseMesh::m_indices[index_offset + 2] = long_lines_count;
        
        index_offset += 3;

        const uint32_t vertices_count = static_cast<uint32_t>(BaseMesh::m_vertices.size());
        for (uint32_t lat_line_index = 0; lat_line_index < lat_lines_count - 3; ++lat_line_index)
        {
            for(uint32_t long_line_index = 0; long_line_index < long_lines_count - 1; ++long_line_index)
            {
                BaseMesh::m_indices[index_offset]     = lat_line_index * long_lines_count + long_line_index + 1;
                BaseMesh::m_indices[index_offset + 1] = lat_line_index * long_lines_count + long_line_index + 2;
                BaseMesh::m_indices[index_offset + 2] = (lat_line_index + 1) * long_lines_count + long_line_index + 1;

                BaseMesh::m_indices[index_offset + 3] = (lat_line_index + 1) * long_lines_count + long_line_index + 1;
                BaseMesh::m_indices[index_offset + 4] = lat_line_index * long_lines_count + long_line_index + 2;
                BaseMesh::m_indices[index_offset + 5] = (lat_line_index + 1) * long_lines_count + long_line_index + 2;

                index_offset += 6;
            }

            BaseMesh::m_indices[index_offset]     = (lat_line_index * long_lines_count) + long_lines_count;
            BaseMesh::m_indices[index_offset + 1] = (lat_line_index * long_lines_count) + 1;
            BaseMesh::m_indices[index_offset + 2] = ((lat_line_index + 1) * long_lines_count) + long_lines_count;

            BaseMesh::m_indices[index_offset + 3] = ((lat_line_index + 1) * long_lines_count) + long_lines_count;
            BaseMesh::m_indices[index_offset + 4] = (lat_line_index * long_lines_count) + 1;
            BaseMesh::m_indices[index_offset + 5] = ((lat_line_index + 1) * long_lines_count) + 1;

            index_offset += 6;
        }

        for(uint32_t long_line_index = 0; long_line_index < long_lines_count - 1; ++long_line_index)
        {
            BaseMesh::m_indices[index_offset]     = (vertices_count - 1);
            BaseMesh::m_indices[index_offset + 1] = (vertices_count - 1) - (long_line_index + 2);
            BaseMesh::m_indices[index_offset + 2] = (vertices_count - 1) - (long_line_index + 1);

            index_offset += 3;
        }

        BaseMesh::m_indices[index_offset]     = (vertices_count - 1);
        BaseMesh::m_indices[index_offset + 1] = (vertices_count - 2);
        BaseMesh::m_indices[index_offset + 2] = (vertices_count - 1) - long_lines_count;
    }
    
    const float GetRadius() const noexcept { return m_radius; }
    
protected:
    const float m_radius;
};

template<typename VType>
class IcosahedronMesh : public BaseMesh<VType>
{
public:
    using BaseMesh = BaseMesh<VType>;
    
    IcosahedronMesh(const Mesh::VertexLayout& vertex_layout, float radius = 1.f, uint32_t subdivisions_count = 0, bool spherify = false)
        : BaseMesh(Mesh::Type::Icosahedron, vertex_layout)
        , m_radius(radius)
    {
        if (Mesh::HasVertexField(Mesh::VertexField::Color))
        {
            throw std::invalid_argument("Colored vertices are not supported for icosahedron mesh.");
        }
        
        const float a = (radius + std::sqrtf(radius * 5.f)) / 2.f;
        const float b = radius;
        const std::array<Mesh::Position, 12> vertex_positions = {{
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
        }};
        
        BaseMesh::m_vertices.resize(vertex_positions.size());
        for(size_t vertex_index = 0; vertex_index < vertex_positions.size(); ++vertex_index)
        {
            VType& vertex = BaseMesh::m_vertices[vertex_index];
            
            Mesh::Position& vertex_position = BaseMesh::template GetVertexField<Mesh::Position>(vertex, Mesh::VertexField::Position);
            vertex_position = vertex_positions[vertex_index];
            
            if (Mesh::HasVertexField(Mesh::VertexField::Normal))
            {
                Mesh::Normal& vertex_normal = BaseMesh::template GetVertexField<Mesh::Normal>(vertex, Mesh::VertexField::Normal);
                vertex_normal = cml::normalize(vertex_position);
            }
            
            if (Mesh::HasVertexField(Mesh::VertexField::TexCoord))
            {
                Mesh::TexCoord& vertex_texcoord = BaseMesh::template GetVertexField<Mesh::TexCoord>(vertex, Mesh::VertexField::TexCoord);
                const Mesh::Position vertex_direction = cml::normalize(vertex_position);

                vertex_texcoord[0] = std::atan2(vertex_direction[2], vertex_direction[0]) / (2.f * cml::constants<float>::pi()) + 0.5f;
                assert(0.f <= vertex_texcoord[0] && vertex_texcoord[0] <= 1.f);

                vertex_texcoord[1] = std::asin(vertex_direction[1]) / cml::constants<float>::pi() + 0.5f;
                assert(0.f <= vertex_texcoord[1] && vertex_texcoord[1] <= 1.f);
            }
        }

        BaseMesh::m_indices = {
             5,  0, 11,
             1,  0,  5,
             7,  0,  1,
            10,  0,  7,
            11,  0, 10,
             9,  1,  5,
             4,  5, 11,
             2, 11, 10,
             6, 10,  7,
             8,  7,  1,
             4,  3,  9,
             2,  3,  4,
             6,  3,  2,
             8,  3,  6,
             9,  3,  8,
             5,  4,  9,
            11,  2,  4,
            10,  6,  2,
             7,  8,  6,
             1,  9,  8,
        };
        
        for(uint32_t subdivision = 0; subdivision < subdivisions_count; ++subdivision)
        {
            Subdivide();
        }
        
        if (spherify)
        {
            Spherify();
        }
    }
    
    const float GetRadius() const noexcept  { return m_radius; }
    
    void Subdivide()
    {
        if (BaseMesh::m_indices.size() % 3 != 0)
            throw std::logic_error("Icosahedron indices count should be a multiple of three representing triangles list.");

        Mesh::Indices new_indices;
        new_indices.reserve(BaseMesh::m_indices.size() * 4);
        BaseMesh::m_vertices.reserve(BaseMesh::m_vertices.size() * 2);
        
        typename BaseMesh::EdgeMidpoints edge_midpoints;

        const size_t triangles_count = BaseMesh::m_indices.size() / 3;
        for (size_t triangle_index = 0; triangle_index < triangles_count; ++triangle_index)
        {
            const Mesh::Index vi1 = BaseMesh::m_indices[triangle_index * 3];
            const Mesh::Index vi2 = BaseMesh::m_indices[triangle_index * 3 + 1];
            const Mesh::Index vi3 = BaseMesh::m_indices[triangle_index * 3 + 2];

            const Mesh::Index vm1 = BaseMesh::AddEdgeMidpoint(Mesh::Edge(vi1, vi2), edge_midpoints);
            const Mesh::Index vm2 = BaseMesh::AddEdgeMidpoint(Mesh::Edge(vi2, vi3), edge_midpoints);
            const Mesh::Index vm3 = BaseMesh::AddEdgeMidpoint(Mesh::Edge(vi3, vi1), edge_midpoints);

            std::array<Mesh::Index, 3 * 4> indices = {
                vi1, vm1, vm3,
                vm1, vi2, vm2,
                vm1, vm2, vm3,
                vm3, vm2, vi3,
            };
            new_indices.insert(new_indices.end(), indices.begin(), indices.end());
        }

        std::swap(BaseMesh::m_indices, new_indices);
    }
    
    void Spherify()
    {
        for(VType& vertex : BaseMesh::m_vertices)
        {
            Mesh::Position& vertex_position = BaseMesh::template GetVertexField<Mesh::Position>(vertex, Mesh::VertexField::Position);
            vertex_position = cml::normalize(vertex_position) * m_radius;
            
            if (Mesh::HasVertexField(Mesh::VertexField::Normal))
            {
                Mesh::Normal& vertex_normal = BaseMesh::template GetVertexField<Mesh::Normal>(vertex, Mesh::VertexField::Normal);
                vertex_normal = cml::normalize(vertex_position);
            }
        }
    }
    
protected:
    const float m_radius;
};

} // namespace Methane::Graphics
