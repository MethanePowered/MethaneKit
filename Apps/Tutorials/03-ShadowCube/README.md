# Shadow Cube Tutorial

| Windows (DirectX 12) | MacOS (Metal) |
| -------------------- | ------------- |
| ![Shadow Cube on Windows](Screenshots/ShadowCubeWinDirectX12.jpg) | ![Shadow Cube on MacOS](Screenshots/ShadowCubeMacMetal.jpg) |

This tutorial demonstrates using of two render passes for rendering shadow of the textured cube on the floor plane.
Tutorial demonstrates using of the following Methane Kit features additionally to features demonstrated in [TexturedCube](../02-TexturedCube) tutorial:
- TBD...

## Application and Frame Class Definitions

`ShadowCubeApp` class is declared in header file [ShadowCubeApp.h](ShadowCubeApp.h).
Private section of the class contains declaration of shader argument structures, 
which layout is matching definition of the equally named structures in [HLSL shader code](#shadow-cube-shaders):
- `Constants` data structure is stored in the `m_scene_constants` member and is uploaded into the `Graphics::Buffer` 
  object `m_const_buffer_ptr`, which has single instance in application since its data is constant for all frames.
- `SceneUniforms` data structure is stored in the `m_scene_uniforms` member and is uploaded into the `Graphics::Buffer` 
  objects in the per-frame `ShadowCubeFrame` objects, each with it's own state of volatile uniform values.
- `MeshUniforms` data structure contains Model/MVP matrices and shadow MVP+Transform matrix stored in 4 instances:
uniforms for shadow and final passes stored in `gfx::TexturedMeshBuffers` objects one for cube mesh in `m_cube_buffers_ptr` 
and one for floor mesh in `m_floor_buffers_ptr`.

[MeshBuffers.hpp](../../../Modules/Graphics/Extensions/Include/Methane/Graphics/MeshBuffers.hpp) implements auxiliary class
`TexturedMeshBuffers<UniformsType>` which is managing vertex, index, uniforms buffers and texture with data for particular
mesh drawing passed to constructor as a reference to [BaseMesh<VType>]((../../../Modules/Graphics/Primitives/Include/Methane/Graphics/Mesh/BaseMesh.hpp)) object.

Supplementary member `m_scene_uniforms_subresources` of type is used to store a pointer to
the `m_scene_uniforms` in the `std::vector` type `gfx::Resource::SubResources` which is passed to `gfx::Buffer::SetData(...)`
method to update the buffer data on GPU.

Two `gfx::Camera` objects are used: one `m_view_camera` is usual perspective view camera, while the other `m_light_camera`
is a directional light camera with orthogonal projection used to generate transformation matrix from view to light
coordinate systems.

Also there are two `gfx::Sampler` objects: one is used for sampling cube and floor textures, the other is used for
sampling shadow map texture.

```cpp
#pragma once

#include <Methane/Kit.h>
#include <Methane/UserInterface/App.hpp>

namespace Methane::Tutorials
{

namespace gfx = Methane::Graphics;

struct ShadowCubeFrame final : gfx::AppFrame
{
    ...    
};

using UserInterfaceApp = UserInterface::App<ShadowCubeFrame>;

class ShadowCubeApp final : public UserInterfaceApp
{
   ...

private:
    struct SHADER_STRUCT_ALIGN Constants
    {
        SHADER_FIELD_ALIGN gfx::Color4f   light_color;
        SHADER_FIELD_PACK  float          light_power;
        SHADER_FIELD_PACK  float          light_ambient_factor;
        SHADER_FIELD_PACK  float          light_specular_factor;
    };

    struct SHADER_STRUCT_ALIGN SceneUniforms
    {
        SHADER_FIELD_ALIGN gfx::Vector4f  eye_position;
        SHADER_FIELD_ALIGN gfx::Vector3f  light_position;
    };

    struct SHADER_STRUCT_ALIGN MeshUniforms
    {
        SHADER_FIELD_ALIGN gfx::Matrix44f model_matrix;
        SHADER_FIELD_ALIGN gfx::Matrix44f mvp_matrix;
        SHADER_FIELD_ALIGN gfx::Matrix44f shadow_mvpx_matrix;
    };

    using TexturedMeshBuffersBase = gfx::TexturedMeshBuffers<MeshUniforms>;
    class TexturedMeshBuffers : public TexturedMeshBuffersBase
    {
    public:
        using TexturedMeshBuffersBase::TexturedMeshBuffersBase;

        void                SetShadowPassUniforms(const MeshUniforms& uniforms) noexcept        { m_shadow_pass_uniforms = uniforms; }
        const MeshUniforms& GetShadowPassUniforms() const noexcept                              { return m_shadow_pass_uniforms; }
        const gfx::Resource::SubResources& GetShadowPassUniformsSubresources() const noexcept   { return m_shadow_pass_uniforms_subresources; }

    private:
        MeshUniforms                m_shadow_pass_uniforms{};
        gfx::Resource::SubResources m_shadow_pass_uniforms_subresources{
            { reinterpret_cast<Data::ConstRawPtr>(&m_shadow_pass_uniforms), sizeof(MeshUniforms) }
        };
    };

    ...

    const float                 m_scene_scale;
    const Constants             m_scene_constants;
    SceneUniforms               m_scene_uniforms{ };
    gfx::Resource::SubResources m_scene_uniforms_subresources{
        { reinterpret_cast<Data::ConstRawPtr>(&m_scene_uniforms), sizeof(SceneUniforms) }
    };
    gfx::Camera                 m_view_camera;
    gfx::Camera                 m_light_camera;
    Ptr<gfx::Buffer>            m_const_buffer_ptr;
    Ptr<gfx::Sampler>           m_texture_sampler_ptr;
    Ptr<gfx::Sampler>           m_shadow_sampler_ptr;
    Ptr<TexturedMeshBuffers>    m_cube_buffers_ptr;
    Ptr<TexturedMeshBuffers>    m_floor_buffers_ptr;
    
    ...
};

} // namespace Methane::Tutorials
```

`ShadowCubeFrame` struct contains frame-dependent volatile resources:
- Shadow & final pass resources in `shadow_pass` and `final_pass`:
  - Mesh resources for `cube` and `floor`:
    - Mesh uniforms buffer `uniforms_buffer_ptr`
    - Program bindings configuration `program_bindings_ptr`
  - Render target texture `rt_texture_ptr`
  - Render pass setup object `pass_ptr`
  - Render command list `cmd_list_ptr`
- Scene uniforms buffer in `scene_uniforms_buffer_ptr`
- Command list set for execution on frame, which contains command lists from shadow and final passes

```cpp
struct ShadowCubeFrame final : gfx::AppFrame
{
    struct PassResources
    {
        struct MeshResources
        {
            Ptr<gfx::Buffer>          uniforms_buffer_ptr;
            Ptr<gfx::ProgramBindings> program_bindings_ptr;
        };

        MeshResources               cube;
        MeshResources               floor;
        Ptr<gfx::Texture>           rt_texture_ptr;
        Ptr<gfx::RenderPass>        pass_ptr;
        Ptr<gfx::RenderCommandList> cmd_list_ptr;
    };

    PassResources            shadow_pass;
    PassResources            final_pass;
    Ptr<gfx::Buffer>         scene_uniforms_buffer_ptr;
    Ptr<gfx::CommandListSet> execute_cmd_list_set_ptr;

    using gfx::AppFrame::AppFrame;
};
```

## Graphics Resources Initialization

Initialization of textures, buffers and samplers is mostly the same as for `Textured Cube` tutorial, so we skip their
description here.

**Final pass** render state initialization has some differences:
- Pixel and vertex shaders are loaded for specific combination of macro definitions enabled during compilation done in build time.
  This macro definitions set is described in `gfx::Shader::MacroDefinitions` variable `textured_shadows_definitions` which
  is passed to `gfx::Shader::CreateVertex/CreatePixel` functions.
- Configuration of `gfx::Program::ArgumentDescriptions` is more complex than for simple cube mesh and mostly describes
  Pixel-shader specific argument modifiers, except `g_mesh_uniforms` for Vertex-shader.
  `g_constants`, `g_shadow_sampler`, `g_texture_sampler` arguments are described with `Constant` modifier, while
  other arguments have no modifiers meaning that that can change during frames rendering.

```cpp
    // ========= Final Pass Render & View States =========
    
    const gfx::Shader::EntryFunction    vs_main{ "ShadowCube", "CubeVS" };
    const gfx::Shader::EntryFunction    ps_main{ "ShadowCube", "CubePS" };
    const gfx::Shader::MacroDefinitions textured_shadows_definitions{ { "ENABLE_SHADOWS", "" }, { "ENABLE_TEXTURING", "" } };

    gfx::RenderState::Settings final_state_settings;
    final_state_settings.program_ptr = gfx::Program::Create(GetRenderContext(),
        gfx::Program::Settings
        {
            gfx::Program::Shaders
            {
                gfx::Shader::CreateVertex(GetRenderContext(), { Data::ShaderProvider::Get(), vs_main, textured_shadows_definitions }),
                gfx::Shader::CreatePixel(GetRenderContext(),  { Data::ShaderProvider::Get(), ps_main, textured_shadows_definitions }),
            },
            gfx::Program::InputBufferLayouts
            {
                gfx::Program::InputBufferLayout
                {
                    gfx::Program::InputBufferLayout::ArgumentSemantics { cube_mesh.GetVertexLayout().GetSemantics() }
                }
            },
            gfx::Program::ArgumentDescriptions
            {
                { { gfx::Shader::Type::Vertex, "g_mesh_uniforms"  }, gfx::Program::Argument::Modifiers::None     },
                { { gfx::Shader::Type::Pixel,  "g_scene_uniforms" }, gfx::Program::Argument::Modifiers::None     },
                { { gfx::Shader::Type::Pixel,  "g_constants"      }, gfx::Program::Argument::Modifiers::Constant },
                { { gfx::Shader::Type::Pixel,  "g_shadow_map"     }, gfx::Program::Argument::Modifiers::None     },
                { { gfx::Shader::Type::Pixel,  "g_shadow_sampler" }, gfx::Program::Argument::Modifiers::Constant },
                { { gfx::Shader::Type::Pixel,  "g_texture"        }, gfx::Program::Argument::Modifiers::None     },
                { { gfx::Shader::Type::Pixel,  "g_texture_sampler"}, gfx::Program::Argument::Modifiers::Constant },
            },
            gfx::PixelFormats
            {
                context_settings.color_format
            },
            context_settings.depth_stencil_format
        }
    );
    final_state_settings.program_ptr->SetName("Textured, Shadows & Lighting");
    final_state_settings.depth.enabled = true;

    m_final_pass.render_state_ptr = gfx::RenderState::Create(GetRenderContext(), final_state_settings);
    m_final_pass.render_state_ptr->SetName("Final pass render state");
```

**Shadow pass** render state is using the same shader code, but compiled with a different macro definitions set `textured_definitions`
and thus the result program having different set of arguments available. Also note that the program include only 
Vertex shader since it will be used for rendering to depth buffer only without color attachment.

```cpp
    // ========= Shadow Pass Render & View States =========
    
    gfx::Shader::MacroDefinitions textured_definitions{ { "ENABLE_TEXTURING", "" } };

    gfx::RenderState::Settings shadow_state_settings;
    shadow_state_settings.program_ptr = gfx::Program::Create(GetRenderContext(),
        gfx::Program::Settings
        {
            gfx::Program::Shaders
            {
                gfx::Shader::CreateVertex(GetRenderContext(), { Data::ShaderProvider::Get(), vs_main, textured_definitions }),
            },
            final_state_settings.program_ptr->GetSettings().input_buffer_layouts,
            gfx::Program::ArgumentDescriptions
            {
                { { gfx::Shader::Type::All, "g_mesh_uniforms"  }, gfx::Program::Argument::Modifiers::None },
            },
            gfx::PixelFormats { /* no color attachments, rendering to depth texture */ },
            shadow_texture_settings.pixel_format
        }
    );
    shadow_state_settings.program_ptr->SetName("Vertex Only: Textured, Lighting");
    shadow_state_settings.depth.enabled = true;

    m_shadow_pass.render_state_ptr = gfx::RenderState::Create(GetRenderContext(), shadow_state_settings);
    m_shadow_pass.render_state_ptr->SetName("Shadow-map render state");
```

The Shadow-pass view state is bound to the size of the Shadow-map texture:

```cpp
    m_shadow_pass.view_state_ptr = gfx::ViewState::Create({
        { gfx::GetFrameViewport(g_shadow_map_size)    },
        { gfx::GetFrameScissorRect(g_shadow_map_size) }
    });
```

Frame-dependent resources are initialized for each frame in loop. Execution command list set includes two command lists:
one for shadow pass rendering and another for final pass rendering.

```cpp
    for(ShadowCubeFrame& frame : GetFrames())
    {
        // Create uniforms buffer with volatile parameters for the whole scene rendering
        frame.scene_uniforms_buffer_ptr = gfx::Buffer::CreateVolatileBuffer(GetRenderContext(), scene_uniforms_data_size);
        frame.scene_uniforms_buffer_ptr->SetName(IndexedName("Scene Uniforms Buffer", frame.index));

        // ========= Shadow Pass Resources =========
        ...

        // ========= Final Pass Resources =========
        ...

        // Rendering command lists sequence
        frame.execute_cmd_list_set_ptr = gfx::CommandListSet::Create({
            *frame.shadow_pass.cmd_list_ptr,
            *frame.final_pass.cmd_list_ptr
        });
    }
```

Shadow-map render target texture `frame.shadow_pass.rt_texture_ptr` is created for each frame using common setting with depth-stencil format taken from 
render context settings. Shadow-map texture settings also specify `Usage` bit-mask with `RenderTarget` and `ShaderRead`
flags to allow both rendering to this texture and sampling from it in a final pass:

```cpp
    using namespace magic_enum::bitwise_operators;
    const gfx::Texture::Settings shadow_texture_settings = gfx::Texture::Settings::DepthStencilBuffer(
        gfx::Dimensions(g_shadow_map_size),
        context_settings.depth_stencil_format,
        gfx::Texture::Usage::RenderTarget | gfx::Texture::Usage::ShaderRead
    );
```

Volatile uniform buffers `frame.shadow_pass.[floor|cube].uniforms_buffer_ptr` are created separately for cube and floor 
meshes both for shadow and final passes rendering. Program bindings are created both for cube and floor meshes, 
which are binding created uniform buffers to the `g_mesh_uniforms` program argument of `All` shader types
(taking into account that there's only Vertex shader in that program).

Shadow render pass `frame.shadow_pass.pass_ptr` is created without color attachments, but with depth attachment
bound to the shadow-map texture for the current frame `frame.shadow_pass.rt_texture_ptr`.
Depth attachment is crated with `Clear` load action to clear the depth texture with provided depth value, 
taken from render context settings `context_settings.clear_depth_stencil->first`; and `Store` action is used to retain
rendered depth texture content for the next render pass. Render command list is created bound to the shadow render pass.

```cpp
        // ========= Shadow Pass Resources =========

        // Create uniforms buffer for Cube rendering in Shadow pass
        frame.shadow_pass.cube.uniforms_buffer_ptr = gfx::Buffer::CreateVolatileBuffer(GetRenderContext(), mesh_uniforms_data_size);
        frame.shadow_pass.cube.uniforms_buffer_ptr->SetName(IndexedName("Cube Uniforms Buffer for Shadow Pass", frame.index));

        // Create uniforms buffer for Floor rendering in Shadow pass
        frame.shadow_pass.floor.uniforms_buffer_ptr = gfx::Buffer::CreateVolatileBuffer(GetRenderContext(), mesh_uniforms_data_size);
        frame.shadow_pass.floor.uniforms_buffer_ptr->SetName(IndexedName("Floor Uniforms Buffer for Shadow Pass", frame.index));

        // Shadow-pass resource bindings for cube rendering
        frame.shadow_pass.cube.program_bindings_ptr = gfx::ProgramBindings::Create(shadow_state_settings.program_ptr, {
            { { gfx::Shader::Type::All, "g_mesh_uniforms"  }, { { frame.shadow_pass.cube.uniforms_buffer_ptr } } },
        });

        // Shadow-pass resource bindings for floor rendering
        frame.shadow_pass.floor.program_bindings_ptr = gfx::ProgramBindings::Create(shadow_state_settings.program_ptr, {
            { { gfx::Shader::Type::All, "g_mesh_uniforms"  }, { { frame.shadow_pass.floor.uniforms_buffer_ptr } } },
        });

        // Create depth texture for shadow map rendering
        frame.shadow_pass.rt_texture_ptr = gfx::Texture::CreateRenderTarget(GetRenderContext(), shadow_texture_settings);
        frame.shadow_pass.rt_texture_ptr->SetName(IndexedName("Shadow Map", frame.index));
        
        // Create shadow pass configuration with depth attachment
        frame.shadow_pass.pass_ptr = gfx::RenderPass::Create(GetRenderContext(), {
            { // No color attachments
            },
            gfx::RenderPass::DepthAttachment(
                {
                    gfx::Texture::Location(frame.shadow_pass.rt_texture_ptr),
                    gfx::RenderPass::Attachment::LoadAction::Clear,
                    gfx::RenderPass::Attachment::StoreAction::Store,
                },
                context_settings.clear_depth_stencil->first
            ),
            gfx::RenderPass::StencilAttachment(),
            gfx::RenderPass::Access::ShaderResources,
            false // intermediate render pass
        });

        // Create render pass and command list for shadow pass rendering
        frame.shadow_pass.cmd_list_ptr = gfx::RenderCommandList::Create(GetRenderContext().GetRenderCommandQueue(), *frame.shadow_pass.pass_ptr);
        frame.shadow_pass.cmd_list_ptr->SetName(IndexedName("Shadow-Map Rendering", frame.index));
```

The same resources are created for the final render pass: uniform buffers for cube and floor meshes.
Program bindings are created for cube and floor rendering too but with more complex set of program arguments, because
final pass rendering program includes pixel and vertex shaders, but not only vertex shader like in shadow pass.

Render target texture is bound to frame screen texture i.e. frame buffer in swap-chain.
Final render pass is also bound to the screen render pass for the current frame, which is created by base graphics 
application class `Methane::Graphics::App`. Render command list is created bound to the final render pass.

```cpp
        // ========= Final Pass Resources =========

        // Create uniforms buffer for Cube rendering in Final pass
        frame.final_pass.cube.uniforms_buffer_ptr = gfx::Buffer::CreateVolatileBuffer(GetRenderContext(), mesh_uniforms_data_size);
        frame.final_pass.cube.uniforms_buffer_ptr->SetName(IndexedName("Cube Uniforms Buffer for Final Pass", frame.index));

        // Create uniforms buffer for Floor rendering in Final pass
        frame.final_pass.floor.uniforms_buffer_ptr = gfx::Buffer::CreateVolatileBuffer(GetRenderContext(), mesh_uniforms_data_size);
        frame.final_pass.floor.uniforms_buffer_ptr->SetName(IndexedName("Floor Uniforms Buffer for Final Pass", frame.index));

        // Final-pass resource bindings for cube rendering
        frame.final_pass.cube.program_bindings_ptr = gfx::ProgramBindings::Create(final_state_settings.program_ptr, {
            { { gfx::Shader::Type::Vertex, "g_mesh_uniforms"  }, { { frame.final_pass.cube.uniforms_buffer_ptr   } } },
            { { gfx::Shader::Type::Pixel,  "g_scene_uniforms" }, { { frame.scene_uniforms_buffer_ptr             } } },
            { { gfx::Shader::Type::Pixel,  "g_constants"      }, { { m_const_buffer_ptr                          } } },
            { { gfx::Shader::Type::Pixel,  "g_shadow_map"     }, { { frame.shadow_pass.rt_texture_ptr            } } },
            { { gfx::Shader::Type::Pixel,  "g_shadow_sampler" }, { { m_shadow_sampler_ptr                        } } },
            { { gfx::Shader::Type::Pixel,  "g_texture"        }, { { m_cube_buffers_ptr->GetTexturePtr()         } } },
            { { gfx::Shader::Type::Pixel,  "g_texture_sampler"}, { { m_texture_sampler_ptr                       } } },
        });

        // Final-pass resource bindings for floor rendering - patched a copy of cube bindings
        frame.final_pass.floor.program_bindings_ptr = gfx::ProgramBindings::CreateCopy(*frame.final_pass.cube.program_bindings_ptr, {
            { { gfx::Shader::Type::Vertex, "g_mesh_uniforms"  }, { { frame.final_pass.floor.uniforms_buffer_ptr  } } },
            { { gfx::Shader::Type::Pixel,  "g_texture"        }, { { m_floor_buffers_ptr->GetTexturePtr()        } } },
        });

        // Bind final pass RT texture and pass to the frame buffer texture and final pass.
        frame.final_pass.rt_texture_ptr = frame.screen_texture_ptr;
        frame.final_pass.pass_ptr       = frame.screen_pass_ptr;

        // Create render pass and command list for final pass rendering
        frame.final_pass.cmd_list_ptr = gfx::RenderCommandList::Create(GetRenderContext().GetRenderCommandQueue(), *frame.final_pass.pass_ptr);
        frame.final_pass.cmd_list_ptr->SetName(IndexedName("Final Scene Rendering", frame.index));
```

## Frame Rendering Cycle

## Shadow Cube Shaders

```cpp
#include "..\..\..\Common\Shaders\Primitives.hlsl"

struct VSInput
{
    float3 position         : POSITION;
    float3 normal           : NORMAL;
#ifdef ENABLE_TEXTURING
    float2 texcoord         : TEXCOORD;
#endif
};

struct PSInput
{
    float4 position         : SV_POSITION;
    float3 world_position   : POSITION0;
    float3 world_normal     : NORMAL;
#ifdef ENABLE_SHADOWS
    float4 shadow_position  : POSITION1;
#endif
#ifdef ENABLE_TEXTURING
    float2 texcoord         : TEXCOORD;
#endif
};

struct Constants
{
    float4 light_color;
    float  light_power;
    float  light_ambient_factor;
    float  light_specular_factor;
};

struct SceneUniforms
{
    float4   eye_position;
    float3   light_position;
};

struct MeshUniforms
{
    float4x4 model_matrix;
    float4x4 mvp_matrix;
#ifdef ENABLE_SHADOWS
    float4x4 shadow_mvpx_matrix;
#endif
};

ConstantBuffer<Constants>     g_constants       : register(b1);
ConstantBuffer<SceneUniforms> g_scene_uniforms  : register(b2);
ConstantBuffer<MeshUniforms>  g_mesh_uniforms   : register(b3);

#ifdef ENABLE_SHADOWS
Texture2D                     g_shadow_map      : register(t0);
SamplerState                  g_shadow_sampler  : register(s0);
#endif

#ifdef ENABLE_TEXTURING
Texture2D                     g_texture         : register(t1);
SamplerState                  g_texture_sampler : register(s1);
#endif

PSInput CubeVS(VSInput input)
{
    const float4 position   = float4(input.position, 1.0F);

    PSInput output;
    output.position         = mul(g_mesh_uniforms.mvp_matrix, position);
    output.world_position   = mul(g_mesh_uniforms.model_matrix, position).xyz;
    output.world_normal     = normalize(mul(g_mesh_uniforms.model_matrix, float4(input.normal, 0.0)).xyz);
#ifdef ENABLE_SHADOWS
    output.shadow_position  = mul(g_mesh_uniforms.shadow_mvpx_matrix, position);
#endif
#ifdef ENABLE_TEXTURING
    output.texcoord         = input.texcoord;
#endif

    return output;
}

float4 CubePS(PSInput input) : SV_TARGET
{
    const float3 fragment_to_light             = normalize(g_scene_uniforms.light_position - input.world_position);
    const float3 fragment_to_eye               = normalize(g_scene_uniforms.eye_position.xyz - input.world_position);
    const float3 light_reflected_from_fragment = reflect(-fragment_to_light, input.world_normal);

#ifdef ENABLE_SHADOWS
    float3       light_proj_pos = input.shadow_position.xyz / input.shadow_position.w;
    const float  current_depth  = light_proj_pos.z - 0.0001F;
    const float  shadow_depth   = g_shadow_map.Sample(g_shadow_sampler, light_proj_pos.xy).r;
    const float  shadow_ratio   = current_depth > shadow_depth ? 1.0F : 0.0F;
#else
    const float  shadow_ratio   = 0.F;
#endif

#ifdef ENABLE_TEXTURING
    const float4 texel_color    = g_texture.Sample(g_texture_sampler, input.texcoord);
#else
    const float4 texel_color    = { 0.8F, 0.8F, 0.8F, 1.F };
#endif

    const float4 ambient_color  = texel_color * g_constants.light_ambient_factor;
    const float4 base_color     = texel_color * g_constants.light_color * g_constants.light_power;

    const float  distance       = length(g_scene_uniforms.light_position - input.world_position);
    const float  diffuse_part   = clamp(dot(fragment_to_light, input.world_normal), 0.0, 1.0);
    const float4 diffuse_color  = base_color * diffuse_part / (distance * distance);

    const float  specular_part  = pow(clamp(dot(fragment_to_eye, light_reflected_from_fragment), 0.0, 1.0), g_constants.light_specular_factor);
    const float4 specular_color = base_color * specular_part / (distance * distance);

    return ColorLinearToSrgb(ambient_color + (1.F - shadow_ratio) * (diffuse_color + specular_color));
}
```

## CMake Build Configuration

