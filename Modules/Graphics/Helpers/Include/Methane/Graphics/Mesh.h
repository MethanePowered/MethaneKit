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

#include <Methane/Data/Types.h>
#include <Methane/Data/Instrumentation.h>

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
        Uber,
        Rect,
        Box,
        Sphere,
        Icosahedron,
    };

    struct Subset
    {
        struct Slice
        {
            const Data::Size offset;
            const Data::Size count;

            Slice(Data::Size in_offset, Data::Size in_count) : offset(in_offset), count(in_count) { }
            Slice(const Slice& other) = default;
        };

        const Type  mesh_type;
        const Slice vertices;
        const Slice indices;
        const bool  indices_adjusted;

        Subset(Type in_mesh_type, const Slice& in_vertices, const Slice& in_indices, bool in_indices_adjusted)
            : mesh_type(in_mesh_type)
            , vertices(in_vertices)
            , indices(in_indices)
            , indices_adjusted(in_indices_adjusted)
        {
            ITT_FUNCTION_TASK();
        }

        Subset(const Subset& other) = default;
    };

    using Subsets = std::vector<Subset>;

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
    Data::Size          GetVertexSize() const noexcept      { return m_vertex_size; }
    const Indices&      GetIndices() const noexcept         { return m_indices; }
    Data::Size          GetIndexCount() const noexcept      { return static_cast<Data::Size>(m_indices.size()); }
    Data::Size          GetIndexDataSize() const noexcept   { return static_cast<Data::Size>(m_indices.size() * sizeof(Index)); }

protected:
    struct Edge
    {
        const Mesh::Index first_index;
        const Mesh::Index second_index;
        
        Edge(Mesh::Index v1_index, Mesh::Index v2_index);
        
        bool operator<(const Edge& other) const;
    };
    
    using VertexFieldOffsets = std::array<int32_t,     static_cast<size_t>(VertexField::Count)>;
    using VertexFieldSizes   = std::array<Data::Size,  static_cast<size_t>(VertexField::Count)>;

    bool HasVertexField(VertexField field) const noexcept;

    static VertexFieldOffsets GetVertexFieldOffsets(const VertexLayout& vertex_layout);
    static Data::Size         GetVertexSize(const VertexLayout& vertex_layout) noexcept;

    const Type                  m_type;
    const VertexLayout          m_vertex_layout;
    const VertexFieldOffsets    m_vertex_field_offsets;
    const Data::Size            m_vertex_size;
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
    Index AddEdgeMidpoint(const Edge& edge, EdgeMidpoints& edge_midpoinds)
    {
        ITT_FUNCTION_TASK();
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

    Vertices m_vertices;
};

template<typename VType>
class UberMesh : public BaseMesh<VType>
{
public:
    using BaseMeshT = BaseMesh<VType>;

    UberMesh(const Mesh::VertexLayout& vertex_layout)
        : BaseMeshT(Mesh::Type::Uber, vertex_layout)
    {
        ITT_FUNCTION_TASK();
    }

    void AddSubMesh(const BaseMeshT& sub_mesh, bool adjust_indices)
    {
        ITT_FUNCTION_TASK();
        const typename BaseMeshT::Vertices& sub_vertices = sub_mesh.GetVertices();
        const Mesh::Indices& sub_indices = sub_mesh.GetIndices();

        m_subsets.emplace_back(sub_mesh.GetType(),
                               Mesh::Subset::Slice(static_cast<Data::Size>(BaseMeshT::m_vertices.size()), static_cast<Data::Size>(sub_vertices.size())),
                               Mesh::Subset::Slice(static_cast<Data::Size>(Mesh::m_indices.size()), static_cast<Data::Size>(sub_indices.size())),
                               adjust_indices);

        if (adjust_indices)
        {
            const Mesh::Index index_offset = static_cast<Mesh::Index>(BaseMeshT::GetVertexCount());
            std::transform(sub_indices.begin(), sub_indices.end(), std::back_inserter(Mesh::m_indices),
                           [index_offset](const Mesh::Index& index) { return index_offset + index; });
        }
        else
        {
            Mesh::m_indices.insert(Mesh::m_indices.end(), sub_indices.begin(), sub_indices.end());
        }

        BaseMeshT::m_vertices.insert(BaseMeshT::m_vertices.end(), sub_vertices.begin(), sub_vertices.end());
    }

    const Mesh::Subsets& GetSubsets() const                     { return m_subsets; }
    size_t               GetSubsetCount() const noexcept        { return m_subsets.size(); }
    const Mesh::Subset&  GetSubset(size_t subset_index) const
    {
        ITT_FUNCTION_TASK();
        if (subset_index >= m_subsets.size())
            throw std::invalid_argument("Sub mesh index is out of bounds.");

        return m_subsets[subset_index];
    }

