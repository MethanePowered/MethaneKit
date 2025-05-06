# Textured Cube Tutorial

| <pre><b>Windows (DirectX 12)       </pre></b>                         | <pre><b>Linux (Vulkan)             </pre></b>                    | <pre><b>MacOS (Metal)              </pre></b>                   | <pre><b>iOS (Metal)</pre></b>                                 |
|-----------------------------------------------------------------------|------------------------------------------------------------------|-----------------------------------------------------------------|---------------------------------------------------------------|
| ![Textured Cube on Windows](Screenshots/TexturedCubeWinDirectX12.jpg) | ![Textured Cube on Linux](Screenshots/TexturedCubeLinVulkan.jpg) | ![Textured Cube on MacOS](Screenshots/TexturedCubeMacMetal.jpg) | ![Textured Cube on iOS](Screenshots/TexturedCubeIOSMetal.jpg) |

This tutorial demonstrates textured cube rendering using Methane Kit:
- [TexturedCubeApp.h](TexturedCubeApp.h)
- [TexturedCubeApp.cpp](TexturedCubeApp.cpp)
- [Shaders/TexturedCubeUniforms.h](Shaders/TexturedCubeUniforms.h)
- [Shaders/TexturedCube.hlsl](Shaders/TexturedCube.hlsl)

Tutorial demonstrates the following Methane Kit features in addition to those demonstrated in 
[Hello Cube](../02-HelloCube):
- Use the base user interface application for graphics UI overlay rendering
- Create 2D textures with data loaded from images and create samplers
- Bind buffers and textures to program arguments and configure argument access modifiers
- Support SRGB gamma-correction in the textures loader and color transformation in pixel shaders

## Application Controls

