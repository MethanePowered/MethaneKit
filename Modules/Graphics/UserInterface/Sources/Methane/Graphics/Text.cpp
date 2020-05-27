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

FILE: Methane/Graphics/Text.cpp
Text rendering primitive.

******************************************************************************/

#include <Methane/Graphics/Text.h>
#include <Methane/Graphics/Font.h>

#include <Methane/Graphics/RenderCommandList.h>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

struct SHADER_STRUCT_ALIGN Text::Constants
{
    SHADER_FIELD_ALIGN Color4f color;
};

struct Text::Mesh
{
    struct Vertex
    {
        Vector2f position;
        Vector2f texcoord;
    };

    using Vertices = std::vector<Vertex>;
    Vertices vertices;

    using Index = uint16_t;
    using Indices = std::vector<Index>;
    Indices  indices;

    FrameSize content_size;

    Data::Size GetVertexSize() const       { return sizeof(Vertex); }
    Data::Size GetIndexSize() const        { return sizeof(Index); }
    Data::Size GetVerticesDataSize() const { return static_cast<Data::Size>(vertices.size() * GetVertexSize()); }
    Data::Size GetIndicesDataSize() const  { return static_cast<Data::Size>(indices.size() * GetIndexSize()); }
    
    using ProcessFontCharAtPosition = const std::function<void(const Font::Char& font_char, const FrameRect::Point& char_pos)>;
    static void ForEachTextCharacter(const std::string& text, Font& font, uint32_t viewport_width,
                                     const ProcessFontCharAtPosition& process_char_at_position)
    {
        META_FUNCTION_TASK();
        const uint32_t line_height = font.GetMaxGlyphSize().height;
        const Refs<const Font::Char> font_chars = font.GetTextChars(text);
        
        // Left-Bottom corner of the current character in text line
        FrameRect::Point char_pos { 0, line_height };
        const Font::Char* p_prev_font_char = nullptr;
        
        for(const auto& font_char_ref : font_chars)
        {
            const Font::Char& font_char = font_char_ref.get();
            assert(!!font_char);

            // Wrap to next line break on "line break" character or when text overruns viewport width
            if (font_char.IsLineBreak() || (viewport_width && char_pos.GetX() + font_char.rect.size.width > viewport_width))
                char_pos = { 0u, char_pos.GetY() + line_height };

            // Skip visualization of "line break" character
            if (font_char.IsLineBreak())
                continue;

            char_pos += p_prev_font_char ? font.GetKerning(*p_prev_font_char, font_char) : FrameRect::Point();
            process_char_at_position(font_char, char_pos);
            
            char_pos.SetX(char_pos.GetX() + font_char.advance.GetX());
            p_prev_font_char = &font_char;
        }
    }
    
    static void UpdateContentSizeWithChar(const Font::Char& font_char, const FrameRect::Point& char_pos, FrameSize& content_size)
    {
        content_size.width  = std::max(content_size.width,  char_pos.GetX() + font_char.offset.GetX() + font_char.rect.size.width);
        content_size.height = std::max(content_size.height, char_pos.GetY() + font_char.offset.GetY() + font_char.rect.size.height);
    }
    
    static FrameSize CalcContentSize(const std::string& text, Font& font, uint32_t viewport_width = 0u)
    {
        META_FUNCTION_TASK();
        FrameSize content_size;
        ForEachTextCharacter(text, font, viewport_width,
            [&content_size](const Font::Char& font_char, const FrameRect::Point& char_pos)
            {
                UpdateContentSizeWithChar(font_char, char_pos, content_size);
            }
        );
        return content_size;
    }

    Mesh(const std::string& text, Font& font, FrameSize& viewport_size, const FrameSize& atlas_size)
    {
        META_FUNCTION_TASK();
        
        bool is_content_size_initialized = false;
        if (!viewport_size.width || !viewport_size.height)
        {
            content_size = CalcContentSize(text, font, viewport_size.width);
            is_content_size_initialized = true;
            if (!viewport_size.width)
            {
                viewport_size.width = content_size.width;
            }
            if (!viewport_size.height)
            {
                viewport_size.height = content_size.height;
            }
        }
        
        const size_t text_length = text.length();
        vertices.reserve(text_length * 4);
        indices.reserve(text_length * 6);
        
        ForEachTextCharacter(text, font, viewport_size.width,
            [&](const Font::Char& font_char, const FrameRect::Point& char_pos)
            {
                AddCharQuad(font_char, char_pos, viewport_size, atlas_size);
                if (!is_content_size_initialized)
                {
                    UpdateContentSizeWithChar(font_char, char_pos, content_size);
                }
            }
        );
    }
    
