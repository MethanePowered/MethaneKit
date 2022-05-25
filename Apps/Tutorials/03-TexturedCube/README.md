# Textured Cube Tutorial

| Windows (DirectX 12) | MacOS (Metal) | Linux (Vulkan)                                                   |
| -------------------- | ------------- |------------------------------------------------------------------|
| ![Textured Cube on Windows](Screenshots/TexturedCubeWinDirectX12.jpg) | ![Textured Cube on MacOS](Screenshots/TexturedCubeMacMetal.jpg) | ![Textured Cube on Linux](Screenshots/TexturedCubeLinVulkan.jpg) |

This tutorial demonstrates textured cube rendering using Methane Kit:
- [TexturedCubeApp.h](TexturedCubeApp.h)
- [TexturedCubeApp.cpp](TexturedCubeApp.cpp)
- [Shaders/TexturedCubeUniforms.h](Shaders/TexturedCubeUniforms.h)
- [Shaders/TexturedCube.hlsl](Shaders/TexturedCube.hlsl)

Tutorial demonstrates the following Methane Kit features additionally to demonstrated in [Hello Cube](../02-HelloCube):
- Base user interface application implementing graphics UI overlay rendering
- Creating 2D textures with data loaded data from images and creating samplers
- Binding buffers and textures to program arguments and configuring argument modifiers
- SRGB gamma-correction support in textures loader and color transformation in pixel shaders

## Application and Frame Class Definitions

Let's start from the application header file [TexturedCubeApp.h](TexturedCubeApp.h) which declares
application class `TexturedCubeApp` derived from the base class `UserInterface::App<TexturedCubeFrame>`
and frame class `TexturedCubeFrame` derived from the base class `Graphics::AppFrame` 
similar to how it was done in [HelloCube](../02-HelloCube) tutorial.
The difference here is the [UserInterface::App](../../../Modules/UserInterface/App) base class used instead of
[Graphics::App](../../../Modules/Graphics/App) class for visualization of optional UI elements and rendering it in screen overlay.

```cpp
#pragma once

#include <Methane/Kit.h>
#include <Methane/UserInterface/App.hpp>

namespace Methane::Tutorials
{

...

namespace gfx = Methane::Graphics;

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
    void OnContextReleased(gfx::Context& context) override;

private:
    bool Animate(double elapsed_seconds, double delta_seconds);

    // Global state members, rendering primitives and graphics resources 
    ...
};

} // namespace Methane::Tutorials
```

