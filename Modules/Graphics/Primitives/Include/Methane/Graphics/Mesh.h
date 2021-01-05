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

FILE: Methane/Graphics/Mesh.h
Abstract mesh class

******************************************************************************/

#pragma once

#include <Methane/Data/Types.h>

#include <cml/vector.h>
#include <magic_enum.hpp>
#include <vector>
#include <array>
#include <map>

namespace Methane::Graphics
{

class Mesh
{
public:
    using Position   = cml::vector3f;
    using Position2D = cml::vector2f;
    using Normal     = cml::vector3f;
    using Color      = cml::vector4f;
    using TexCoord   = cml::vector2f;
    using Index      = uint16_t;
    using Indices    = std::vector<Index>;

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
        Color
    };

    class VertexLayout : public std::vector<VertexField>
    {
    public:
        class IncompatibleException : public std::logic_error
        {
        public:
            explicit IncompatibleException(VertexField missing_field);

            [[nodiscard]] VertexField GetMissingField() const noexcept { return m_missing_field; }

        private:
            const VertexField m_missing_field;
        };

        using std::vector<VertexField>::vector;

        [[nodiscard]] std::vector<std::string> GetSemantics() const;

        [[nodiscard]] static std::string GetSemanticByVertexField(VertexField vertex_field);
    };

    Mesh(Type type, const VertexLayout& vertex_layout);

    [[nodiscard]] Type                GetType() const noexcept               { return m_type; }
    [[nodiscard]] const VertexLayout& GetVertexLayout() const noexcept       { return m_vertex_layout; }
    [[nodiscard]] Data::Size          GetVertexSize() const noexcept         { return m_vertex_size; }
    [[nodiscard]] const Indices&      GetIndices() const noexcept            { return m_indices; }
    [[nodiscard]] Index               GetIndex(Data::Index i) const noexcept { return i < m_indices.size() ? m_indices[i] : 0; }
    [[nodiscard]] Data::Size          GetIndexCount() const noexcept         { return static_cast<Data::Size>(m_indices.size()); }
    [[nodiscard]] Data::Size          GetIndexDataSize() const noexcept      { return static_cast<Data::Size>(m_indices.size() * sizeof(Index)); }

protected:
    struct Edge
    {
        const Mesh::Index first_index;
        const Mesh::Index second_index;
        
        Edge(Mesh::Index v1_index, Mesh::Index v2_index);

        [[nodiscard]] bool operator<(const Edge& other) const;
    };
    
    using VertexFieldOffsets = std::array<int32_t, magic_enum::enum_count<VertexField>()>;

    void CheckLayoutHasVertexField(VertexField field) const;
    [[nodiscard]] bool HasVertexField(VertexField field) const noexcept;
    [[nodiscard]] int32_t GetVertexFieldOffset(VertexField field) const { return m_vertex_field_offsets[static_cast<size_t>(field)]; }

    void ResizeIndices(size_t indices_count)         { m_indices.resize(indices_count, 0); }
    void SetIndex(size_t index, Index vertex_index)  { m_indices[index] = vertex_index; }
    void SetIndices(Indices&& indices) noexcept      { m_indices = std::move(indices); }
    void SwapIndices(Indices& indices)               { m_indices.swap(indices); }
    void AppendIndices(const Mesh::Indices& indices) { m_indices.insert(m_indices.end(), indices.begin(), indices.end()); }
    auto GetIndicesBackInserter()                    { return std::back_inserter(m_indices); }

    [[nodiscard]] static VertexFieldOffsets GetVertexFieldOffsets(const VertexLayout& vertex_layout);
    [[nodiscard]] static Data::Size         GetVertexSize(const VertexLayout& vertex_layout) noexcept;
    [[nodiscard]] static Data::Size         GetVertexFieldSize(VertexField vertex_field)   { return GetVertexFieldSize(static_cast<size_t>(vertex_field)); }
    [[nodiscard]] static Data::Size         GetVertexFieldSize(size_t vertex_field_index);
    [[nodiscard]] static const Position2D&  GetFacePosition2D(size_t index);
    [[nodiscard]] static Data::Size         GetFacePositionCount() noexcept;
    [[nodiscard]] static const TexCoord&    GetFaceTexCoord(size_t index);
    [[nodiscard]] static Mesh::Index        GetFaceIndex(size_t index);
    [[nodiscard]] static Data::Size         GetFaceIndicesCount() noexcept;
    [[nodiscard]] static const Color&       GetColor(size_t index);
    [[nodiscard]] static Data::Size         GetColorsCount() noexcept;

private:
    const Type               m_type;
    const VertexLayout       m_vertex_layout;
    const VertexFieldOffsets m_vertex_field_offsets;
    const Data::Size         m_vertex_size;
    Indices                  m_indices;
};

} // namespace Methane::Graphics
