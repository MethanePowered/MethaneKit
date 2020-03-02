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

FILE: Methane/Graphics/Mesh.h
Abstract mesh class

******************************************************************************/

#pragma once

#include "Methane/Graphics/MathTypes.h"
#include <Methane/Data/Types.h>

#include <vector>
#include <array>
#include <map>

namespace Methane::Graphics
{

class Mesh
{
public:
    using Position  = Vector3f;
    using Normal    = Vector3f;
    using Color     = Vector4f;
    using TexCoord  = Vector2f;
    using Index     = uint16_t;
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

        Subset(Type in_mesh_type, const Slice& in_vertices, const Slice& in_indices, bool in_indices_adjusted);
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

    class VertexLayout : public std::vector<VertexField>
    {
    public:
        using std::vector<VertexField>::vector;

        std::vector<std::string> GetSemantics() const;

        static std::string GetSemanticByVertexField(VertexField vertex_field);
    };

    Mesh(Type type, const VertexLayout& vertex_layout);

    Type                GetType() const noexcept            { return m_type; }
    const VertexLayout& GetVertexLayout() const noexcept    { return m_vertex_layout; }
    Data::Size          GetVertexSize() const noexcept      { return m_vertex_size; }
    const Indices&      GetIndices() const noexcept         { return m_indices; }
    const Index         GetIndex(uint32_t i) const noexcept { return i < m_indices.size() ? m_indices[i] : 0; }
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

} // namespace Methane::Graphics