Methane Kit is designed to use [deferred rendering approach](https://docs.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-render-multi-thread-render) with triple buffering for minimized waiting for frame-buffers getting
released in swap-chain.
In order to prepare graphics resource states ahead of next frames rendering, `TexturedCubeFrame` structure keeps 
volatile frame dependent resources used for rendering to dedicated frame-buffer. It includes uniforms buffer and 
program bindings objects as well as render command list for render commands encoding
and a set of command lists submitted for execution on GPU via command queue.

```cpp
struct TexturedCubeFrame final : Graphics::AppFrame
{
    Ptr<gfx::Buffer>            uniforms_buffer_ptr;
    Ptr<gfx::ProgramBindings>   program_bindings_ptr;
    Ptr<gfx::RenderCommandList> render_cmd_list_ptr;
    Ptr<gfx::CommandListSet>    execute_cmd_list_set_ptr;

    using gfx::AppFrame::AppFrame;
};
```

[Shaders/TexturedCubeUniforms.h](Shaders/TexturedCubeUniforms.h) header contains declaration of `Constants` and `Uniforms` structures with data saved in constants buffer `m_const_buffer_ptr` field of `TexturedCubeApp` class below and uniforms buffer `uniforms_buffer_ptr` field of `TexturedCubeFrame` structure above.
Structures from this header are reused in [HLSL shader code](#textured-cube-shaders) and 16-byte packing in C++ is used
gor common memory layout in HLSL and C++.

Uniform structures in [Shaders/TexturedCubeUniforms.h](Shaders/TexturedCubeUniforms.h):
```hlsl
struct Constants
{
    float4 light_color;
    float  light_power;
    float  light_ambient_factor;
    float  light_specular_factor;
};

struct Uniforms
{
    float3   eye_position;
    float3   light_position;
    float4x4 mvp_matrix;
    float4x4 model_matrix;
};
```

```cpp
namespace hlslpp
{
#pragma pack(push, 16)
#include "Shaders/TexturedCubeUniforms.h"
#pragma pack(pop)
}

class TexturedCubeApp final : public UserInterfaceApp
{
    ...

private:
    const float             m_cube_scale = 15.F;
    const hlslpp::Constants m_shader_constants{
        { 1.F, 1.F, 0.74F, 1.F },  // - light_color
        700.F,                     // - light_power
        0.04F,                     // - light_ambient_factor
        30.F                       // - light_specular_factor
    };
    hlslpp::Uniforms        m_shader_uniforms { };
    gfx::Camera             m_camera;
    Ptr<gfx::RenderState>   m_render_state_ptr;
    Ptr<gfx::BufferSet>     m_vertex_buffer_set_ptr;
    Ptr<gfx::Buffer>        m_index_buffer_ptr;
    Ptr<gfx::Buffer>        m_const_buffer_ptr;
    Ptr<gfx::Texture>       m_cube_texture_ptr;
    Ptr<gfx::Sampler>       m_texture_sampler_ptr;

    const gfx::Resource::SubResources m_shader_uniforms_subresources{
        { reinterpret_cast<Data::ConstRawPtr>(&m_shader_uniforms), sizeof(hlslpp::Uniforms) }
    };
};
```

## Application Construction and Initialization

Application is created with constructor defined in [TexturedCubeApp.cpp](TexturedCubeApp.cpp).
Graphics application settings are generated by utility function `Samples::GetGraphicsAppSettings(...)` 
defined in [Methane/Samples/AppSettings.hpp](../../Common/Include/Methane/Samples/AppSettings.hpp).
Camera orientation is reset to the default state. Camera and light rotating animation is added to the animation pool bound to 
`TexturedCubeApp::Animate` function described below.

```cpp
TexturedCubeApp::TexturedCubeApp()
    : UserInterfaceApp(
        Samples::GetGraphicsAppSettings("Methane Textured Cube", Samples::g_default_app_options_color_only_and_anim), {},
        "Methane tutorial of textured cube rendering")
{
    m_shader_uniforms.light_position = hlslpp::float3(0.F, 20.F, -25.F);
    m_camera.ResetOrientation({ { 13.0F, 13.0F, -13.0F }, { 0.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } });

    m_shader_uniforms.model_matrix = hlslpp::float4x4::scale(m_cube_scale);

    // Setup animations
    GetAnimations().emplace_back(std::make_shared<Data::TimeAnimation>(std::bind(&TexturedCubeApp::Animate, this, std::placeholders::_1, std::placeholders::_2)));
}
```

## Graphics Resources Initialization

Cube vertex structure is defined with fields for position, normal and texture coordinates, as well auxiliary
layout description used for automatic mesh vertex data generation.

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

Initialization of the `UserInterface::App` resources is done with base class `UserInterface::Init()` call.
Initial camera projection size is set with `m_camera.Resize(...)` call by passing frame size from the context settings,
initialized in the base class `Graphics::App::InitContext(...)`.

Vertices and indices data of the cube mesh are generated with `Graphics::CubeMesh<CubeVertex>` template class defined
using vertex structure with layout description defined above. Vertex and index buffers are created with 
`Graphics::Buffer::CreateVertexBuffer(...)` and `Graphics::Buffer::CreateIndexBuffer(...)` factory functions and generated data
is filled to buffers with `Graphics::Buffer::SetData(...)` call, which is taking a collection of sub-resources,
where every subresource is derived from `Data::Chunk` and describes a continuous memory range 
as well as `Graphics::Resource::SubResource::Index` which is pointing to related part of resource.

Similarly constants buffer is created with `Graphics::Buffer::CreateConstantBuffer(...)` and filled with data from member
variable `m_shader_constants`.

```cpp
void TexturedCubeApp::Init()
{
    UserInterfaceApp::Init();

    m_camera.Resize(GetRenderContext().GetSettings().frame_size);

    // Create vertex buffer for cube mesh
    const gfx::CubeMesh<CubeVertex> cube_mesh(CubeVertex::layout);
    const Data::Size vertex_data_size = cube_mesh.GetVertexDataSize();
    const Data::Size vertex_size      = cube_mesh.GetVertexSize();
    Ptr<gfx::Buffer> vertex_buffer_ptr = gfx::Buffer::CreateVertexBuffer(GetRenderContext(), vertex_data_size, vertex_size);
    vertex_buffer_ptr->SetData({ { reinterpret_cast<Data::ConstRawPtr>(cube_mesh.GetVertices().data()), vertex_data_size } });
    m_vertex_buffer_set_ptr = gfx::BufferSet::CreateVertexBuffers({ *vertex_buffer_ptr });

    // Create index buffer for cube mesh
    const Data::Size index_data_size = cube_mesh.GetIndexDataSize();
    m_index_buffer_ptr = gfx::Buffer::CreateIndexBuffer(GetRenderContext(), index_data_size, gfx::GetIndexFormat(cube_mesh.GetIndex(0)));
    m_index_buffer_ptr->SetData({ { reinterpret_cast<Data::ConstRawPtr>(cube_mesh.GetIndices().data()), index_data_size } });

    // Create constants buffer for frame rendering
    const auto constants_data_size = static_cast<Data::Size>(sizeof(m_shader_constants));
    m_const_buffer_ptr = gfx::Buffer::CreateConstantBuffer(GetRenderContext(), constants_data_size);
    m_const_buffer_ptr->SetData({ { reinterpret_cast<Data::ConstRawPtr>(&m_shader_constants), constants_data_size } });

    ...
}
```

Cube face texture is created using `Graphics::ImageLoader` class through the instance provided by 
`Graphics::App::GetImageLoader()` function. Texture is loaded from JPEG image embedded in application resources
by path in embedded file system `MethaneBubbles.jpg`. Image is added to application resources in build time and
[configured in CMakeLists.txt](#cmake-build-configuration). `Graphics::ImageLoader::Options` is passed to
image loading function to request mipmaps generation and using SRGB texture format.

Sampler object is created with `Graphics::Sampler::Create(...)` function which defines
parameters of texture sampling from shader.

```cpp
void TexturedCubeApp::Init()
{
    ...

    // Load texture image from file
    using namespace magic_enum::bitwise_operators;
    const gfx::ImageLoader::Options image_options = gfx::ImageLoader::Options::Mipmapped
                                                  | gfx::ImageLoader::Options::SrgbColorSpace;
    m_cube_texture_ptr = GetImageLoader().LoadImageToTexture2D(GetRenderContext(), "MethaneBubbles.jpg", image_options, "Cube Face Texture");

    // Create sampler for image texture
    m_texture_sampler_ptr = gfx::Sampler::Create(GetRenderContext(),
        gfx::Sampler::Settings
        {
            gfx::Sampler::Filter  { gfx::Sampler::Filter::MinMag::Linear },
            gfx::Sampler::Address { gfx::Sampler::Address::Mode::ClampToEdge }
        }
    );

    ...
}
```

`Graphics::Program` object is created in `Graphics::RenderState::Settings` structure using `Graphics::Program::Create(...)` factory function.
Vertex and Pixel shaders are created and loaded from embedded resources as pre-compiled byte-code.
Program settings also includes additional description `Graphics::Program::ArgumentAccessors` of program arguments bound to graphics resources.
Argument description define specific access modifiers for program arguments used in `Graphics::ProgramBindings` object.
Also it is important to note that render state settings enables depth testing for correct rendering of cube faces.
Finally, render state is created using filled settings structure with `Graphics::RenderState::Create(...)` factory function.

```cpp
void TexturedCubeApp::Init()
{
    ...

    // Create render state with program
    m_render_state_ptr = gfx::RenderState::Create(GetRenderContext(),
        gfx::RenderState::Settings
        {
            gfx::Program::Create(GetRenderContext(),
                gfx::Program::Settings
                {
                    gfx::Program::Shaders
                    {
                        gfx::Shader::CreateVertex(GetRenderContext(), { Data::ShaderProvider::Get(), { "TexturedCube", "CubeVS" } }),
                        gfx::Shader::CreatePixel( GetRenderContext(), { Data::ShaderProvider::Get(), { "TexturedCube", "CubePS" } }),
                    },
                    gfx::Program::InputBufferLayouts
                    {
                        gfx::Program::InputBufferLayout
                        {
                            gfx::Program::InputBufferLayout::ArgumentSemantics { cube_mesh.GetVertexLayout().GetSemantics() }
                        }
                    },
                    gfx::Program::ArgumentAccessors
                    {
                        { { gfx::Shader::Type::All,   "g_uniforms"  }, gfx::Program::ArgumentAccessor::Type::FrameConstant },
                        { { gfx::Shader::Type::Pixel, "g_constants" }, gfx::Program::ArgumentAccessor::Type::Constant },
                        { { gfx::Shader::Type::Pixel, "g_texture"   }, gfx::Program::ArgumentAccessor::Type::Constant },
                        { { gfx::Shader::Type::Pixel, "g_sampler"   }, gfx::Program::ArgumentAccessor::Type::Constant },
                    },
                    GetScreenRenderPattern().GetAttachmentFormats()
                }
            ),
            GetScreenRenderPatternPtr()
        }
    );

    ...
}
```

Final part of initialization is related to frame-dependent resources, creating independent resource objects for each frame in swap-chain:
- Create uniforms buffer with `Buffer::CreateConstantBuffer(...)` function.
- Create program arguments to resources bindings with `ProgramBindings::Create(..)` function.
- Create rendering command list with `RenderCommandList::Create(...)` and 
create set of command lists with `CommandListSet::Create(...)` for execution in command queue.

Finally at the end of `Init()` function `App::CompleteInitialization()` is called to complete graphics
resources initialization to prepare for rendering. It uploads graphics resources to GPU and initializes shader bindings on GPU.

```cpp
void TexturedCubeApp::Init()
{    
    ...

    // Create frame buffer resources
    const auto uniforms_data_size = static_cast<Data::Size>(sizeof(m_shader_uniforms));
    for(TexturedCubeFrame& frame : GetFrames())
    {
        // Create uniforms buffer with volatile parameters for frame rendering
        frame.uniforms_buffer_ptr = gfx::Buffer::CreateConstantBuffer(GetRenderContext(), uniforms_data_size, false, true);

        // Configure program resource bindings
        frame.program_bindings_ptr = gfx::ProgramBindings::Create(m_render_state_ptr->GetSettings().program_ptr, {
            { { gfx::Shader::Type::All,   "g_uniforms"  }, { { *frame.uniforms_buffer_ptr } } },
            { { gfx::Shader::Type::Pixel, "g_constants" }, { { *m_const_buffer_ptr        } } },
            { { gfx::Shader::Type::Pixel, "g_texture"   }, { { *m_cube_texture_ptr        } } },
            { { gfx::Shader::Type::Pixel, "g_sampler"   }, { { *m_texture_sampler_ptr     } } },
        }, frame.index);
        
        // Create command list for rendering
        frame.render_cmd_list_ptr = gfx::RenderCommandList::Create(GetRenderContext().GetRenderCommandKit().GetQueue(), *frame.screen_pass_ptr);
        frame.execute_cmd_list_set_ptr = gfx::CommandListSet::Create({ *frame.render_cmd_list_ptr });
    }e.execute_cmd_list_set_ptr = gfx::CommandListSet::Create({ *frame.render_cmd_list_ptr }, frame.index);
    }

    UserInterfaceApp::CompleteInitialization();
}
```

`TexturedCubeApp::OnContextReleased` callback method releases all graphics resources before graphics context is released,
for example when graphics device is changed via [Graphics::AppContextController](../../../Modules/Graphics/App/README.md#graphicsappcontextcontrollerincludemethanegraphicsappcontextcontrollerh)
with `LCtrl + X` shortcut.

```cpp
void TexturedCubeApp::OnContextReleased(gfx::Context& context)
{
    m_texture_sampler_ptr.reset();
    m_cube_texture_ptr.reset();
    m_const_buffer_ptr.reset();
    m_index_buffer_ptr.reset();
    m_vertex_buffer_set_ptr.reset();
    m_render_state_ptr.reset();

    UserInterfaceApp::OnContextReleased(context);
}
```

## Frame Rendering Cycle

Animation function bound to time-animation is called automatically as a part of every render cycle, just before `App::Update` function call.
This function rotates light position and camera in opposite directions.

```cpp
bool TexturedCubeApp::Animate(double, double delta_seconds)
{
    const float rotation_angle_rad = static_cast<float>(delta_seconds * 360.F / 4.F) * gfx::ConstFloat::RadPerDeg;
    hlslpp::float3x3 light_rotate_matrix = hlslpp::float3x3::rotation_axis(m_camera.GetOrientation().up, rotation_angle_rad);
    m_shader_uniforms.light_position = hlslpp::mul(m_shader_uniforms.light_position, light_rotate_matrix);
    m_camera.Rotate(m_camera.GetOrientation().up, static_cast<float>(delta_seconds * 360.F / 8.F));
    return true;
}
```

`TexturedCubeApp::Update()` function is called before `App::Render()` call to update shader uniforms with model-view-project (MVP)
matrices and eye position based on current camera orientation, updated in animation.

```cpp
bool TexturedCubeApp::Update()
{
    if (!UserInterfaceApp::Update())
        return false;

    // Update Model, View, Projection matrices based on camera location
    m_shader_uniforms.mvp_matrix   = hlslpp::transpose(hlslpp::mul(m_shader_uniforms.model_matrix, m_camera.GetViewProjMatrix()));
    m_shader_uniforms.eye_position = m_camera.GetOrientation().eye;
    
    return true;
}
```

`TexturedCubeApp::Render()` method is called after all. Initial base method `UserInterfaceApp::Render()` call waits for 
previously current frame buffer presenting is completed. When frame buffer is free, new frame rendering can be started:
1. Uniforms buffer is filled with new shader uniforms data updated in calls above.
2. Render command list encoding starts with `RenderCommandList::Reset(...)` call taking render state object and optional debug group,
which is defining named region in commands sequence.
    1. View state is set with viewports and scissor rects
    2. Program bindings are set
    3. Vertex buffers set is set
    4. Indexed draw call is issued
3. `UserInterface::App::RenderOverlay(...)` is called to record UI drawing command in render command list.
4. Render command list is committed and passed to `Graphics::CommandQueue::Execute` call for execution on GPU.
5. `RenderContext::Present()` is called to schedule frame buffer presenting to screen.

```cpp
bool TexturedCubeApp::Render()
{
    if (!UserInterfaceApp::Render())
        return false;

    // Update uniforms buffer related to current frame
    const TexturedCubeFrame& frame = GetCurrentFrame();
    frame.uniforms_buffer_ptr->SetData(m_shader_uniforms_subresources);

    // Issue commands for cube rendering
    META_DEBUG_GROUP_CREATE_VAR(s_debug_group, "Cube Rendering");
    frame.render_cmd_list_ptr->ResetWithState(*m_render_state_ptr, s_debug_group.get());
    frame.render_cmd_list_ptr->SetViewState(GetViewState());
    frame.render_cmd_list_ptr->SetProgramBindings(*frame.program_bindings_ptr);
    frame.render_cmd_list_ptr->SetVertexBuffers(*m_vertex_buffer_set_ptr);
    frame.render_cmd_list_ptr->SetIndexBuffer(*m_index_buffer_ptr);
    frame.render_cmd_list_ptr->DrawIndexed(gfx::RenderCommandList::Primitive::Triangle);

    RenderOverlay(*frame.render_cmd_list_ptr);

    frame.render_cmd_list_ptr->Commit();

    // Execute command list on render queue and present frame to screen
    GetRenderContext().GetRenderCommandKit().GetQueue().Execute(*frame.execute_cmd_list_set_ptr);
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

HLSL 6 shaders [Shaders/Cube.hlsl](Shaders/Cube.hlsl) implement Phong shading with texturing.
SRGB gamma-correction is implemented with `ColorLinearToSrgb(...)` function from [Common/Shaders/Primitives.hlsl](../../Common/Shaders/Primitives.hlsl)
 which is converting final color from linear-space to SRGB color-space.

```cpp
#include "TexturedCubeUniforms.h"
#include "..\..\..\Common\Shaders\Primitives.hlsl"

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

ConstantBuffer<Constants> g_constants : register(b1);
ConstantBuffer<Uniforms>  g_uniforms  : register(b2);
Texture2D                 g_texture   : register(t0);
SamplerState              g_sampler   : register(s0);

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

Shaders configuration file [Shaders/Cube.cfg](Shaders/Cube.cfg) 
is created in pair with shaders file and describes shader types along with entry points and 
optional sets of macro definitions used to pre-build shaders to bytecode at build time.

```ini
frag=CubePS
vert=CubeVS
```

## CMake Build Configuration

CMake build configuration [CMakeLists.txt](CMakeLists.txt) of the application
is powered by the included Methane CMake modules:
- [MethaneApplications.cmake](../../CMake/MethaneApplications.cmake) - defines function `add_methane_application`
- [MethaneShaders.cmake](../../CMake/MethaneShaders.cmake) - defines function `add_methane_shaders`
- [MethaneResources.cmake](../../CMake/MethaneResources.cmake) - defines functions `add_methane_embedded_textures` and `add_methane_copy_textures`

Shaders are compiled in build time and added as byte code to the application embedded resources.
Texture images are added to the application embedded resources too.

```cmake
include(MethaneApplications)
include(MethaneShaders)
include(MethaneResources)

set(SOURCES
    TexturedCubeApp.h
    TexturedCubeApp.cpp
    Shaders/TexturedCubeUniforms.h
)

set(SHADERS_HLSL ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Cube.hlsl)
set(TEXTURES_DIR ${RESOURCES_DIR}/Textures)
set(TEXTURES ${TEXTURES_DIR}/MethaneBubbles.jpg)

add_methane_application(MethaneTexturedCube
    "${SOURCES}"
    "${RESOURCES_DIR}"
    "Apps"
    "Methane Textured Cube"
    "Tutorial demonstrating textured rotating cube rendering with Methane Kit."s
    "${METHANE_COPYRIGHT}"
    "${METHANE_VERSION_SHORT}"
    "${METHANE_VERSION_BUILD}"
)
add_methane_embedded_textures(MethaneTexturedCube "${TEXTURES_DIR}" "${TEXTURES}")
add_methane_shaders(MethaneTexturedCube "${SHADERS_HLSL}" "6_0")

target_link_libraries(MethaneTexturedCube
    PRIVATE
        MethaneAppsCommon
)
```

## Continue learning

Continue learning Methane Graphics programming in the next tutorial [Shadow Cube](../04-ShadowCube), which is demonstrating
multi-pass rendering for drawing simple shadows.