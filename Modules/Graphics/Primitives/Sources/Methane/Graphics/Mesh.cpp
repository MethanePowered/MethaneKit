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

FILE: Methane/Graphics/Mesh.cpp
Abstract mesh class

******************************************************************************/

#include <Methane/Graphics/Mesh.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <cml/mathlib/mathlib.h>
#include <array>

namespace Methane::Graphics
{

static constexpr Data::Size g_face_positions_count = 4;
static constexpr Data::Size g_face_indices_count = 6;
static constexpr Data::Size g_colors_count = 6;

Data::Size Mesh::GetVertexFieldSize(size_t vertex_field_index)
{
    static const std::array<Data::Size, static_cast<size_t>(VertexField::Count)> s_vertex_field_sizes {{
        sizeof(Position),
        sizeof(Normal),
        sizeof(TexCoord),
        sizeof(Color),
    }};
    return s_vertex_field_sizes[vertex_field_index];
}

const Mesh::Position2D& Mesh::GetFacePosition2D(size_t index)
{
    // Quad vertices in clockwise order
    static const std::array<Mesh::Position2D, 4> s_face_positions_2d {{
        { -0.5F, -0.5F },
        { -0.5F,  0.5F },
        {  0.5F,  0.5F },
        {  0.5F, -0.5F }
    }};
    return s_face_positions_2d[index];
}

Data::Size Mesh::GetFacePositionCount() noexcept
{
    return g_face_positions_count;
}

const Mesh::TexCoord& Mesh::GetFaceTexCoord(size_t index)
{
    // Quad texture-coords for the vertices above
    static const std::array<Mesh::TexCoord, g_face_positions_count> s_face_texcoords{{
        { 0.0F, 1.0F },
        { 0.0F, 0.0F },
        { 1.0F, 0.0F },
        { 1.0F, 1.0F },
    }};
    return s_face_texcoords[index];
}

Data::Size Mesh::GetColorsCount() noexcept
{
    return g_colors_count;
}

const Mesh::Color& Mesh::GetColor(size_t index)
{
    static const std::array<Mesh::Color, g_colors_count> s_colors{{
        { 1.0F, 0.0F, 0.0F, 1.0F },
        { 0.0F, 1.0F, 0.0F, 1.0F },
        { 0.0F, 0.0F, 1.0F, 1.0F },
        { 1.0F, 0.0F, 1.0F, 1.0F },
        { 1.0F, 1.0F, 0.0F, 1.0F },
        { 0.0F, 1.0F, 1.0F, 1.0F },
    }};
    return s_colors[index];
}

Mesh::Index Mesh::GetFaceIndex(size_t index)
{
    // Face indices in quad to form two triangles in clockwise order
    static const std::array<Index, 6> s_face_indices{
        0, 1, 2, 0, 2, 3
    };
    return s_face_indices[index];
}

Data::Size Mesh::GetFaceIndicesCount() noexcept
{
    return g_face_indices_count;
}

std::string Mesh::VertexLayout::GetSemanticByVertexField(VertexField vertex_field)
{
    META_FUNCTION_TASK();

    switch(vertex_field)
    {
    case VertexField::Position: return "POSITION";
    case VertexField::Normal:   return "NORMAL";
    case VertexField::TexCoord: return "TEXCOORD";
    case VertexField::Color:    return "COLOR";
    default:                    META_UNEXPECTED_ENUM_ARG_RETURN(vertex_field, "");
    }
}

Mesh::VertexLayout::IncompatibleException::IncompatibleException(VertexField missing_field)
    : std::logic_error(fmt::format("Mesh vertex layout is incompatible, field {} is missing.", VertexLayout::GetSemanticByVertexField(missing_field)))
    , m_missing_field(missing_field)
{
    META_FUNCTION_TASK();
}

std::vector<std::string> Mesh::VertexLayout::GetSemantics() const
{
    META_FUNCTION_TASK();

    std::vector<std::string> semantic_names;
    for(VertexField vertex_field : *this)
    {
        semantic_names.emplace_back(GetSemanticByVertexField(vertex_field));
    }
    return semantic_names;
}

Mesh::Subset::Subset(Type in_mesh_type, const Slice& in_vertices, const Slice& in_indices, bool in_indices_adjusted)
    : mesh_type(in_mesh_type)
    , vertices(in_vertices)
    , indices(in_indices)
    , indices_adjusted(in_indices_adjusted)
{
    META_FUNCTION_TASK();
}

Mesh::VertexFieldOffsets Mesh::GetVertexFieldOffsets(const VertexLayout& vertex_layout)
{
    META_FUNCTION_TASK();

    size_t current_offset = 0;
    VertexFieldOffsets field_offsets{};
    std::fill(field_offsets.begin(), field_offsets.end(), -1);
    for (VertexField vertex_field : vertex_layout)
    {
        const size_t vertex_field_index = static_cast<size_t>(vertex_field);
        field_offsets[vertex_field_index] = static_cast<int32_t>(current_offset);
        current_offset += GetVertexFieldSize(vertex_field_index);
    }

    META_CHECK_ARG_NAME_DESCR("vertex_layout", field_offsets[static_cast<size_t>(VertexField::Position)] >= 0, "position field must be specified in vertex layout");
    return field_offsets;
}

Data::Size Mesh::GetVertexSize(const VertexLayout& vertex_layout) noexcept
{
    META_FUNCTION_TASK();
    Data::Size vertex_size = 0;
    for (VertexField vertex_field : vertex_layout)
    {
        vertex_size += GetVertexFieldSize(vertex_field);
    }
    return vertex_size;
}

Mesh::Mesh(Type type, const VertexLayout& vertex_layout)
    : m_type(type)
    , m_vertex_layout(vertex_layout)
    , m_vertex_field_offsets(GetVertexFieldOffsets(m_vertex_layout))
    , m_vertex_size(GetVertexSize(m_vertex_layout))
{
    META_FUNCTION_TASK();
    CheckLayoutHasVertexField(VertexField::Position);
}

bool Mesh::HasVertexField(VertexField field) const noexcept
{
    META_FUNCTION_TASK();
    return m_vertex_field_offsets[static_cast<size_t>(field)] >= 0;
}

void Mesh::CheckLayoutHasVertexField(VertexField field) const
{
    META_FUNCTION_TASK();
    if (!HasVertexField(field))
        throw VertexLayout::IncompatibleException(field);
}


Mesh::Edge::Edge(Mesh::Index v1_index, Mesh::Index v2_index)
    : first_index( v1_index < v2_index ? v1_index : v2_index)
    , second_index(v1_index < v2_index ? v2_index : v1_index)
{
}
    
bool Mesh::Edge::operator<(const Edge& other) const
{
    return first_index < other.first_index ||
          (first_index == other.first_index && second_index < other.second_index);
}

} // namespace Methane::Graphics