    void AddCharQuad(const Font::Char& font_char, const FrameRect::Point& screen_char_pos,
                     const FrameSize& viewport_size, const FrameSize& atlas_size)
    {
        META_FUNCTION_TASK();

        Point2f view_char_pos = screen_char_pos + font_char.offset;
        view_char_pos += Point2f(0.f, font_char.rect.size.height); // convert left-bottom to left-top position
        view_char_pos -= Point2f(viewport_size.width, viewport_size.height) / 2.f; // relative to viewport center

        const float viewport_width  = viewport_size.width  ? static_cast<float>(viewport_size.width)  :  0.5f;
        const float viewport_height = viewport_size.height ? static_cast<float>(viewport_size.height) : -0.5f;

        // Char quad rectangle in viewport coordinates [-1, 1] x [-1, 1]
        const Rect<float, float> ver_rect {
            {
                static_cast<float>(view_char_pos.GetX()) *  2.f / viewport_width,
                static_cast<float>(view_char_pos.GetY()) * -2.f / viewport_height,
            },
            {
                static_cast<float>(font_char.rect.size.width)  * 2.f / viewport_width,
                static_cast<float>(font_char.rect.size.height) * 2.f / viewport_height,
            }
        };

        // Char atlas rectangle in texture coordinates [0, 1] x [0, 1]
        const Rect<float, float> tex_rect {
            {
                static_cast<float>(font_char.rect.origin.GetX()) / atlas_size.width,
                static_cast<float>(font_char.rect.origin.GetY()) / atlas_size.height,
            },
            {
                static_cast<float>(font_char.rect.size.width)  / atlas_size.width,
                static_cast<float>(font_char.rect.size.height) / atlas_size.height,
            }
        };

        if (vertices.size() + 6 > std::numeric_limits<Index>::max())
            throw std::runtime_error("Text mesh index buffer overflow.");

        const Index start_index = static_cast<Index>(vertices.size());

        vertices.emplace_back(Vertex{
            { ver_rect.GetLeft(), ver_rect.GetBottom() },
            { tex_rect.GetLeft(), tex_rect.GetTop() },
        });
        vertices.emplace_back(Vertex{
            { ver_rect.GetLeft(), ver_rect.GetTop() },
            { tex_rect.GetLeft(), tex_rect.GetBottom() },
        });
        vertices.emplace_back(Vertex{
            { ver_rect.GetRight(), ver_rect.GetTop() },
            { tex_rect.GetRight(), tex_rect.GetBottom() },
        });
        vertices.emplace_back(Vertex{
            { ver_rect.GetRight(), ver_rect.GetBottom() },
            { tex_rect.GetRight(), tex_rect.GetTop() },
        });

        indices.push_back(start_index);
        indices.push_back(start_index + 1);
        indices.push_back(start_index + 2);
        indices.push_back(start_index + 2);
        indices.push_back(start_index + 3);
        indices.push_back(start_index);
    }
};

