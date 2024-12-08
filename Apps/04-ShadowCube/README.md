# Shadow Cube Tutorial

| <pre><b>Windows (DirectX 12)       </pre></b>                     | <pre><b>Linux (Vulkan)             </pre></b>                | <pre><b>MacOS (Metal)              </pre></b>               | <pre><b>iOS (Metal)</pre></b>                             |
|-------------------------------------------------------------------|--------------------------------------------------------------|-------------------------------------------------------------|-----------------------------------------------------------|
| ![Shadow Cube on Windows](Screenshots/ShadowCubeWinDirectX12.jpg) | ![Shadow Cube on Linux](Screenshots/ShadowCubeLinVulkan.jpg) | ![Shadow Cube on MacOS](Screenshots/ShadowCubeMacMetal.jpg) | ![Shadow Cube on iOS](Screenshots/ShadowCubeIOSMetal.jpg) |

This tutorial demonstrates rendering shadow of the textured cube on the floor plane in two render passes using Methane Kit:
- [ShadowCubeApp.h](ShadowCubeApp.h)
- [ShadowCubeApp.cpp](ShadowCubeApp.cpp)
- [Shaders/ShadowCubeUniforms.h](Shaders/ShadowCubeUniforms.h)
- [Shaders/ShadowCube.hlsl](Shaders/ShadowCube.hlsl)

