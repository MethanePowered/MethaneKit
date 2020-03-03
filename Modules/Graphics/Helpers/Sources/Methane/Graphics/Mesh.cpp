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

FILE: Methane/Graphics/Mesh.cpp
Abstract mesh class

******************************************************************************/

#include <Methane/Graphics/Mesh.h>
#include <Methane/Instrumentation.h>

#include <cml/mathlib/mathlib.h>

#include <cassert>

namespace Methane::Graphics
{

const Mesh::VertexFieldSizes Mesh::g_vertex_field_sizes = {{
    sizeof(Position), 
    sizeof(Normal),
    sizeof(TexCoord),
    sizeof(Color),
}};

const Mesh::Positions2D Mesh::g_face_positions_2d = { // Quad vertices in clockwise order
    { -0.5f, -0.5f },
    { -0.5f,  0.5f },
    {  0.5f,  0.5f },
    {  0.5f, -0.5f }
};

const Mesh::TexCoords   Mesh::g_face_texcoords = { // Quad texture-coords for the vertices above
    { 0.0f, 1.0f },
    { 0.0f, 0.0f },
    { 1.0f, 0.0f },
    { 1.0f, 1.0f },
};

const Mesh::Indices     Mesh::g_face_indices = { // Face indices in quad to form two triangles in clockwise order
    0, 1, 2, 0, 2, 3
};

const Mesh::Colors      Mesh::g_colors = {
    { 1.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 1.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 1.0f, 1.0f },
    { 1.0f, 0.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 0.0f, 1.0f },
    { 0.0f, 1.0f, 1.0f, 1.0f },
};

std::string Mesh::VertexLayout::GetSemanticByVertexField(VertexField vertex_field)
{
    ITT_FUNCTION_TASK();

    switch(vertex_field)
    {
    case VertexField::Position: return "POSITION";
    case VertexField::Normal:   return "NORMAL";
    case VertexField::TexCoord: return "TEXCOORD";
    case VertexField::Color:    return "COLOR";
    default:                    assert(0);
    }

    return "";
}

std::vector<std::string> Mesh::VertexLayout::GetSemantics() const
{
    ITT_FUNCTION_TASK();

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
    ITT_FUNCTION_TASK();
}

Mesh::VertexFieldOffsets Mesh::GetVertexFieldOffsets(const VertexLayout& vertex_layout)
{
    ITT_FUNCTION_TASK();

    size_t current_offset = 0;
    VertexFieldOffsets field_offsets = {};
    std::fill(field_offsets.begin(), field_offsets.end(), -1);
    for (VertexField vertex_field : vertex_layout)
    {
        const size_t vertex_field_index = static_cast<size_t>(vertex_field);
        field_offsets[vertex_field_index] = static_cast<int32_t>(current_offset);
        current_offset += g_vertex_field_sizes[vertex_field_index];
    }
    if (field_offsets[static_cast<size_t>(VertexField::Position)] < 0)
    {
        throw std::invalid_argument("Position field must be specified in vertex layout");
    }
    return field_offsets;
}

Data::Size Mesh::GetVertexSize(const VertexLayout& vertex_layout) noexcept
{
    ITT_FUNCTION_TASK();

    Data::Size vertex_size = 0;
    for (VertexField vertex_field : vertex_layout)
    {
        vertex_size += g_vertex_field_sizes[static_cast<size_t>(vertex_field)];
    }
    return vertex_size;
}

Mesh::Mesh(Type type, const VertexLayout& vertex_layout)
    : m_type(type)
    , m_vertex_layout(vertex_layout)
    , m_vertex_field_offsets(GetVertexFieldOffsets(m_vertex_layout))
    , m_vertex_size(GetVertexSize(m_vertex_layout))
{
    ITT_FUNCTION_TASK();
    if (!Mesh::HasVertexField(Mesh::VertexField::Position))
    {
        throw std::invalid_argument("Vertex positions must be available in mesh layout.");
    }
}

bool Mesh::HasVertexField(VertexField field) const noexcept
{
    ITT_FUNCTION_TASK();
    return m_vertex_field_offsets[static_cast<size_t>(field)] >= 0;
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
