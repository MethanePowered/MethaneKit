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

    enum class CharAction
    {
        Continue,
        Wrap,
        Stop,
    };

    using ProcessFontCharAtPosition = const std::function<CharAction(const Font::Char& text_char, const FrameRect::Point& char_pos, size_t char_index)>;
    static void ForEachTextCharacter(const std::u32string& text, Font& font, uint32_t viewport_width, Wrap wrap,
                                     const ProcessFontCharAtPosition& process_char_at_position)
    {
        META_FUNCTION_TASK();
        const Font::Chars text_chars = font.GetTextChars(text);
        const ProcessFontCharAtPosition& word_wrap_char_at_position = // word wrap mode processor
            [&](const Font::Char& text_char, const FrameRect::Point& char_pos, size_t char_index) -> CharAction
            {
                if (text_char.IsWhiteSpace())
                {
                    // Word wrap prediction: check if next word fits in given viewport width
                    bool word_wrap_required = false;
                    const FrameRect::Point start_char_pos = { char_pos.GetX() + text_char.GetAdvance().GetX(), char_pos.GetY() };
                    ForEachTextCharacterInRange(font, text_chars, char_index + 1, text_chars.size(), start_char_pos, viewport_width, Wrap::Anywhere,
                        [&word_wrap_required, &start_char_pos, &text_chars](const Font::Char& text_char, const FrameRect::Point& char_pos, size_t char_index) -> CharAction
                        {
                            // Word has ended if whitespace character is received or line break character was passed
                            if (text_char.IsWhiteSpace() || (char_index && text_chars[char_index - 1].get().IsLineBreak()))
                                return CharAction::Stop;

                            word_wrap_required = char_pos.GetY() > start_char_pos.GetY();
                            return word_wrap_required ? CharAction::Stop : CharAction::Continue;
                        }
                    );
                    if (word_wrap_required)
                        return CharAction::Wrap;
                }
                return process_char_at_position(text_char, char_pos, char_index);
            };
        ForEachTextCharacterInRange(font, text_chars, 0, text_chars.size(),
                                    FrameRect::Point{ 0, font.GetLineHeight() }, viewport_width, wrap,
                                    wrap == Wrap::Word && viewport_width ? word_wrap_char_at_position : process_char_at_position);
    }

    static void ForEachTextCharacterInRange(Font& font, const Font::Chars& text_chars, size_t begin_index, size_t end_index,
                                            FrameRect::Point char_pos, uint32_t viewport_width, Wrap wrap,
                                            const ProcessFontCharAtPosition& process_char_at_position)
    {
        META_FUNCTION_TASK();
        const Font::Char* p_prev_text_char = nullptr;
        for (size_t char_index = begin_index; char_index < end_index; ++char_index)
        {
            const Font::Char& text_char = text_chars[char_index].get();
            assert(!!text_char);

            // Wrap to next line break on "line break" character or when text overruns viewport width
            if (text_char.IsLineBreak() || (wrap == Wrap::Anywhere && viewport_width && char_pos.GetX() + text_char.GetRect().size.width > viewport_width))
            {
                char_pos = { 0u, char_pos.GetY() + font.GetLineHeight() };
                p_prev_text_char = nullptr;
            }

            // Skip visualization of "line break" character
            if (text_char.IsLineBreak())
                continue;

            if (!p_prev_text_char && char_index && !text_chars[char_index - 1].get().IsLineBreak())
                p_prev_text_char = &(text_chars[char_index - 1].get());

            if (p_prev_text_char)
                char_pos += font.GetKerning(*p_prev_text_char, text_char);

            CharAction action = process_char_at_position(text_char, char_pos, char_index);
            switch (action)
            {
            case CharAction::Continue:
                char_pos.SetX(char_pos.GetX() + text_char.GetAdvance().GetX());
                p_prev_text_char = &text_char;
                break;

            case CharAction::Wrap:
                char_pos = { 0u, char_pos.GetY() + font.GetLineHeight() };
                p_prev_text_char = nullptr;
                break;

            case CharAction::Stop:
                return;
            }
        }
    }
    
    static void UpdateContentSizeWithChar(const Font::Char& font_char, const FrameRect::Point& char_pos, FrameSize& content_size)
    {
        content_size.width  = std::max(content_size.width,  char_pos.GetX() + font_char.GetOffset().GetX() + font_char.GetRect().size.width);
        content_size.height = std::max(content_size.height, char_pos.GetY() + font_char.GetOffset().GetY() + font_char.GetRect().size.height);
    }
    
    static FrameSize CalcContentSize(const std::u32string& text, Font& font, uint32_t viewport_width = 0u, Wrap wrap = Wrap::None)
    {
        META_FUNCTION_TASK();
        FrameSize content_size;
        ForEachTextCharacter(text, font, viewport_width, wrap,
            [&content_size](const Font::Char& text_char, const FrameRect::Point& char_pos, size_t) -> CharAction
            {
                UpdateContentSizeWithChar(text_char, char_pos, content_size);
                return CharAction::Continue;
            }
        );
        return content_size;
    }

    Mesh(const std::u32string& text, Wrap wrap, Font& font, FrameSize& viewport_size, const FrameSize& atlas_size)
    {
        META_FUNCTION_TASK();
        
        bool is_content_size_initialized = false;
        if (!viewport_size.width || !viewport_size.height)
        {
            content_size = CalcContentSize(text, font, viewport_size.width, wrap);
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
        
        ForEachTextCharacter(text, font, viewport_size.width, wrap,
            [&](const Font::Char& font_char, const FrameRect::Point& char_pos, size_t) -> CharAction
            {
                AddCharQuad(font_char, char_pos, viewport_size, atlas_size);
                if (!is_content_size_initialized)
                {
                    UpdateContentSizeWithChar(font_char, char_pos, content_size);
                }
                return CharAction::Continue;
            }
        );
    }
    
    void AddCharQuad(const Font::Char& font_char, const FrameRect::Point& screen_char_pos,
                     const FrameSize& viewport_size, const FrameSize& atlas_size)
    {
        META_FUNCTION_TASK();
        if (!viewport_size.width || !viewport_size.height)
            throw std::invalid_argument("All dimensions of the text viewport must be greater than zero.");

        Point2f view_char_pos = screen_char_pos + font_char.GetOffset();
        view_char_pos += Point2f(0.f, font_char.GetRect().size.height); // convert left-bottom to left-top position
        view_char_pos -= Point2f(viewport_size.width, viewport_size.height) / 2.f; // relative to viewport center

        // Char quad rectangle in viewport coordinates [-1, 1] x [-1, 1]
        const Rect<float, float> ver_rect {
            {
                static_cast<float>(view_char_pos.GetX()) *  2.f / viewport_size.width,
                static_cast<float>(view_char_pos.GetY()) * -2.f / viewport_size.height,
            },
            {
                static_cast<float>(font_char.GetRect().size.width)  * 2.f / viewport_size.width,
                static_cast<float>(font_char.GetRect().size.height) * 2.f / viewport_size.height,
            }
        };

        // Char atlas rectangle in texture coordinates [0, 1] x [0, 1]
        const Rect<float, float> tex_rect {
            {
                static_cast<float>(font_char.GetRect().origin.GetX()) / atlas_size.width,
                static_cast<float>(font_char.GetRect().origin.GetY()) / atlas_size.height,
            },
            {
                static_cast<float>(font_char.GetRect().size.width)  / atlas_size.width,
                static_cast<float>(font_char.GetRect().size.height) / atlas_size.height,
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

Text::Text(RenderContext& context, Font& font, const SettingsUtf8&  settings)
    : Text(context, font, SettingsUtf32{
            settings.name,
            Font::ConvertUtf8To32(settings.text),
            settings.screen_rect,
            settings.screen_rect_in_pixels,
            settings.color,
            settings.wrap
        })
{
    META_FUNCTION_TASK();
}

Text::Text(RenderContext& context, Font& font, SettingsUtf32 settings)
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

    UpdateMeshData();
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

    m_sp_const_program_bindings = ProgramBindings::Create(m_sp_state->GetSettings().sp_program, {
        { { Shader::Type::Pixel, "g_constants" }, { { m_sp_const_buffer    } } },
        { { Shader::Type::Pixel, "g_texture"   }, { { m_sp_atlas_texture   } } },
        { { Shader::Type::Pixel, "g_sampler"   }, { { m_sp_texture_sampler } } },
    });

    m_sp_font->Connect(*this);
}

Text::~Text() = default;

std::string Text::GetTextUtf8() const
{
    META_FUNCTION_TASK();
    return Font::ConvertUtf32To8(m_settings.text);
}

void Text::SetText(const std::string& text)
{
    META_FUNCTION_TASK();
    SetTextInScreenRect(text, m_settings.screen_rect, m_settings.screen_rect_in_pixels);
}

void Text::SetText(const std::u32string& text)
{
    META_FUNCTION_TASK();
    SetTextInScreenRect(text, m_settings.screen_rect, m_settings.screen_rect_in_pixels);
}

void Text::SetTextInScreenRect(const std::string& text, const FrameRect& screen_rect, bool rect_in_pixels)
{
    META_FUNCTION_TASK();
    SetTextInScreenRect(Font::ConvertUtf8To32(text), screen_rect, rect_in_pixels);
}

void Text::SetTextInScreenRect(const std::u32string& text, const FrameRect& screen_rect, bool rect_in_pixels)
{
    META_FUNCTION_TASK();
    if (m_settings.text == text && m_settings.screen_rect == screen_rect && m_settings.screen_rect_in_pixels == rect_in_pixels)
        return;

    m_settings.text = text;
    m_settings.screen_rect = screen_rect;
    m_settings.screen_rect_in_pixels = rect_in_pixels;

    if (m_settings.text.empty())
    {
        m_sp_new_mesh_data.reset();
        return;
    }

    m_viewport_rect = m_settings.screen_rect;
    if (!m_settings.screen_rect_in_pixels)
        m_viewport_rect *= m_context.GetContentScalingFactor();

    UpdateMeshData();

    m_sp_state->SetViewports({ GetFrameViewport(m_viewport_rect) });
    m_sp_state->SetScissorRects({ GetFrameScissorRect(m_viewport_rect) });
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

    UpdateMeshData();

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

void Text::Draw(RenderCommandList& cmd_list)
{
    META_FUNCTION_TASK();
    if (m_settings.text.empty())
        return;

    UpdateAtlasTexture();
    UpdateMeshBuffers();
    UpdateConstantsBuffer();
    
    cmd_list.Reset(m_sp_state);
    cmd_list.SetProgramBindings(*m_sp_const_program_bindings);
    cmd_list.SetVertexBuffers(*m_sp_vertex_buffers);
    cmd_list.DrawIndexed(RenderCommandList::Primitive::Triangle, *m_sp_index_buffer);
}

void Text::OnFontAtlasTextureReset(Font& font, const Ptr<Texture>& sp_old_atlas_texture, const Ptr<Texture>& sp_new_atlas_texture)
{
    META_FUNCTION_TASK();
    META_UNUSED(sp_old_atlas_texture);
    if (m_sp_font.get() != std::addressof(font) || std::addressof(m_context) != std::addressof(sp_new_atlas_texture->GetContext()))
        return;

    assert(m_sp_atlas_texture.get() == sp_old_atlas_texture.get());
    m_sp_new_atlas_texture = sp_new_atlas_texture;
}

void Text::UpdateAtlasTexture()
{
    META_FUNCTION_TASK();

    m_sp_font->UpdateAtlasTexture(m_context);

    if (!m_sp_new_atlas_texture)
        return;

    m_sp_atlas_texture = m_sp_new_atlas_texture;
    m_sp_new_atlas_texture.reset();

    const Ptr<ProgramBindings::ArgumentBinding>& sp_atlas_texture_binding = m_sp_const_program_bindings->Get({ Shader::Type::Pixel, "g_texture" });
    if (!sp_atlas_texture_binding)
        throw std::logic_error("Can not find atlas texture argument binding.");

    sp_atlas_texture_binding->SetResourceLocations({ { m_sp_atlas_texture } });

    UpdateMeshData();
}

void Text::UpdateMeshData()
{
    META_FUNCTION_TASK();
    assert(m_sp_atlas_texture);
    m_sp_new_mesh_data = std::make_unique<Mesh>(m_settings.text, m_settings.wrap, *m_sp_font, m_viewport_rect.size, m_sp_atlas_texture->GetSettings().dimensions);
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
