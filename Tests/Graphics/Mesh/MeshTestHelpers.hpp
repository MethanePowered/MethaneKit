/******************************************************************************

Copyright 2025 Evgeny Gorodetskiy

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

FILE: Tests/Graphics/Mesh/MeshTestHelpers.hpp
Mesh test helpers

******************************************************************************/

#pragma once

#include <Methane/Graphics/Mesh.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <fmt/format.h>

namespace // anonymous
{

using namespace Methane::Graphics;

struct MeshVertex
{
#ifdef MESH_VERTEX_POSITION
    Mesh::Position position;
#endif
#ifdef MESH_VERTEX_NORMAL
    Mesh::Normal   normal;
#endif
#ifdef MESH_VERTEX_COLOR
    Mesh::Color    color;
#endif
#ifdef MESH_VERTEX_TEXCOORD
    Mesh::TexCoord texcoord;
#endif

    bool operator==(const MeshVertex& other) const = default;

    inline static const Mesh::VertexLayout layout{
#ifdef MESH_VERTEX_POSITION
        Mesh::VertexField::Position,
#endif
#ifdef MESH_VERTEX_NORMAL
        Mesh::VertexField::Normal,
#endif
#ifdef MESH_VERTEX_COLOR
        Mesh::VertexField::Color,
#endif
#ifdef MESH_VERTEX_TEXCOORD
        Mesh::VertexField::TexCoord
#endif
    };
};

} // anonymous namespace

template<>
struct Catch::StringMaker<MeshVertex>
{
    static std::string convert(const MeshVertex& v)
    {
        return fmt::format(
            "{{\n"
#ifdef MESH_VERTEX_POSITION
            "    .position = {},\n"
#endif
#ifdef MESH_VERTEX_NORMAL
            "    .normal   = {},\n"
#endif
#ifdef MESH_VERTEX_COLOR
            "    .color    = {},\n"
#endif
#ifdef MESH_VERTEX_TEXCOORD
            "    .texcoord = {}\n"
#endif
            "}}"
#ifdef MESH_VERTEX_POSITION
            , v.position
#endif
#ifdef MESH_VERTEX_NORMAL
            , v.normal
#endif
#ifdef MESH_VERTEX_COLOR
            , v.color
#endif
#ifdef MESH_VERTEX_TEXCOORD
            , v.texcoord
#endif
        );
    }
};

namespace // anonymous
{

template<typename RawVectorType>
class RawVectorApproxEqualsMatcher
    : public Catch::Matchers::MatcherBase<RawVectorType>
{
public:
    RawVectorApproxEqualsMatcher(const RawVectorType& reference_vector, RawVectorType::ComponentType precision)
        : m_reference_vector(reference_vector)
        , m_precision(precision)
    { }

    bool match(const RawVectorType& other) const override
    {
        bool matches = true;
        for(size_t i = 0; i < RawVectorType::Size; ++i)
        {
            matches &= std::abs(m_reference_vector[i] - other[i]) < m_precision;
            if (!matches)
                break;
        }
        return matches;
    }

    std::string describe() const override
    {
        return fmt::format("approximately equals to {} with precision {}", m_reference_vector, m_precision);
    }

private:
    RawVectorType m_reference_vector;
    RawVectorType::ComponentType m_precision;
};

class MeshVertexApproxEqualsMatcher
    : public Catch::Matchers::MatcherBase<MeshVertex>
{
public:
    MeshVertexApproxEqualsMatcher(const MeshVertex& reference_vertex, float precision)
        : m_reference_vertex(reference_vertex)
#ifdef MESH_VERTEX_POSITION
        , m_position_matcher(m_reference_vertex.position, precision)
#endif
#ifdef MESH_VERTEX_NORMAL
        , m_normal_matcher(m_reference_vertex.normal, precision)
#endif
#ifdef MESH_VERTEX_COLOR
        , m_color_matcher(m_reference_vertex.color, precision)
#endif
#ifdef MESH_VERTEX_TEXCOORD
        , m_texcoord_matcher(m_reference_vertex.texcoord, precision)
#endif
    { }

    bool match(const MeshVertex& other) const override
    {
        return
#ifdef MESH_VERTEX_POSITION
            m_position_matcher.match(other.position) &&
#endif
#ifdef MESH_VERTEX_NORMAL
            m_normal_matcher.match(other.normal) &&
#endif
#ifdef MESH_VERTEX_COLOR
            m_color_matcher.match(other.color) &&
#endif
#ifdef MESH_VERTEX_TEXCOORD
            m_texcoord_matcher.match(other.texcoord) &&
#endif
            true;
    }

    std::string describe() const override
    {
        return fmt::format("approximately equals to {}", Catch::StringMaker<MeshVertex>::convert(m_reference_vertex));
    }

private:
    using Mesh = Methane::Graphics::Mesh;

    MeshVertex m_reference_vertex;
#ifdef MESH_VERTEX_POSITION
    RawVectorApproxEqualsMatcher<Mesh::Position> m_position_matcher;
#endif
#ifdef MESH_VERTEX_NORMAL
    RawVectorApproxEqualsMatcher<Mesh::Normal>   m_normal_matcher;
#endif
#ifdef MESH_VERTEX_COLOR
    RawVectorApproxEqualsMatcher<Mesh::Color>    m_color_matcher;
#endif
#ifdef MESH_VERTEX_TEXCOORD
    RawVectorApproxEqualsMatcher<Mesh::TexCoord> m_texcoord_matcher;
#endif
};

inline auto MeshVertexApproxEquals(const MeshVertex& reference_vertex, float precision) -> decltype(auto)
{
    return MeshVertexApproxEqualsMatcher(reference_vertex, precision);
}

inline void CheckMeshVerticesApproxEquals(const std::vector<MeshVertex>& mesh_vertices,
                                          const std::vector<MeshVertex>& reference_vertices,
                                          float precision = 0.001F)
{
    REQUIRE(mesh_vertices.size() == reference_vertices.size());
    for(size_t vertex_index = 0U; vertex_index < reference_vertices.size(); ++vertex_index)
    {
        CHECK_THAT(mesh_vertices[vertex_index], MeshVertexApproxEquals(reference_vertices[vertex_index], precision));
    }
}

} // anonymous namespace