    std::pair<const VType*, size_t> GetSubsetVertices(size_t subset_index) const
    {
        ITT_FUNCTION_TASK();
        const Mesh::Subset& subset = GetSubset(subset_index);
        return { BaseMeshT::GetVertices().data() + subset.vertices.offset, subset.vertices.count };
    }

    std::pair<const Mesh::Index*, size_t> GetSubsetIndices(size_t subset_index) const
    {
        ITT_FUNCTION_TASK();
        const Mesh::Subset& subset = GetSubset(subset_index);
        return { Mesh::GetIndices().data() + subset.indices.offset, subset.indices.count };
    }

private:
    Mesh::Subsets m_subsets;
};

template<typename VType>
class RectMesh : public BaseMesh<VType>
{
public:
    using BaseMeshT = BaseMesh<VType>;

    enum class FaceType
    {
        XY,
        XZ,
        YZ,
    };

    RectMesh(const Mesh::VertexLayout& vertex_layout, float width = 1.f, float height = 1.f, float depth_pos = 0.f, size_t color_index = 0, FaceType face_type = FaceType::XY, Mesh::Type type = Mesh::Type::Rect)
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
    using BaseMeshT = BaseMesh<VType>;
    using RectMeshT = RectMesh<VType>;

public:
    BoxMesh(const Mesh::VertexLayout& vertex_layout, float width = 1.f, float height = 1.f, float depth = 1.f)
        : RectMeshT(vertex_layout, width, height, depth / 2.f, 0, RectMeshT::FaceType::XY, Mesh::Type::Box)
        , m_depth(depth)
    {
        ITT_FUNCTION_TASK();
        AddFace(RectMeshT(vertex_layout, width,  height, -depth  / 2.f, 1, RectMeshT::FaceType::XY));
        AddFace(RectMeshT(vertex_layout, width,  depth,   height / 2.f, 2, RectMeshT::FaceType::XZ));
        AddFace(RectMeshT(vertex_layout, width,  depth,  -height / 2.f, 3, RectMeshT::FaceType::XZ));
        AddFace(RectMeshT(vertex_layout, height, depth,   width  / 2.f, 4, RectMeshT::FaceType::YZ));
        AddFace(RectMeshT(vertex_layout, height, depth,  -width  / 2.f, 5, RectMeshT::FaceType::YZ));
    }

