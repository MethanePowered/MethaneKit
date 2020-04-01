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

struct SHADER_STRUCT_ALIGN TextConstants
{
    SHADER_FIELD_ALIGN Color4f blend_color;
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

    Data::Size GetVertexSize() const       { return sizeof(Vertex); }
    Data::Size GetIndexSize() const        { return sizeof(Index); }
    Data::Size GetVerticesDataSize() const { return static_cast<Data::Size>(vertices.size() * GetVertexSize()); }
    Data::Size GetIndicesDataSize() const  { return static_cast<Data::Size>(indices.size() * GetIndexSize()); }

    Mesh(const std::string& text, Font& font, const FrameSize& viewport_size, const FrameSize& atlas_size)
    {
        ITT_FUNCTION_TASK();

        // Left-Bottom corner of the current character in text line
        const uint32_t line_height = font.GetMaxGlyphSize().height;
        FrameRect::Point char_pos { 0, line_height };

        const Refs<const Font::Char> font_chars = font.GetTextChars(text);
        vertices.reserve(font_chars.size() * 4);
        indices.reserve(font_chars.size() * 6);

        const Font::Char* p_prev_font_char = nullptr;
        for(const auto& font_char_ref : font_chars)
        {
            const Font::Char& font_char = font_char_ref.get();
            assert(!!font_char);
            if (font_char.IsLineBreak())
            {
                char_pos = { 0u, char_pos.GetY() + line_height };
                continue;
            }

            char_pos += p_prev_font_char ? font.GetKerning(*p_prev_font_char, font_char) : FrameRect::Point();
            AddCharQuad(font_char, char_pos, viewport_size, atlas_size);

            char_pos.SetX(char_pos.GetX() + font_char.advance.GetX());
            p_prev_font_char = &font_char;
        }
    }

    void AddCharQuad(const Font::Char& font_char, const FrameRect::Point& screen_char_pos,
                     const FrameSize& viewport_size, const FrameSize& atlas_size)
    {
        ITT_FUNCTION_TASK();

        Point2f view_char_pos = screen_char_pos + font_char.offset;
        view_char_pos += Point2f(0.f, font_char.rect.size.height); // convert left-bottom to left-top position
        view_char_pos -= Point2f(viewport_size.width, viewport_size.height) / 2.f; // relative to viewport center

        // Char quad rectangle in viewport coordinates [-1, 1] x [-1, 1]
        const Rect<float, float> ver_rect {
            {
                static_cast<float>(view_char_pos.GetX()) *  2.f / viewport_size.width,
                static_cast<float>(view_char_pos.GetY()) * -2.f / viewport_size.height,
            },
            {
                static_cast<float>(font_char.rect.size.width)  * 2.f / viewport_size.width,
                static_cast<float>(font_char.rect.size.height) * 2.f / viewport_size.height,
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

Text::Text(RenderContext& context, Font& font, Settings settings, bool rect_in_pixels)
    : m_settings(std::move(settings))
    , m_context(context)
    , m_sp_font(font.shared_from_this())
    , m_sp_atlas_texture(font.GetAtlasTexturePtr(context))
{
    ITT_FUNCTION_TASK();

    const RenderContext::Settings& context_settings = context.GetSettings();
    if (!rect_in_pixels)
        m_settings.screen_rect *= m_context.GetContentScalingFactor();

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
    state_settings.viewports                                            = { GetFrameViewport(m_settings.screen_rect) };
    state_settings.scissor_rects                                        = { GetFrameScissorRect(m_settings.screen_rect) };
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

    const Data::Size const_buffer_size = static_cast<Data::Size>(sizeof(TextConstants));
    m_sp_const_buffer = Buffer::CreateConstantBuffer(context, Buffer::GetAlignedBufferSize(const_buffer_size));
    m_sp_const_buffer->SetName(m_settings.name + " Screen-Quad Constants Buffer");

    UpdateMeshBuffers();
    UpdateConstantsBuffer();

    m_sp_const_program_bindings = ProgramBindings::Create(state_settings.sp_program, {
        { { Shader::Type::Pixel, "g_constants" }, { { m_sp_const_buffer    } } },
        { { Shader::Type::Pixel, "g_texture"   }, { { m_sp_atlas_texture   } } },
        { { Shader::Type::Pixel, "g_sampler"   }, { { m_sp_texture_sampler } } },
    });
}

void Text::SetText(const std::string& text)
{
    if (m_settings.text == text)
        return;

    m_settings.text = text;
    UpdateMeshBuffers();
}

void Text::SetBlendColor(const Color4f& blend_color)
{
    ITT_FUNCTION_TASK();

    if (m_settings.blend_color == blend_color)
        return;

    m_settings.blend_color  = blend_color;
    UpdateConstantsBuffer();
}

void Text::SetScreenRect(const FrameRect& screen_rect, bool rect_in_pixels)
{
    ITT_FUNCTION_TASK();

    if (m_settings.screen_rect == screen_rect)
        return;

    m_settings.screen_rect = rect_in_pixels ? screen_rect : screen_rect * m_context.GetContentScalingFactor();

    m_sp_state->SetViewports({ GetFrameViewport(m_settings.screen_rect) });
    m_sp_state->SetScissorRects({ GetFrameScissorRect(m_settings.screen_rect) });
}

void Text::Draw(RenderCommandList& cmd_list) const
{
    ITT_FUNCTION_TASK();
    
    cmd_list.Reset(m_sp_state);
    cmd_list.SetProgramBindings(*m_sp_const_program_bindings);
    cmd_list.SetVertexBuffers({ *m_sp_vertex_buffer });
    cmd_list.DrawIndexed(RenderCommandList::Primitive::Triangle, *m_sp_index_buffer);
}

void Text::UpdateMeshBuffers()
{
    ITT_FUNCTION_TASK();

    assert(!!m_sp_font);
    assert(!!m_sp_atlas_texture);
    assert(!m_settings.text.empty());
    if (m_settings.text.empty())
        return;

    const Mesh text_mesh(m_settings.text, *m_sp_font, m_settings.screen_rect.size, m_sp_atlas_texture->GetSettings().dimensions);

    m_sp_vertex_buffer = Buffer::CreateVertexBuffer(m_context, text_mesh.GetVerticesDataSize(), text_mesh.GetVertexSize());
    m_sp_vertex_buffer->SetName(m_settings.name + " Text Vertex Buffer");
    m_sp_vertex_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(text_mesh.vertices.data()), text_mesh.GetVerticesDataSize() } });

    m_sp_index_buffer = Buffer::CreateIndexBuffer(m_context, text_mesh.GetIndicesDataSize(), PixelFormat::R16Uint);
    m_sp_index_buffer->SetName(m_settings.name + " Text Index Buffer");
    m_sp_index_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(text_mesh.indices.data()), text_mesh.GetIndicesDataSize() } });
}

void Text::UpdateConstantsBuffer() const
{
    ITT_FUNCTION_TASK();
    TextConstants constants {
        m_settings.blend_color
    };

    m_sp_const_buffer->SetData({
        {
            reinterpret_cast<Data::ConstRawPtr>(&constants),
            static_cast<Data::Size>(sizeof(constants))
        }
    });
}

} // namespace Methane::Graphics