Tutorial demonstrates using of the following Methane Kit features additionally to features demonstrated in [TexturedCube](../03-TexturedCube) tutorial:
- Render with multiple render passes;
- Use texture as render target attachment in one pass and as an input program binding in another pass;
- Compile and loading shaders code with macro-definitions to render state programs;
- Use graphics extension `MeshBuffers` and `TexturedMeshBuffers` classes to simplify mesh rendering code;
- Simple shadows rendering technique. See detailed [technique description here](http://www.opengl-tutorial.org/ru/intermediate-tutorials/tutorial-16-shadow-mapping/)

## Application Controls

Common keyboard controls are enabled by the `Platform`, `Graphics` and `UserInterface` application controllers:
- [Methane::Platform::AppController](/Modules/Platform/App/README.md#platform-application-controller)
- [Methane::Graphics::AppController, AppContextController](/Modules/Graphics/App/README.md#graphics-application-controllers)
- [Methane::UserInterface::AppController](/Modules/UserInterface/App/README.md#user-interface-application-controllers)

## Application and Frame Class Definitions

`ShadowCubeApp` class is declared in header file [ShadowCubeApp.h](ShadowCubeApp.h) and the application class
is derived from [UserInterface::App](/Modules/UserInterface/App) base class, same as in [previous tutorial](../02-TexturedCube).
[Shaders/ShadowCubeUniforms.h](Shaders/ShadowCubeUniforms.h) header contains declaration of shader uniform structures shared between [HLSL shader code](#shadow-cube-shaders) and C++:
- `Constants` data structure with lighting parameters is saved to the root constant buffer via program argument binding
`g_constants`, which has single instance in application and its data is constant for all frames.
- `SceneUniforms` data structure with eye and light positions is saved to the root constant buffer via program argument
binding `g_scene_uniforms` with per-frame buffers with volatile uniforms data.
- `MeshUniforms` data structure contains Model/MVP matrices and shadow MVP+Transform matrix stored in 4 instances of
cube and floor meshes multiplied by shadow and final passes.
Uniforms for shadow and final passes stored in `gfx::TexturedMeshBuffers` objects one for cube mesh in `m_cube_buffers_ptr` 
and one for floor mesh in `m_floor_buffers_ptr`.

Uniform structures in [Shaders/ShadowCubeUniforms.h](Shaders/ShadowCubeUniforms.h):
```hlsl
struct Constants
{
    float4 light_color;
    float  light_power;
    float  light_ambient_factor;
    float  light_specular_factor;
    float  _padding;
};

struct SceneUniforms
{
    float4   eye_position;
    float4   light_position;
};

struct MeshUniforms
{
    float4x4 model_matrix;
    float4x4 mvp_matrix;
    float4x4 shadow_mvpx_matrix;
};
```

[MeshBuffers.hpp](../../Modules/Graphics/Extensions/Include/Methane/Graphics/MeshBuffers.hpp) implements auxiliary class
`TexturedMeshBuffers<UniformsType>` which is managing vertex and index buffers and texture with data for particular
mesh drawing passed to constructor as a reference to [BaseMesh<VType>]((../../../Modules/Graphics/Primitives/Include/Methane/Graphics/Mesh/BaseMesh.hpp)) object.

Two `gfx::Camera` objects are used: one `m_view_camera` is usual perspective view camera, while the other `m_light_camera`
is a directional light camera with orthogonal projection used to generate transformation matrix from view to light
coordinate systems.

Also, there are two `Rhi::Sampler` objects: one is used for sampling cube and floor textures, while the other is used for
sampling shadow map texture.

```cpp
#pragma once

#include <Methane/Kit.h>
#include <Methane/UserInterface/App.hpp>

namespace hlslpp
{
#define ENABLE_SHADOWS
#pragma pack(push, 16)
#include "Shaders/ShadowCubeUniforms.h"
#pragma pack(pop)
}

namespace Methane::Tutorials
{

namespace gfx = Methane::Graphics;
namespace rhi = Methane::Graphics::Rhi;

struct ShadowCubeFrame final : gfx::AppFrame
{
    ...
};

using UserInterfaceApp = UserInterface::App<ShadowCubeFrame>;

class ShadowCubeApp final : public UserInterfaceApp
{
   ...

private:
    using TexturedMeshBuffers = gfx::TexturedMeshBuffers<hlslpp::MeshUniforms>;

    struct RenderPassState
    {
        RenderPassState(bool is_final_pass, const std::string& command_group_name);
        void Release();

        const bool                       is_final_pass;
        const rhi::CommandListDebugGroup debug_group;
        rhi::RenderState                 render_state;
        rhi::ViewState                   view_state;
    };

    bool Animate(double elapsed_seconds, double delta_seconds);
    void RenderScene(const RenderPassState& render_pass, const ShadowCubeFrame::PassResources& render_pass_resources) const;

    gfx::Camera              m_view_camera;
    gfx::Camera              m_light_camera;
    rhi::Sampler             m_texture_sampler;
    rhi::Sampler             m_shadow_sampler;
    Ptr<TexturedMeshBuffers> m_cube_buffers_ptr;
    Ptr<TexturedMeshBuffers> m_floor_buffers_ptr;
    rhi::RenderPattern       m_shadow_pass_pattern;
    RenderPassState          m_shadow_pass { false, "Shadow Render Pass" };
    RenderPassState          m_final_pass  { true,  "Final Render Pass" };
};

} // namespace Methane::Tutorials
```

`ShadowCubeFrame` struct contains frame-dependent volatile resources:
- Shadow & final pass resources in `shadow_pass` and `final_pass`:
  - Mesh bindings for `cube` and `floor`:
    - Program bindings configuration `program_bindings`
    - Program argument binding for scene uniforms `scene_uniforms_binding_ptr`
    - Program argument binding for mesh uniforms `mesh_uniforms_binding_ptr`
  - Render target texture `rt_texture`
  - Render pass object `render_pass`
  - Render command list `cmd_list`
- Command list set for execution on frame, which contains command lists from shadow and final passes.

```cpp
struct ShadowCubeFrame final : gfx::AppFrame
{
    struct PassResources
    {
        struct ProgramBindings
        {
            rhi::ProgramBindings          program_bindings;
            rhi::IProgramArgumentBinding* scene_uniforms_binding_ptr = nullptr;
            rhi::IProgramArgumentBinding* mesh_uniforms_binding_ptr  = nullptr;
        };

        rhi::Texture           rt_texture;
        rhi::RenderPass        render_pass;
        rhi::RenderCommandList cmd_list;
        ProgramBindings        cube_bindings;
        ProgramBindings        floor_bindings;
    };

    PassResources       shadow_pass;
    PassResources       final_pass;
    rhi::CommandListSet execute_cmd_list_set;

    using gfx::AppFrame::AppFrame;
};
```

## Graphics Resources Initialization

Initialization of textures, buffers and samplers is mostly the same as for `Textured Cube` tutorial, so we skip their
description here.

**Final pass** render state initialization has some differences:
- Pixel and vertex shaders are loaded for specific combination of macro definitions used during compilation during build.
  This macro definitions set is described in `rhi::Shader::MacroDefinitions` variable `textured_shadows_definitions` which
  is passed to `rhi::Program::ShaderSet` description structure.

```cpp
    // ========= Final Pass Render & View States =========

    const rhi::Shader::EntryFunction    vs_main{ "ShadowCube", "CubeVS" };
    const rhi::Shader::EntryFunction    ps_main{ "ShadowCube", "CubePS" };
    const rhi::Shader::MacroDefinitions textured_shadows_definitions{ { "ENABLE_SHADOWS", "" }, { "ENABLE_TEXTURING", "" } };

    // Create final pass rendering state with program
    rhi::RenderState::Settings final_state_settings
    {
        render_context.CreateProgram(
            rhi::Program::Settings
            {
                rhi::Program::ShaderSet
                {
                    { rhi::ShaderType::Vertex, { Data::ShaderProvider::Get(), vs_main, textured_shadows_definitions } },
                    { rhi::ShaderType::Pixel,  { Data::ShaderProvider::Get(), ps_main, textured_shadows_definitions } },
                },
                rhi::ProgramInputBufferLayouts
                {
                    rhi::Program::InputBufferLayout
                    {
                        rhi::Program::InputBufferLayout::ArgumentSemantics { cube_mesh.GetVertexLayout().GetSemantics() }
                    }
                },
                rhi::ProgramArgumentAccessors
                {
                    META_PROGRAM_ARG_ROOT_BUFFER_CONSTANT(rhi::ShaderType::Pixel, "g_constants"),
                    META_PROGRAM_ARG_ROOT_BUFFER_FRAME_CONSTANT(rhi::ShaderType::Pixel, "g_scene_uniforms"),
                    META_PROGRAM_ARG_ROOT_BUFFER_MUTABLE(rhi::ShaderType::Vertex, "g_mesh_uniforms")
                },
                GetScreenRenderPattern().GetAttachmentFormats()
            }
        ),
        GetScreenRenderPattern()
    };
    final_state_settings.depth.enabled = true;

    m_final_pass.render_state = render_context.CreateRenderState( final_state_settings);
    m_final_pass.view_state = GetViewState();
```

`rhi::RenderPattern` class is used to define specific color/depth/stencil attachments configuration including their formats, 
load and store actions, without relation to specific resources used for attachments (this relation is set with `rhi::RenderPass` objects).
**Shadow pass** pattern uses only depth attachment which is cleared on load and stored for further use in final screen render pass.
The pattern also defines render pass access to shader resources and is marked with intermediate pass flag.

```cpp
    // ========= Shadow Pass Render & View States =========

    // Create shadow-pass render pattern
    m_shadow_pass_pattern = render_context.CreateRenderPattern({
        { }, // No color attachments
        rhi::IRenderPattern::DepthAttachment(
            0U, context_settings.depth_stencil_format, 1U,
            rhi::RenderPassAttachment::LoadAction::Clear,
            rhi::RenderPassAttachment::StoreAction::Store,
            context_settings.clear_depth_stencil->first
        ),
        std::nullopt, // No stencil attachment
        rhi::RenderPassAccessMask(rhi::RenderPassAccess::ShaderResources),
        false // intermediate render pass
    });
```

**Shadow pass** render state is using the same shader code, but compiled with a different macro definitions set `textured_definitions`
and thus the result program having different set of arguments available. Also note that the program include only 
Vertex shader since it will be used for rendering to depth buffer only without color attachment.

```cpp
    // Create shadow-pass rendering state with program
    const rhi::Shader::MacroDefinitions textured_definitions{ { "ENABLE_TEXTURING", "" } };
    rhi::RenderState::Settings shadow_state_settings
    {
        render_context.CreateProgram(
            rhi::Program::Settings
            {
                rhi::Program::ShaderSet
                {
                    { rhi::ShaderType::Vertex, { Data::ShaderProvider::Get(), vs_main, textured_definitions } },
                },
                final_state_settings.program.GetSettings().input_buffer_layouts,
                rhi::ProgramArgumentAccessors
                {
                    META_PROGRAM_ARG_ROOT_BUFFER_MUTABLE(rhi::ShaderType::Vertex, "g_mesh_uniforms")
                },
                m_shadow_pass_pattern.GetAttachmentFormats()
            }
        ),
        m_shadow_pass_pattern
    };
    shadow_state_settings.depth.enabled = true;
    m_shadow_pass.render_state = render_context.CreateRenderState( shadow_state_settings);
```

The Shadow-pass view state is bound to the size of the Shadow-map texture:

```cpp
    m_shadow_pass.view_state = rhi::ViewState({
        { gfx::GetFrameViewport(g_shadow_map_size)    },
        { gfx::GetFrameScissorRect(g_shadow_map_size) }
    });
```

Frame-dependent resources are initialized for each frame in loop. Execution command list set includes two command lists:
one for shadow pass rendering and another for final pass rendering.

```cpp
    for(ShadowCubeFrame& frame : GetFrames())
    {
        // ========= Shadow Pass Resources =========
        ...

        // ========= Final Pass Resources =========
        ...

        // Rendering command lists sequence
        frame.execute_cmd_list_set = rhi::CommandListSet({
            frame.shadow_pass.cmd_list.GetInterface(),
            frame.final_pass.cmd_list.GetInterface()
        }, frame.index);
    }
```

Shadow-map render target texture `frame.shadow_pass.rt_texture_ptr` is created for each frame using common setting with 
depth-stencil format taken from render context settings. Shadow-map texture settings also specify `Usage` bit-mask with
`RenderTarget` and `ShaderRead` flags to allow both rendering to this texture and sampling from it in a final pass:

```cpp
    const rhi::Texture::Settings shadow_texture_settings = rhi::Texture::Settings::ForDepthStencil(
        gfx::Dimensions(g_shadow_map_size),
        context_settings.depth_stencil_format, context_settings.clear_depth_stencil,
        rhi::ResourceUsageMask({ rhi::ResourceUsage::RenderTarget, rhi::ResourceUsage::ShaderRead })
    );
```

Program bindings `frame.shadow_pass.[cube|floor]_bindings.program_bindings` are created both for cube and floor meshes,
with argument binding for `g_mesh_uniforms` argument of `Vertex` shader type saved to `mesh_uniforms_binding_ptr`
to be used later in `ShadowCubeApp::Update()` method for setup of the root constant data.

Shadow render pass `frame.shadow_pass.render_pass` is created without color attachments, but with depth attachment
bound to the shadow-map texture for the current frame `frame.shadow_pass.rt_texture`.
Depth attachment is crated with `Clear` load action to clear the depth texture with provided depth value, 
taken from render context settings `context_settings.clear_depth_stencil->first`; and `Store` action is used to retain
rendered depth texture content for the next render pass. Render command list is created bound to the shadow render pass.

```cpp
        // ========= Shadow Pass Resources =========
        
        // Shadow-pass resource bindings for cube rendering
        ShadowCubeFrame::PassResources::ProgramBindings& shadow_cube_binds = frame.shadow_pass.cube_bindings;
        shadow_cube_binds.program_bindings = shadow_state_settings.program.CreateBindings({ }, frame.index);
        shadow_cube_binds.mesh_uniforms_binding_ptr = &shadow_cube_binds.program_bindings.Get({ rhi::ShaderType::Vertex, "g_mesh_uniforms" });

        // Shadow-pass resource bindings for floor rendering
        ShadowCubeFrame::PassResources::ProgramBindings& shadow_floor_binds = frame.shadow_pass.floor_bindings;
        shadow_floor_binds.program_bindings = shadow_state_settings.program.CreateBindings({ }, frame.index);
        shadow_floor_binds.mesh_uniforms_binding_ptr = &shadow_floor_binds.program_bindings.Get({ rhi::ShaderType::Vertex, "g_mesh_uniforms" });

        // Create depth texture for shadow map rendering
        frame.shadow_pass.rt_texture = render_context.CreateTexture(shadow_texture_settings);
        
        // Create shadow pass configuration with depth attachment
        frame.shadow_pass.render_pass = m_shadow_pass_pattern.CreateRenderPass({
            { frame.shadow_pass.rt_texture.GetInterface() },
            shadow_texture_settings.dimensions.AsRectSize()
        });
        
        // Create render pass and command list for shadow pass rendering
        frame.shadow_pass.cmd_list = render_cmd_queue.CreateRenderCommandList(frame.shadow_pass.render_pass);
```

The same resources are created for the final render pass.
Program bindings are created for cube and floor rendering but with extended set of program arguments, because
final pass rendering program includes both pixel and vertex shaders, but not only vertex shader like in shadow pass.

Render target texture is bound to frame screen texture i.e. frame buffer in swap-chain.
Final render pass is also bound to the screen render pass for the current frame, which is created by base graphics 
application class `Methane::Graphics::App`. Render command list is created bound to the final render pass.

```cpp
        // ========= Final Pass Resources =========

        // Final-pass resource bindings for cube rendering
        ShadowCubeFrame::PassResources::ProgramBindings& final_cube_binds = frame.final_pass.cube_bindings;
        final_cube_binds.program_bindings = final_state_settings.program.CreateBindings({
            { { rhi::ShaderType::Pixel,  "g_constants"      }, rhi::RootConstant(g_scene_constants)               },
            { { rhi::ShaderType::Pixel,  "g_shadow_map"     }, frame.shadow_pass.rt_texture.GetResourceView()     },
            { { rhi::ShaderType::Pixel,  "g_shadow_sampler" }, m_shadow_sampler.GetResourceView()                 },
            { { rhi::ShaderType::Pixel,  "g_texture"        }, m_cube_buffers_ptr->GetTexture().GetResourceView() },
            { { rhi::ShaderType::Pixel,  "g_texture_sampler"}, m_texture_sampler.GetResourceView()                },
        }, frame.index);
        final_cube_binds.scene_uniforms_binding_ptr = &final_cube_binds.program_bindings.Get({ rhi::ShaderType::Pixel, "g_scene_uniforms" });
        final_cube_binds.mesh_uniforms_binding_ptr  = &final_cube_binds.program_bindings.Get({ rhi::ShaderType::Vertex, "g_mesh_uniforms" });

        // Final-pass resource bindings for floor rendering - patched a copy of cube bindings
        ShadowCubeFrame::PassResources::ProgramBindings& final_floor_binds = frame.final_pass.floor_bindings;
        final_floor_binds.program_bindings = rhi::ProgramBindings(frame.final_pass.cube_bindings.program_bindings, {
            { { rhi::ShaderType::Pixel,  "g_texture" }, m_floor_buffers_ptr->GetTexture().GetResourceView() },
        }, frame.index);
        final_floor_binds.scene_uniforms_binding_ptr = &final_floor_binds.program_bindings.Get({ rhi::ShaderType::Pixel,  "g_scene_uniforms" });
        final_floor_binds.mesh_uniforms_binding_ptr  = &final_floor_binds.program_bindings.Get({ rhi::ShaderType::Vertex, "g_mesh_uniforms"  });

        // Bind final pass RT texture and pass to the frame buffer texture and final pass.
        frame.final_pass.rt_texture  = frame.screen_texture;
        frame.final_pass.render_pass = frame.screen_pass;
        
        // Create render pass and command list for final pass rendering
        frame.final_pass.cmd_list = render_cmd_queue.CreateRenderCommandList(frame.final_pass.render_pass);

        // Rendering command lists sequence
        frame.execute_cmd_list_set = rhi::CommandListSet({
            frame.shadow_pass.cmd_list.GetInterface(),
            frame.final_pass.cmd_list.GetInterface()
        }, frame.index);
```

When render context is going to be released, all related resources must be released before that. This is done in 
`ShadowCubeApp::OnContextReleased` callback method with a helper method `ShadowCubeApp::RenderPass::Release()` 
releasing render pass pipeline states:

```cpp
void ShadowCubeApp::OnContextReleased(gfx::Context& context)
{
    m_final_pass.Release();
    m_shadow_pass.Release();

    m_floor_buffers_ptr.reset();
    m_cube_buffers_ptr.reset();

    m_shadow_sampler = {};
    m_texture_sampler = {};
    m_shadow_pass_pattern = {};

    UserInterfaceApp::OnContextReleased(context);
}

void ShadowCubeApp::RenderPassState::Release()
{
    render_state = {};
    view_state = {};
}
```

## Frame Rendering Cycle

Animation function bound to time-animation in constructor of `ShadowCubeApp` class is called automatically as a part of 
every render cycle, just before `App::Update` function call. This function rotates light position and camera in opposite directions.

```cpp
ShadowCubeApp::ShadowCubeApp()
{
    ...
    GetAnimations().emplace_back(std::make_shared<Data::TimeAnimation>(std::bind(&ShadowCubeApp::Animate, this, std::placeholders::_1, std::placeholders::_2)));
}

bool ShadowCubeApp::Animate(double, double delta_seconds)
{
    m_view_camera.Rotate(m_view_camera.GetOrientation().up, static_cast<float>(delta_seconds * 360.F / 8.F));
    m_light_camera.Rotate(m_light_camera.GetOrientation().up, static_cast<float>(delta_seconds * 360.F / 4.F));
    return true;
}
```

`ShadowCubeApp::Update()` function is called before `App::Render()` call to update shader uniforms:
- Scene uniforms structure is created with eye and light positions calculated in `ShadowCubeApp::Animate` function.
- Cube and Floor mesh uniform structures are created separately for Final and Render passes:
  - Shadow pass MVP matrix is calculated from the light point of view using `m_light_camera.GetViewProjMatrix()`
    and the shadow-MVPx matrix is not used for shadow-map rendering, so it is set to zero matrix.
  - Final pass MVP matrix is calculated from the observer point of view using `m_view_camera.GetViewProjMatrix()`.
    The shadow-MVPx matrix is used to calculate current pixel coordinates in the shadow-map texture, so we use
    MVP matrix used during shadow pass rendering multiplied by coordinates transformation matrix to convert 
    from homogenous [-1, 1] to texture coordinates [0,1].

All uniforms are set directly to program argument binding with `rhi::ProgramArgumentBinding::SetRootConstant(...)` method.

```cpp
bool ShadowCubeApp::Update()
{
    if (!UserInterfaceApp::Update())
        return false;

    const hlslpp::SceneUniforms scene_uniforms{
        hlslpp::float4(m_view_camera.GetOrientation().eye, 1.F), // eye_position
        hlslpp::float4(m_light_camera.GetOrientation().eye, 1.F) // light_position
    };
    const rhi::RootConstant scene_uniforms_constant(scene_uniforms);

    // Prepare homogenous [-1,1] to texture [0,1] coordinates transformation matrix
    static const hlslpp::float4x4 s_homogen_to_texture_coords_matrix = hlslpp::mul(
        hlslpp::float4x4::scale(0.5F, -0.5F, 1.F),
        hlslpp::float4x4::translation(0.5F, 0.5F, 0.F)
    );

    const hlslpp::float4x4 scale_matrix = hlslpp::float4x4::scale(15.F);
    const hlslpp::float4x4 cube_model_matrix = hlslpp::mul(hlslpp::float4x4::translation(0.F, 0.5F, 0.F), scale_matrix);

    const ShadowCubeFrame& frame = GetCurrentFrame();

    // Update Cube uniforms
    const hlslpp::MeshUniforms final_cube_uniforms{
        hlslpp::transpose(cube_model_matrix),
        hlslpp::transpose(hlslpp::mul(cube_model_matrix, m_view_camera.GetViewProjMatrix())),
        hlslpp::transpose(hlslpp::mul(hlslpp::mul(cube_model_matrix, m_light_camera.GetViewProjMatrix()), s_homogen_to_texture_coords_matrix))
    };
    const hlslpp::MeshUniforms shadow_cube_uniforms{
        hlslpp::transpose(cube_model_matrix),
        hlslpp::transpose(hlslpp::mul(cube_model_matrix, m_light_camera.GetViewProjMatrix())),
        hlslpp::float4x4()
    };
    frame.final_pass.cube_bindings.scene_uniforms_binding_ptr->SetRootConstant(scene_uniforms_constant);
    frame.final_pass.cube_bindings.mesh_uniforms_binding_ptr->SetRootConstant(rhi::RootConstant(final_cube_uniforms));
    frame.shadow_pass.cube_bindings.mesh_uniforms_binding_ptr->SetRootConstant(rhi::RootConstant(shadow_cube_uniforms));

    // Update Floor uniforms
    const hlslpp::MeshUniforms final_floor_uniforms{
        hlslpp::transpose(scale_matrix),
        hlslpp::transpose(hlslpp::mul(scale_matrix, m_view_camera.GetViewProjMatrix())),
        hlslpp::transpose(hlslpp::mul(hlslpp::mul(scale_matrix, m_light_camera.GetViewProjMatrix()), s_homogen_to_texture_coords_matrix))
    };
    const hlslpp::MeshUniforms shadow_floor_uniforms{
        hlslpp::transpose(scale_matrix),
        hlslpp::transpose(hlslpp::mul(scale_matrix, m_light_camera.GetViewProjMatrix())),
        hlslpp::float4x4()
    };
    frame.final_pass.floor_bindings.scene_uniforms_binding_ptr->SetRootConstant(scene_uniforms_constant);
    frame.final_pass.floor_bindings.mesh_uniforms_binding_ptr->SetRootConstant(rhi::RootConstant(final_floor_uniforms));
    frame.shadow_pass.floor_bindings.mesh_uniforms_binding_ptr->SetRootConstant(rhi::RootConstant(shadow_floor_uniforms));
    
    return true;
}
```

Scene rendering consists is done in `ShadowCubeApp::Render()` method in 3 steps:
1. Shadow pass rendering commands are encoded with `ShadowCubeApp::RenderScene(...)` method for the current scene 
   using already configured shadow render pass bound to shadow render command list and shadow-pass uniforms.
2. Final pass rendering commands are encoded with `ShadowCubeApp::RenderScene(...)` method for the same scene 
   using already configured final render pass bound to final render command list and final-pass uniforms.
3. Shadow and Final pass rendering command lists are sent for execution to GPU using render command queue from context 
   and frame present is scheduled.

```cpp
bool ShadowCubeApp::Render()
{
    if (!UserInterfaceApp::Render())
        return false;

    // Record commands for shadow & final render passes
    const ShadowCubeFrame& frame = GetCurrentFrame();
    RenderScene(m_shadow_pass, frame.shadow_pass);
    RenderScene(m_final_pass, frame.final_pass);

    // Execute rendering commands and present frame to screen
    GetRenderContext().GetRenderCommandKit().GetQueue().Execute(frame.execute_cmd_list_set);
    GetRenderContext().Present();
    
    return true;
}
```

Scene rendering commands encoding is done similarly for both shadow and render passes:
1. Render command list is reset with state taken from render pass resources and already configured debug group description.
2. View state is set with viewports and scissor rects.
3. Cube and floor meshes drawing commands are encoded using
   [TexturedMeshBuffers<UniformsType>::Draw(...)](../../Modules/Graphics/Extensions/Include/Methane/Graphics/MeshBuffers.hpp)
   method which is doing:
   1. Setting program bindings to resources;
   2. Setting vertex buffer to draw;
   3. Encodes `DrawIndexed` command for a given mesh subset.
   4. Methane application overlay is rendered as a part of Final pass only using `Graphics::App::RenderOverlay(...)` method
   from base application class.
   5. Command list is committed making it ready for execution.

```cpp
void ShadowCubeApp::RenderScene(const RenderPassState& render_pass, const ShadowCubeFrame::PassResources& render_pass_resources) const
{
    const rhi::RenderCommandList& cmd_list = render_pass_resources.cmd_list;

    // Reset command list with initial rendering state
    cmd_list.ResetWithState(render_pass.render_state, &render_pass.debug_group);
    cmd_list.SetViewState(render_pass.view_state);

    // Draw scene with cube and floor
    m_cube_buffers_ptr->Draw(cmd_list, render_pass_resources.cube_bindings.program_bindings);
    m_floor_buffers_ptr->Draw(cmd_list, render_pass_resources.floor_bindings.program_bindings);

    if (render_pass.is_final_pass)
    {
        RenderOverlay(cmd_list);
    }

    cmd_list.Commit();
}
```

Graphics render loop is started from `main(...)` entry function using `GraphicsApp::Run(...)` method which is also parsing command line arguments.

```cpp
int main(int argc, const char* argv[])
{
    return ShadowCubeApp().Run({ argc, argv });
}
```

## Shadow Cube Shaders

HLSL 6 shaders [Shaders/ShadowCube.hlsl](Shaders/ShadowCube.hlsl) implement both shadow pass rendering and 
final pass with phong lighting, texturing and shadow map sampling all in one source file with help of `#ifdef ... #endif`
pre-processor guards. These code blocks are enabled with macro-definitions passed to shader compiler:
- `ENABLE_TEXTURING` macro-definition:
  - Adds `texcoord` vector to `VSInput` and `PSInput` argument structures; enables code for passing texture coordinates
    from vertex to pixel shader with interpolation.
  - Adds texture `g_texture` along with sampler `g_texture_sampler`
    and enables code path for its sampling in pixel shader `CubePS`.
- `ENABLE_SHADOWS` macro-definition:
  - Adds `shadow_position` vector to `PSInput` arguments structure and enables code path to calculate it 
    in vertex shader `CubeVS`;
  - Adds `shadow_mvpx_matrix` matrix to MeshUniforms structure of `g_mesh_uniforms` buffer;
  - Adds shadow-map texture `g_shadow_map` along with shadow-map sampler `g_shadow_sampler`
    and enables code path for shadow map sampling in pixel shader `CubePS`.

```cpp
#include "ShadowCubeUniforms.h"
#include "../../Common/Shaders/Primitives.hlsl"

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

ConstantBuffer<Constants>     g_constants       : register(b0, META_ARG_CONSTANT);
ConstantBuffer<SceneUniforms> g_scene_uniforms  : register(b1, META_ARG_FRAME_CONSTANT);
ConstantBuffer<MeshUniforms>  g_mesh_uniforms   : register(b2, META_ARG_MUTABLE);

#ifdef ENABLE_SHADOWS
Texture2D    g_shadow_map      : register(t0, META_ARG_FRAME_CONSTANT);
SamplerState g_shadow_sampler  : register(s0, META_ARG_CONSTANT);
#endif

#ifdef ENABLE_TEXTURING
Texture2D    g_texture         : register(t1, META_ARG_MUTABLE);
SamplerState g_texture_sampler : register(s1, META_ARG_CONSTANT);
#endif

PSInput CubeVS(VSInput input)
{
    const float4 position   = float4(input.position, 1.0F);

    PSInput output;
    output.position         = mul(position, g_mesh_uniforms.mvp_matrix);
    output.world_position   = mul(position, g_mesh_uniforms.model_matrix).xyz;
    output.world_normal     = normalize(mul(float4(input.normal, 0.0), g_mesh_uniforms.model_matrix).xyz);
#ifdef ENABLE_SHADOWS
    output.shadow_position  = mul(position, g_mesh_uniforms.shadow_mvpx_matrix);
#endif
#ifdef ENABLE_TEXTURING
    output.texcoord         = input.texcoord;
#endif

    return output;
}

float4 CubePS(PSInput input) : SV_TARGET
{
    const float3 fragment_to_light             = normalize(g_scene_uniforms.light_position.xyz - input.world_position);
    const float3 fragment_to_eye               = normalize(g_scene_uniforms.eye_position.xyz - input.world_position);
    const float3 light_reflected_from_fragment = reflect(-fragment_to_light, input.world_normal);

#ifdef ENABLE_SHADOWS
    const float3 light_proj_pos = input.shadow_position.xyz / input.shadow_position.w;
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

    const float  distance       = length(g_scene_uniforms.light_position.xyz - input.world_position);
    const float  diffuse_part   = clamp(dot(fragment_to_light, input.world_normal), 0.0, 1.0);
    const float4 diffuse_color  = base_color * diffuse_part / (distance * distance);

    const float  specular_part  = pow(clamp(dot(fragment_to_eye, light_reflected_from_fragment), 0.0, 1.0), g_constants.light_specular_factor);
    const float4 specular_color = base_color * specular_part / (distance * distance);

    return ColorLinearToSrgb(ambient_color + (1.F - shadow_ratio) * (diffuse_color + specular_color));
}
```

## CMake Build Configuration

Shaders are compiled in build time and are added as byte code to the application embedded resources.
Note that vertex shader `CubeVS` is built twice with different set of macro definitions:
one instance is used for shadow pass, the other is for final pass rendering.
Texture images are added to the application embedded resources too.

```cmake
include(MethaneApplications)
include(MethaneShaders)
include(MethaneResources)

add_methane_application(
    TARGET MethaneShadowCube
    NAME "Methane Shadow Cube"
    DESCRIPTION "Tutorial demonstrating shadow and final render passes done with Methane Kit."
    INSTALL_DIR "Apps"
    SOURCES
        ShadowCubeApp.h
        ShadowCubeApp.cpp
        Shaders/ShadowCubeUniforms.h
)

set(TEXTURES_DIR ${RESOURCES_DIR}/Textures)
list(APPEND TEXTURES
    ${TEXTURES_DIR}/MethaneBubbles.jpg
    ${TEXTURES_DIR}/MarbleWhite.jpg
)
add_methane_embedded_textures(MethaneShadowCube "${TEXTURES_DIR}" "${TEXTURES}")

add_methane_shaders_source(
    TARGET MethaneShadowCube
    SOURCE Shaders/ShadowCube.hlsl
    VERSION 6_0
    TYPES
        frag=CubePS:ENABLE_SHADOWS,ENABLE_TEXTURING
        vert=CubeVS:ENABLE_SHADOWS,ENABLE_TEXTURING
        vert=CubeVS:ENABLE_TEXTURING
)

add_methane_shaders_library(MethaneShadowCube)

target_link_libraries(MethaneShadowCube
    PRIVATE
    MethaneAppsCommon
)
```

## Continue learning

Continue learning Methane Graphics programming in the next tutorial [Typography](../05-Typography), which is demonstrating
text rendering using dynamic font atlas textures.