    float GetDepth() const noexcept { return m_depth; }

protected:
    void AddFace(const RectMeshT& face_mesh) noexcept
    {
        ITT_FUNCTION_TASK();
        const size_t initial_vertices_count = BaseMeshT::m_vertices.size();

        const typename BaseMeshT::Vertices& face_vertices = face_mesh.GetVertices();
        BaseMeshT::m_vertices.insert(BaseMeshT::m_vertices.end(), face_vertices.begin(), face_vertices.end());

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
    using BaseMeshT = BaseMesh<VType>;
    
    SphereMesh(const Mesh::VertexLayout& vertex_layout, float radius = 1.f, uint32_t lat_lines_count = 10, uint32_t long_lines_count = 16)
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
            throw std::invalid_argument("Lattitude lines count should not be less than 3.");
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
        
        cml::matrix33f pitch_step_matrix = { }, yaw_step_matrix = { };
        cml::matrix_rotation_world_x(pitch_step_matrix, cml::constants<float>::pi() / (m_lat_lines_count - 1));
        cml::matrix_rotation_world_y(yaw_step_matrix, 2.0 * cml::constants<float>::pi() / m_long_lines_count);

        cml::matrix33f pitch_matrix = cml::identity_3x3();
        if (!has_texcoord)
            pitch_matrix = pitch_step_matrix;
        
        const uint32_t actual_lat_lines_count = has_texcoord ? m_lat_lines_count : m_lat_lines_count - 1;
        const uint32_t first_lat_line_index   = has_texcoord ? 0 : 1;
        const uint32_t first_vertex_index     = has_texcoord ? 0 : 1;

        for (uint32_t lat_line_index = first_lat_line_index; lat_line_index < actual_lat_lines_count; ++lat_line_index)
        {
            cml::matrix33f yaw_matrix = cml::identity_3x3();

            for(uint32_t long_line_index = 0; long_line_index < actual_long_lines_count; ++long_line_index)
            {
                const cml::matrix33f rotation_matrix = pitch_matrix * yaw_matrix;
                const uint32_t       vertex_index    = (lat_line_index - first_lat_line_index) * actual_long_lines_count + long_line_index + first_vertex_index;
                
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
        const uint32_t index_long_lines_count = has_texcoord ? m_long_lines_count + 1 : m_long_lines_count - 1;
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

template<typename VType>
class IcosahedronMesh : public BaseMesh<VType>
{
public:
    using BaseMeshT = BaseMesh<VType>;
    
    IcosahedronMesh(const Mesh::VertexLayout& vertex_layout, float radius = 1.f, uint32_t subdivisions_count = 0, bool spherify = false)
        : BaseMeshT(Mesh::Type::Icosahedron, vertex_layout)
        , m_radius(radius)
    {
        ITT_FUNCTION_TASK();

        const bool has_colors   = Mesh::HasVertexField(Mesh::VertexField::Color);
        const bool has_normals  = Mesh::HasVertexField(Mesh::VertexField::Normal);
        const bool has_texcoord = Mesh::HasVertexField(Mesh::VertexField::TexCoord);

        if (has_colors)
        {
            throw std::invalid_argument("Colored vertices are not supported for icosahedron mesh.");
        }
        
        const float a = (radius + std::sqrt(radius * 5.f)) / 2.f;
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
        
        BaseMeshT::m_vertices.resize(vertex_positions.size());
        for(size_t vertex_index = 0; vertex_index < vertex_positions.size(); ++vertex_index)
        {
            VType& vertex = BaseMeshT::m_vertices[vertex_index];
            
            Mesh::Position& vertex_position = BaseMeshT::template GetVertexField<Mesh::Position>(vertex, Mesh::VertexField::Position);
            vertex_position = vertex_positions[vertex_index];
            
            if (has_normals)
            {
                Mesh::Normal& vertex_normal = BaseMeshT::template GetVertexField<Mesh::Normal>(vertex, Mesh::VertexField::Normal);
                vertex_normal = cml::normalize(vertex_position);
            }
            
            if (has_texcoord)
            {
                Mesh::TexCoord& vertex_texcoord = BaseMeshT::template GetVertexField<Mesh::TexCoord>(vertex, Mesh::VertexField::TexCoord);
                const Mesh::Position vertex_direction = cml::normalize(vertex_position);

                vertex_texcoord[0] = std::atan2(vertex_direction[2], vertex_direction[0]) / (2.f * cml::constants<float>::pi()) + 0.5f;
                assert(0.f <= vertex_texcoord[0] && vertex_texcoord[0] <= 1.f);

                vertex_texcoord[1] = std::asin(vertex_direction[1]) / cml::constants<float>::pi() + 0.5f;
                assert(0.f <= vertex_texcoord[1] && vertex_texcoord[1] <= 1.f);
            }
        }

        BaseMeshT::m_indices = {
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
        ITT_FUNCTION_TASK();
        if (BaseMeshT::m_indices.size() % 3 != 0)
            throw std::logic_error("Icosahedron indices count should be a multiple of three representing triangles list.");

        Mesh::Indices new_indices;
        new_indices.reserve(BaseMeshT::m_indices.size() * 4);
        BaseMeshT::m_vertices.reserve(BaseMeshT::m_vertices.size() * 2);
        
        typename BaseMeshT::EdgeMidpoints edge_midpoints;

        const size_t triangles_count = BaseMeshT::m_indices.size() / 3;
        for (size_t triangle_index = 0; triangle_index < triangles_count; ++triangle_index)
        {
            const Mesh::Index vi1 = BaseMeshT::m_indices[triangle_index * 3];
            const Mesh::Index vi2 = BaseMeshT::m_indices[triangle_index * 3 + 1];
            const Mesh::Index vi3 = BaseMeshT::m_indices[triangle_index * 3 + 2];

            const Mesh::Index vm1 = BaseMeshT::AddEdgeMidpoint(Mesh::Edge(vi1, vi2), edge_midpoints);
            const Mesh::Index vm2 = BaseMeshT::AddEdgeMidpoint(Mesh::Edge(vi2, vi3), edge_midpoints);
            const Mesh::Index vm3 = BaseMeshT::AddEdgeMidpoint(Mesh::Edge(vi3, vi1), edge_midpoints);

            std::array<Mesh::Index, 3 * 4> indices = {
                vi1, vm1, vm3,
                vm1, vi2, vm2,
                vm1, vm2, vm3,
                vm3, vm2, vi3,
            };
            new_indices.insert(new_indices.end(), indices.begin(), indices.end());
        }

        std::swap(BaseMeshT::m_indices, new_indices);
    }
    
    void Spherify()
    {
        ITT_FUNCTION_TASK();

        const bool has_normals = Mesh::HasVertexField(Mesh::VertexField::Normal);

        for(VType& vertex : BaseMeshT::m_vertices)
        {
            Mesh::Position& vertex_position = BaseMeshT::template GetVertexField<Mesh::Position>(vertex, Mesh::VertexField::Position);
            vertex_position = cml::normalize(vertex_position) * m_radius;
            
            if (has_normals)
            {
                Mesh::Normal& vertex_normal = BaseMeshT::template GetVertexField<Mesh::Normal>(vertex, Mesh::VertexField::Normal);
                vertex_normal = cml::normalize(vertex_position);
            }
        }
    }
    
protected:
    const float m_radius;
};

} // namespace Methane::Graphics