Common keyboard controls are enabled by the `Platform`, `Graphics` and `UserInterface` application controllers:
- [Methane::Platform::AppController](/Modules/Platform/App/README.md#platform-application-controller)
- [Methane::Graphics::AppController, AppContextController](/Modules/Graphics/App/README.md#graphics-application-controllers)
- [Methane::UserInterface::AppController](/Modules/UserInterface/App/README.md#user-interface-application-controllers)

## Application and Frame Class Definitions

Let's start from the application header file `TexturedCubeApp.h`, which declares the application class `TexturedCubeApp` 
derived from the base class `UserInterface::App<TexturedCubeFrame>` and the frame class `TexturedCubeFrame` derived from 
the base class `Graphics::AppFrame`, similar to how it was done in the [Hello Cube](../02-HelloCube) tutorial. 
The difference here is the use of the [UserInterface::App](../../Modules/UserInterface/App) base class instead of the 
[Graphics::App](../../Modules/Graphics/App) class for visualizing optional UI elements and rendering them in a screen overlay.

```cpp
#pragma once

#include <Methane/Kit.h>
#include <Methane/UserInterface/App.hpp>

namespace Methane::Tutorials
{

...

namespace gfx = Methane::Graphics;
namespace rhi = Methane::Graphics::Rhi;

struct TexturedCubeFrame final : gfx::AppFrame
{
    // Volatile frame-dependent resources for rendering to dedicated frame-buffer in swap-chain
    ...

    using gfx::AppFrame::AppFrame;
};

using UserInterfaceApp = UserInterface::App<TexturedCubeFrame>;

class TexturedCubeApp final : public UserInterfaceApp
{
public:
    TexturedCubeApp();
    ~TexturedCubeApp() override;

    // GraphicsApp overrides
    void Init() override;
    bool Resize(const gfx::FrameSize& frame_size, bool is_minimized) override;
    bool Update() override;
    bool Render() override;

protected:
    // IContextCallback override
    void OnContextReleased(rhi::IContext& context) override;

private:
    bool Animate(double elapsed_seconds, double delta_seconds);

    // Global state members, rendering primitives and graphics resources 
    ...
};

} // namespace Methane::Tutorials
```

Methane Kit is designed to use the 
[deferred rendering approach](https://docs.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-render-multi-thread-render) 
with triple buffering to minimize waiting for frame-buffer releases in the swap-chain. To prepare graphics resource states 
ahead of the next frame's rendering, the `TexturedCubeFrame` structure keeps volatile frame-dependent resources used for 
rendering to a dedicated frame-buffer. It includes a program bindings object, a render command list for encoding render 
commands, and a set of command lists submitted for execution on the GPU via the command queue.

```cpp
struct TexturedCubeFrame final : Graphics::AppFrame
{
    rhi::ProgramBindings          program_bindings;
    rhi::IProgramArgumentBinding* uniforms_binding_ptr = nullptr;
    rhi::RenderCommandList        render_cmd_list;
    rhi::CommandListSet           execute_cmd_list_set;

    using gfx::AppFrame::AppFrame;
};
```

[Shaders/TexturedCubeUniforms.h](Shaders/TexturedCubeUniforms.h) header contains declaration of `Constants` and `Uniforms` 
structures with data saved in root constant buffers of the main program object.
Structures from this header are reused in [HLSL shader code](#textured-cube-shaders) and 16-byte packing in C++ is used
for common memory layout in HLSL and C++.

Uniform structures in [Shaders/TexturedCubeUniforms.h](Shaders/TexturedCubeUniforms.h):
```hlsl
struct Constants
{
    float4 light_color;
    float  light_power;
    float  light_ambient_factor;
    float  light_specular_factor;
    float  _padding;
};

struct Uniforms
{
    float3   eye_position;
    float3   light_position;
    float4x4 mvp_matrix;
    float4x4 model_matrix;
};
```

The size of the uniform buffer structures should be aligned to 16 bytes, which is defined with the `pragma pack` directive in 
the application header file [TexturedCubeApp.h](TexturedCubeApp.h). Uniforms data is stored in the application class member
`m_shader_uniforms` and is updated in the `TexturedCubeApp::Update()` method.

```cpp
namespace hlslpp
{
#pragma pack(push, 16)
#include "Shaders/TexturedCubeUniforms.h"
#pragma pack(pop)
}

namespace Methane::Tutorials
{

class TexturedCubeApp final : public UserInterfaceApp
{
    ...

private:
    hlslpp::Uniforms m_shader_uniforms { };
    gfx::Camera      m_camera;
    rhi::RenderState m_render_state;
    rhi::BufferSet   m_vertex_buffer_set;
    rhi::Buffer      m_index_buffer;
    rhi::Texture     m_cube_texture;
    rhi::Sampler     m_texture_sampler;
};

} // namespace Methane::Tutorials
```

Default values of the shader constants are defined in the application source file and
checked in [TexturedCubeApp.cpp](TexturedCubeApp.cpp) and size alignment is checked with static asserts:

```cpp
const hlslpp::Constants g_shader_constants{
    { 1.F, 1.F, 0.74F, 1.F },  // - light_color
    700.F,                     // - light_power
    0.04F,                     // - light_ambient_factor
    30.F                       // - light_specular_factor
};

static_assert(sizeof(hlslpp::Constants) % 16 == 0, "Size of Constants struct should have 16 byte alignment!");
static_assert(sizeof(hlslpp::Uniforms) % 16 == 0,  "Size of Uniforms struct should have 16 byte alignment!");
```

## Application Construction and Initialization

Application is created with constructor defined in [TexturedCubeApp.cpp](TexturedCubeApp.cpp). Graphics application settings 
are generated by utility function `GetGraphicsTutorialAppSettings(...)` defined in 
[Methane/Tutorials/AppSettings.hpp](../../Common/Include/Methane/Tutorials/AppSettings.hpp). Camera orientation is reset to 
the default state. Camera and light rotating animation is added to the animation pool bound to `TexturedCubeApp::Animate` 
function described below.

```cpp
TexturedCubeApp::TexturedCubeApp()
    : UserInterfaceApp(
        GetGraphicsTutorialAppSettings("Methane Textured Cube", AppOptions::GetDefaultWithColorOnlyAndAnim()),
        GetUserInterfaceTutorialAppSettings(AppOptions::GetDefaultWithColorOnlyAndAnim()),
        "Methane tutorial of textured cube rendering")
{
    m_shader_uniforms.light_position = hlslpp::float3(0.F, 20.F, -25.F);
    m_shader_uniforms.model_matrix = hlslpp::float4x4::scale(15.F);

    m_camera.ResetOrientation({ { 13.0F, 13.0F, -13.0F }, { 0.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } });

    // Setup animations
    GetAnimations().emplace_back(std::make_shared<Data::TimeAnimation>(std::bind(&TexturedCubeApp::Animate, this, std::placeholders::_1, std::placeholders::_2)));
}
```

## Graphics Resources Initialization

Cube vertex structure is defined with fields for position, normal, and texture coordinates, as well as an auxiliary layout 
description used for automatic mesh vertex data generation.

```cpp
struct CubeVertex
{
    gfx::Mesh::Position position;
    gfx::Mesh::Normal   normal;
    gfx::Mesh::TexCoord texcoord;

    inline static const gfx::Mesh::VertexLayout layout{
        gfx::Mesh::VertexField::Position,
        gfx::Mesh::VertexField::Normal,
        gfx::Mesh::VertexField::TexCoord,
    };
};
```

Initialization of the `UserInterface::App` resources is done with the base class `UserInterface::Init()` method. Initial camera 
projection size is set with the `m_camera.Resize(...)` call by passing the frame size from the context settings, initialized in the 
base class `Graphics::App::InitContext(...)`.

Vertices and indices data of the cube mesh are generated with the `Graphics::CubeMesh<CubeVertex>` template class defined using the 
vertex structure with the layout description defined above. Vertex and index buffers are created with the 
`GetRenderContext().CreateBuffer(...)` factory method using `rhi::BufferSettings::ForVertexBuffer(...)` and 
`rhi::BufferSettings::ForIndexBuffer(...)` settings. Generated data is copied to buffers with the `rhi::Buffer::SetData(...)` call, 
which takes a sub-resource derived from the `Data::Chunk` class describing a continuous memory range and holding its data.

```cpp
void TexturedCubeApp::Init()
{
    UserInterfaceApp::Init();

    const rhi::CommandQueue render_cmd_queue = GetRenderContext().GetRenderCommandKit().GetQueue();
    m_camera.Resize(GetRenderContext().GetSettings().frame_size);

    // Create vertex buffer for cube mesh
    const gfx::CubeMesh<CubeVertex> cube_mesh(CubeVertex::layout);
    const Data::Size vertex_data_size   = cube_mesh.GetVertexDataSize();
    const Data::Size  vertex_size       = cube_mesh.GetVertexSize();
    rhi::Buffer vertex_buffer = GetRenderContext().CreateBuffer(rhi::BufferSettings::ForVertexBuffer(vertex_data_size, vertex_size));
    vertex_buffer.SetName("Cube Vertex Buffer");
    vertex_buffer.SetData(render_cmd_queue, {
        reinterpret_cast<Data::ConstRawPtr>(cube_mesh.GetVertices().data()),
        vertex_data_size
    });
    m_vertex_buffer_set = rhi::BufferSet(rhi::BufferType::Vertex, { vertex_buffer });

    // Create index buffer for cube mesh
    const Data::Size index_data_size    = cube_mesh.GetIndexDataSize();
    const gfx::PixelFormat index_format = gfx::GetIndexFormat(cube_mesh.GetIndex(0));
    m_index_buffer = GetRenderContext().CreateBuffer(rhi::BufferSettings::ForIndexBuffer(index_data_size, index_format));
    m_index_buffer.SetName("Cube Index Buffer");
    m_index_buffer.SetData(render_cmd_queue, {
        reinterpret_cast<Data::ConstRawPtr>(cube_mesh.GetIndices().data()),
        index_data_size
    });

    ...
}
```

Cube face texture is created using the `Graphics::ImageLoader` class available via the `Graphics::App::GetImageLoader()` function.
The texture is loaded from a JPEG image embedded in the application resources by the path in the embedded file system 
`MethaneBubbles.jpg`. The image is added to the application resources at build time and is [configured in CMakeLists.txt](#cmake-build-configuration).
The `Graphics::ImageOptionMask` is passed to the image loader function to request mipmaps generation and to use the SRGB color format.

The `rhi::Sampler` object is created with the `GetRenderContext().CreateSampler(...)` function, which defines the parameters of 
texture sampling from the shader.

```cpp
void TexturedCubeApp::Init()
{
    ...

    // Load texture image from file
    constexpr gfx::ImageOptionMask image_options({ gfx::ImageOption::Mipmapped, gfx::ImageOption::SrgbColorSpace });
    m_cube_texture = GetImageLoader().LoadImageToTexture2D(render_cmd_queue, "MethaneBubbles.jpg", image_options, "Cube Face Texture");

    // Create sampler for image texture
    m_texture_sampler = GetRenderContext().CreateSampler(
        rhi::Sampler::Settings
        {
            .filter  = rhi::Sampler::Filter  { rhi::Sampler::Filter::MinMag::Linear },
            .address = rhi::Sampler::Address { rhi::Sampler::Address::Mode::ClampToEdge }
        }
    );

    ...
}
```

`rhi::Program` object is created in the `rhi::Program::Settings` structure using the `GetRenderContext().CreateProgram(...)` 
factory method. Vertex and Pixel shaders are created and loaded from embedded resources as pre-compiled byte-code. 
It is important to note that render state settings enable depth testing for the correct rendering of cube faces. 
Finally, the render state is created using the settings structure via the `GetRenderContext().CreateRenderState(...)` 
factory method.

`rhi::ProgramArgumentAccessors{ ... }` in `rhi::Program::Settings` overrides program argument accessor definitions:
- The uniform argument `g_uniforms` of all shaders is defined as a root constant buffer automatically managed by the program 
with a frame-constant access pattern (one buffer per frame is used). Note that the `FRAME_CONSTANT` access pattern defined 
in C++ should match the `META_ARG_FRAME_CONSTANT` space modifier defined in the shaders HLSL.
- The uniform argument `g_constants` of the pixel shader is also defined as a root constant buffer with a constant access 
pattern (a single buffer is used for all frames in the swap-chain). Note that the `CONSTANT` access pattern defined in C++ 
should match the `META_ARG_CONSTANT` space modifier defined in the shaders HLSL.
- Other program arguments `g_texture` and `g_sampler` are not overridden in the program settings and are used with default 
access patterns defined in the shaders HLSL.

```cpp
void TexturedCubeApp::Init()
{
    ...

    // Create render state with program
    m_render_state = GetRenderContext().CreateRenderState(
        rhi::RenderState::Settings
        {
            .program = GetRenderContext().CreateProgram(
                rhi::Program::Settings
                {
                    .shader_set = rhi::Program::ShaderSet
                    {
                        { rhi::ShaderType::Vertex, { Data::ShaderProvider::Get(), { "TexturedCube", "CubeVS" } } },
                        { rhi::ShaderType::Pixel,  { Data::ShaderProvider::Get(), { "TexturedCube", "CubePS" } } },
                    },
                    .input_buffer_layouts = rhi::ProgramInputBufferLayouts
                    {
                        rhi::Program::InputBufferLayout
                        {
                            rhi::Program::InputBufferLayout::ArgumentSemantics { cube_mesh.GetVertexLayout().GetSemantics() }
                        }
                    },
                    .argument_accessors = rhi::ProgramArgumentAccessors
                    {
                        META_PROGRAM_ARG_ROOT_BUFFER_CONSTANT(rhi::ShaderType::Pixel, "g_constants"),
                        META_PROGRAM_ARG_ROOT_BUFFER_FRAME_CONSTANT(rhi::ShaderType::All, "g_uniforms")
                    },
                    .attachment_formats = GetScreenRenderPattern().GetAttachmentFormats()
                }
            ),
            .render_pattern = GetScreenRenderPattern()
        }
    );

    ...
}
```

Final part of initialization is related to frame-dependent resources, creating independent resource objects for each frame in 
the swap-chain:
- Create program arguments to resources bindings with `m_render_state.GetProgram().CreateBindings(..)` function. Note that 
  the program argument `g_uniforms` binding is not initialized here, and its pointer is saved to the `uniforms_binding_ptr` 
  field for later initialization in the `TexturedCubeApp::Update()` method.
- Create a rendering command list with `render_cmd_queue.CreateRenderCommandList(...)` and create a set of command lists 
  with `rhi::CommandListSet(...)` for execution in the command queue.

Finally, at the end of the `Init()` function, `App::CompleteInitialization()` is called to complete graphics resources 
initialization to prepare for rendering. It uploads graphics resources to the GPU and initializes shader bindings on the GPU.

```cpp
void TexturedCubeApp::Init()
{    
    ...

    // Create frame buffer resources
    for(TexturedCubeFrame& frame : GetFrames())
    {
        // Configure program resource bindings
        frame.program_bindings = m_render_state.GetProgram().CreateBindings({
            { { rhi::ShaderType::Pixel, "g_constants" }, rhi::RootConstant(g_shader_constants) },
            { { rhi::ShaderType::Pixel, "g_texture"   }, m_cube_texture.GetResourceView() },
            { { rhi::ShaderType::Pixel, "g_sampler"   }, m_texture_sampler.GetResourceView() }
        }, frame.index);
        frame.uniforms_binding_ptr = &frame.program_bindings.Get({ rhi::ShaderType::All, "g_uniforms" });

        // Create command list for rendering
        frame.render_cmd_list = render_cmd_queue.CreateRenderCommandList(frame.screen_pass);
        frame.execute_cmd_list_set = rhi::CommandListSet({ frame.render_cmd_list.GetInterface() }, frame.index);
    }

    UserInterfaceApp::CompleteInitialization();
}
```

`TexturedCubeApp::OnContextReleased` callback method releases all graphics resources before the graphics context is released.
This is necessary when the graphics device is switched via 
[Graphics::AppContextController](../../Modules/Graphics/App/README.md#graphicsappcontextcontrollerincludemethanegraphicsappcontextcontrollerh)
using the `LCtrl + X` shortcut.

```cpp
void TexturedCubeApp::OnContextReleased(gfx::Context& context)
{
    m_texture_sampler = {};
    m_cube_texture = {};
    m_index_buffer = {};
    m_vertex_buffer_set = {};
    m_render_state = {};

    UserInterfaceApp::OnContextReleased(context);
}
```

## Frame Rendering Cycle

Animation function bound to time-animation is called automatically as part of every render cycle, just before the 
`App::Update` function call. This function rotates the light position and camera in opposite directions.

```cpp
bool TexturedCubeApp::Animate(double, double delta_seconds)
{
    const float rotation_angle_rad = static_cast<float>(delta_seconds * 360.F / 4.F) * gfx::ConstFloat::RadPerDeg;
    const hlslpp::float3x3 light_rotate_matrix = hlslpp::float3x3::rotation_axis(m_camera.GetOrientation().up, rotation_angle_rad);
    m_shader_uniforms.light_position = hlslpp::mul(m_shader_uniforms.light_position, light_rotate_matrix);
    m_camera.Rotate(m_camera.GetOrientation().up, static_cast<float>(delta_seconds * 360.F / 8.F));
    return true;
}
```

`TexturedCubeApp::Update()` function is called before the `App::Render()` call to update shader uniforms with model-view-project 
(MVP) matrices and eye position based on the current camera orientation, updated in animation. The root constant buffer is updated 
with this data using the program argument binding method `rhi::IProgramArgumentBinding::SetRootConstant(...)`.

```cpp
bool TexturedCubeApp::Update()
{
    if (!UserInterfaceApp::Update())
        return false;

    // Update Model, View, Projection matrices based on camera location
    m_shader_uniforms.mvp_matrix   = hlslpp::transpose(hlslpp::mul(m_shader_uniforms.model_matrix, m_camera.GetViewProjMatrix()));
    m_shader_uniforms.eye_position = m_camera.GetOrientation().eye;

    GetCurrentFrame().uniforms_binding_ptr->SetRootConstant(rhi::RootConstant(m_shader_uniforms));
    return true;
}
```

`TexturedCubeApp::Render()` method is called after all. Initial base method `UserInterfaceApp::Render()` call waits for 
previously current frame buffer presenting to be completed. When the frame buffer is free, new frame rendering can be started:
1. Render command list encoding starts with `rhi::RenderCommandList::Reset(...)` call taking render state object and optional 
   debug group, which defines a named region in the commands sequence.
    1. View state is set with viewports and scissor rects.
    2. Program bindings are set.
    3. Vertex buffers set is set.
    4. Indexed draw call is issued.
2. `UserInterface::App::RenderOverlay(...)` is called to record UI drawing command in the render command list.
3. Render command list is committed and passed to `rhi::CommandQueue::Execute` call for execution on the GPU.
4. `RenderContext::Present()` is called to schedule frame buffer presenting to the screen.

```cpp
bool TexturedCubeApp::Render()
{
    if (!UserInterfaceApp::Render())
        return false;

    const TexturedCubeFrame& frame = GetCurrentFrame();

    // Issue commands for cube rendering
    META_DEBUG_GROUP_VAR(s_debug_group, "Cube Rendering");
    frame.render_cmd_list.ResetWithState(m_render_state, &s_debug_group);
    frame.render_cmd_list.SetViewState(GetViewState());
    frame.render_cmd_list.SetProgramBindings(frame.program_bindings);
    frame.render_cmd_list.SetVertexBuffers(m_vertex_buffer_set);
    frame.render_cmd_list.SetIndexBuffer(m_index_buffer);
    frame.render_cmd_list.DrawIndexed(rhi::RenderPrimitive::Triangle);

    RenderOverlay(frame.render_cmd_list);

    // Execute command list on render queue and present frame to screen
    frame.render_cmd_list.Commit();
    GetRenderContext().GetRenderCommandKit().GetQueue().Execute(frame.execute_cmd_list_set);
    GetRenderContext().Present();

    return true;
}
```

Graphics render loop is started from `main(...)` entry function using `GraphicsApp::Run(...)` method which is also parsing command line arguments.

```cpp
int main(int argc, const char* argv[])
{
    return TexturedCubeApp().Run({ argc, argv });
}
```

## Textured Cube Shaders

HLSL 6 shaders [Shaders/Cube.hlsl](Shaders/Cube.hlsl) implement Phong shading with texturing. SRGB gamma-correction is implemented 
with the `ColorLinearToSrgb(...)` function from [Common/Shaders/Primitives.hlsl](../Common/Shaders/Primitives.hlsl), which converts 
the final color from linear-space to SRGB color-space.

```cpp
#include "TexturedCubeUniforms.h"
#include "../../Common/Shaders/Primitives.hlsl"

struct VSInput
{
    float3 position         : POSITION;
    float3 normal           : NORMAL;
    float2 texcoord         : TEXCOORD;
};

struct PSInput
{
    float4 position         : SV_POSITION;
    float3 world_position   : POSITION;
    float3 world_normal     : NORMAL;
    float2 texcoord         : TEXCOORD;
};

ConstantBuffer<Constants> g_constants : register(b0, META_ARG_CONSTANT);
ConstantBuffer<Uniforms>  g_uniforms  : register(b1, META_ARG_FRAME_CONSTANT);
Texture2D                 g_texture   : register(t0, META_ARG_CONSTANT);
SamplerState              g_sampler   : register(s0, META_ARG_CONSTANT);

PSInput CubeVS(VSInput input)
{
    const float4 position = float4(input.position, 1.F);

    PSInput output;
    output.position       = mul(position, g_uniforms.mvp_matrix);
    output.world_position = mul(position, g_uniforms.model_matrix).xyz;
    output.world_normal   = normalize(mul(float4(input.normal, 0.F), g_uniforms.model_matrix).xyz);
    output.texcoord       = input.texcoord;

    return output;
}

float4 CubePS(PSInput input) : SV_TARGET
{
    const float3 fragment_to_light  = normalize(g_uniforms.light_position - input.world_position);
    const float3 fragment_to_eye    = normalize(g_uniforms.eye_position.xyz - input.world_position);
    const float3 light_reflected_from_fragment = reflect(-fragment_to_light, input.world_normal);

    const float4 texel_color    = g_texture.Sample(g_sampler, input.texcoord);
    const float4 ambient_color  = texel_color * g_constants.light_ambient_factor;
    const float4 base_color     = texel_color * g_constants.light_color * g_constants.light_power;

    const float  distance       = length(g_uniforms.light_position - input.world_position);
    const float  diffuse_part   = clamp(dot(fragment_to_light, input.world_normal), 0.0, 1.0);
    const float4 diffuse_color  = base_color * diffuse_part / (distance * distance);

    const float  specular_part  = pow(clamp(dot(fragment_to_eye, light_reflected_from_fragment), 0.0, 1.0), g_constants.light_specular_factor);
    const float4 specular_color = base_color * specular_part / (distance * distance);;

    return ColorLinearToSrgb(ambient_color + diffuse_color + specular_color);
}
```

## CMake Build Configuration

CMake build configuration [CMakeLists.txt](CMakeLists.txt) of the application is powered by the included Methane CMake 
modules:
- [MethaneApplications.cmake](../../CMake/MethaneApplications.cmake) - defines function `add_methane_application`
- [MethaneShaders.cmake](../../CMake/MethaneShaders.cmake) - defines function `add_methane_shaders`
- [MethaneResources.cmake](../../CMake/MethaneResources.cmake) - defines functions `add_methane_embedded_textures` and 
`add_methane_copy_textures`

Shaders are compiled at build time and added as byte code to the application's embedded resources. Texture images are also 
added to the application's embedded resources.

```cmake
include(MethaneApplications)
include(MethaneShaders)
include(MethaneResources)

add_methane_application(
    TARGET MethaneTexturedCube
    NAME "Methane Textured Cube"
    DESCRIPTION "Tutorial demonstrating textured rotating cube rendering with Methane Kit."
    INSTALL_DIR "Apps"
    SOURCES
        TexturedCubeApp.h
        TexturedCubeApp.cpp
        Shaders/TexturedCubeUniforms.h
)

set(TEXTURES_DIR ${RESOURCES_DIR}/Textures)
set(TEXTURES ${TEXTURES_DIR}/MethaneBubbles.jpg)
add_methane_embedded_textures(MethaneTexturedCube "${TEXTURES_DIR}" "${TEXTURES}")

add_methane_shaders_source(
    TARGET MethaneTexturedCube
    SOURCE Shaders/TexturedCube.hlsl
    VERSION 6_0
    TYPES
        frag=CubePS
        vert=CubeVS
)

add_methane_shaders_library(MethaneTexturedCube)

target_link_libraries(MethaneTexturedCube
    PRIVATE
        MethaneAppsCommon
)
```

## Continue learning

Continue learning Methane Graphics programming in the next tutorial [Shadow Cube](../04-ShadowCube), 
which is demonstrating multi-pass rendering for drawing simple shadows.
