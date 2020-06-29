/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/UserInterface/TextMesh.h
Methane text mesh generation helper.

******************************************************************************/

#pragma once

#include <Methane/UserInterface/Text.h>

#include <Methane/Graphics/Types.h>

#include <vector>

namespace Methane::Graphics
{

class TextMesh
{
public:
    struct Vertex
    {
        Vector2f position;
        Vector2f texcoord;
    };

    using Index = uint16_t;
    using Indices = std::vector<Index>;
    using Vertices = std::vector<Vertex>;

    TextMesh(const std::u32string& text, Text::Wrap wrap, Font& font, FrameSize& viewport_size);

    const Vertices&  GetVertices() const noexcept    { return vertices; }
    const Indices&   GetIndices() const noexcept     { return indices; }
    const FrameSize& GetContentSize() const noexcept { return content_size; }

    Data::Size GetVertexSize() const noexcept        { return static_cast<Data::Size>(sizeof(Vertex)); }
    Data::Size GetVerticesDataSize() const noexcept  { return static_cast<Data::Size>(vertices.size() * sizeof(Vertex)); }

    Data::Size GetIndexSize() const noexcept         { return static_cast<Data::Size>(sizeof(Index)); }
    Data::Size GetIndicesDataSize() const noexcept   { return static_cast<Data::Size>(indices.size() * sizeof(Index)); }

private:
    void AddCharQuad(const Font::Char& font_char, const FrameRect::Point& screen_char_pos,
                     const FrameSize& viewport_size, const FrameSize& atlas_size);

    Vertices  vertices;
    Indices   indices;
    FrameSize content_size;
};

} // namespace Methane::Graphics