Text::Text(RenderContext& context, Font& font, Settings settings)
    : m_settings(std::move(settings))
    , m_context(context)
    , m_sp_font(font.shared_from_this())
    , m_sp_atlas_texture(font.GetAtlasTexturePtr(context))
{
    META_FUNCTION_TASK();

    const RenderContext::Settings& context_settings = context.GetSettings();

    m_viewport_rect = m_settings.screen_rect;
    if (!m_settings.screen_rect_in_pixels)
        m_viewport_rect *= m_context.GetContentScalingFactor();

    m_sp_new_mesh_data = std::make_unique<Mesh>(m_settings.text, *m_sp_font, m_viewport_rect.size, m_sp_atlas_texture->GetSettings().dimensions);
    UpdateMeshBuffers();

    m_sp_new_const_data = std::make_unique<Constants>(Constants{ m_settings.color });
    UpdateConstantsBuffer();

    RenderState::Settings state_settings;
    state_settings.sp_program = Program::Create(context,
        Program::Settings
        {
            Program::Shaders
            {
                Shader::CreateVertex(context, { Data::ShaderProvider::Get(), { "Text", "TextVS" }, { } }),
                Shader::CreatePixel( context, { Data::ShaderProvider::Get(), { "Text", "TextPS" }, { } }),
            },
            Program::InputBufferLayouts
            {
                Program::InputBufferLayout
                {
                    Program::InputBufferLayout::ArgumentSemantics { "POSITION", "TEXCOORD" }
                }
            },
            Program::ArgumentDescriptions
            {
                { { Shader::Type::Pixel, "g_constants" }, Program::Argument::Modifiers::Constant },
                { { Shader::Type::Pixel, "g_texture"   }, Program::Argument::Modifiers::Constant },
                { { Shader::Type::Pixel, "g_sampler"   }, Program::Argument::Modifiers::Constant },
            },
            PixelFormats
            {
                context_settings.color_format
            },
            context_settings.depth_stencil_format
        }
    );
    state_settings.sp_program->SetName(m_settings.name + " Screen-Quad Shading");
    state_settings.viewports                                            = { GetFrameViewport(m_viewport_rect) };
    state_settings.scissor_rects                                        = { GetFrameScissorRect(m_viewport_rect) };
    state_settings.depth.enabled                                        = false;
    state_settings.depth.write_enabled                                  = false;
    state_settings.rasterizer.is_front_counter_clockwise                = true;
    state_settings.blending.render_targets[0].blend_enabled             = true;
    state_settings.blending.render_targets[0].source_rgb_blend_factor   = RenderState::Blending::Factor::SourceAlpha;
    state_settings.blending.render_targets[0].dest_rgb_blend_factor     = RenderState::Blending::Factor::OneMinusSourceAlpha;
    state_settings.blending.render_targets[0].source_alpha_blend_factor = RenderState::Blending::Factor::Zero;
    state_settings.blending.render_targets[0].dest_alpha_blend_factor   = RenderState::Blending::Factor::Zero;

    m_sp_state = RenderState::Create(context, state_settings);
    m_sp_state->SetName(m_settings.name + " Screen-Quad Render State");

    m_sp_texture_sampler = Sampler::Create(context, {
        { Sampler::Filter::MinMag::Linear     },
        { Sampler::Address::Mode::ClampToZero },
    });
    m_sp_texture_sampler->SetName(m_settings.name + " Screen-Quad Texture Sampler");

    m_sp_const_program_bindings = ProgramBindings::Create(state_settings.sp_program, {
        { { Shader::Type::Pixel, "g_constants" }, { { m_sp_const_buffer    } } },
        { { Shader::Type::Pixel, "g_texture"   }, { { m_sp_atlas_texture   } } },
        { { Shader::Type::Pixel, "g_sampler"   }, { { m_sp_texture_sampler } } },
    });
}

Text::~Text() = default;

void Text::SetText(const std::string& text)
{
    if (m_settings.text == text)
        return;

    m_settings.text = text;

    if (m_settings.text.empty())
    {
        m_sp_new_mesh_data.reset();
        return;
    }

    m_viewport_rect = m_settings.screen_rect;
    if (!m_settings.screen_rect_in_pixels)
        m_viewport_rect *= m_context.GetContentScalingFactor();

    assert(!!m_sp_font);
    assert(!!m_sp_atlas_texture);
    m_sp_new_mesh_data = std::make_unique<Mesh>(
        m_settings.text, *m_sp_font, m_viewport_rect.size,
        m_sp_atlas_texture->GetSettings().dimensions
    );

    m_sp_state->SetViewports({ GetFrameViewport(m_viewport_rect) });
    m_sp_state->SetScissorRects({ GetFrameScissorRect(m_viewport_rect) });
}

void Text::SetColor(const Color4f& color)
{
    META_FUNCTION_TASK();

    if (m_settings.color == color)
        return;

    m_settings.color = color;
    m_sp_new_const_data = std::make_unique<Constants>(Constants{
        m_settings.color
    });
}

void Text::SetScreenRect(const FrameRect& screen_rect, bool rect_in_pixels)
{
    META_FUNCTION_TASK();

    if (m_settings.screen_rect == screen_rect && m_settings.screen_rect_in_pixels == rect_in_pixels)
        return;

    m_settings.screen_rect = screen_rect;
    m_settings.screen_rect_in_pixels = rect_in_pixels;

    m_viewport_rect = screen_rect;
    if (!rect_in_pixels)
        m_viewport_rect *= m_context.GetContentScalingFactor();

    m_sp_new_mesh_data = std::make_unique<Mesh>(m_settings.text, *m_sp_font, m_viewport_rect.size, m_sp_atlas_texture->GetSettings().dimensions);

    m_sp_state->SetViewports({ GetFrameViewport(m_viewport_rect) });
    m_sp_state->SetScissorRects({ GetFrameScissorRect(m_viewport_rect) });
}

void Text::Draw(RenderCommandList& cmd_list)
{
    META_FUNCTION_TASK();

    if (m_settings.text.empty())
        return;

    if (m_sp_new_mesh_data)
        UpdateMeshBuffers();

    if (m_sp_new_const_data)
        UpdateConstantsBuffer();
    
    cmd_list.Reset(m_sp_state);
    cmd_list.SetProgramBindings(*m_sp_const_program_bindings);
    cmd_list.SetVertexBuffers(*m_sp_vertex_buffers);
    cmd_list.DrawIndexed(RenderCommandList::Primitive::Triangle, *m_sp_index_buffer);
}

void Text::UpdateMeshBuffers()
{
    META_FUNCTION_TASK();
    if (!m_sp_new_mesh_data)
        return;

    // Update vertex buffer
    const Data::Size vertices_data_size = m_sp_new_mesh_data->GetVerticesDataSize();
    if (!m_sp_vertex_buffers || (*m_sp_vertex_buffers)[0].GetDataSize() < vertices_data_size)
    {
        Ptr<Buffer> sp_vertex_buffer = Buffer::CreateVertexBuffer(m_context, vertices_data_size, m_sp_new_mesh_data->GetVertexSize());
        sp_vertex_buffer->SetName(m_settings.name + " Text Vertex Buffer");
        m_sp_vertex_buffers = Buffers::CreateVertexBuffers({ *sp_vertex_buffer });
    }
    (*m_sp_vertex_buffers)[0].SetData({
        Resource::SubResource(
            reinterpret_cast<Data::ConstRawPtr>(m_sp_new_mesh_data->vertices.data()), vertices_data_size,
            Resource::SubResource::Index(), Resource::BytesRange(0u, vertices_data_size)
        )
    });

    // Update index buffer
    const Data::Size indices_data_size = m_sp_new_mesh_data->GetIndicesDataSize();
    if (!m_sp_index_buffer || m_sp_index_buffer->GetDataSize() < indices_data_size)
    {
        m_sp_index_buffer = Buffer::CreateIndexBuffer(m_context, indices_data_size, PixelFormat::R16Uint);
        m_sp_index_buffer->SetName(m_settings.name + " Text Index Buffer");
    }
    m_sp_index_buffer->SetData({ 
        Resource::SubResource(
            reinterpret_cast<Data::ConstRawPtr>(m_sp_new_mesh_data->indices.data()), indices_data_size,
            Resource::SubResource::Index(), Resource::BytesRange(0u, indices_data_size)
        )
    });

    m_sp_new_mesh_data.reset();
}

void Text::UpdateConstantsBuffer()
{
    META_FUNCTION_TASK();
    if (!m_sp_new_const_data)
        return;

    const Data::Size const_data_size = static_cast<Data::Size>(sizeof(Constants));
    if (!m_sp_const_buffer)
    {
        m_sp_const_buffer = Buffer::CreateConstantBuffer(m_context, Buffer::GetAlignedBufferSize(const_data_size));
        m_sp_const_buffer->SetName(m_settings.name + " Screen-Quad Constants Buffer");
    }
    m_sp_const_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(m_sp_new_const_data.get()), const_data_size } });

    m_sp_new_const_data.reset();
}

} // namespace Methane::Graphics
