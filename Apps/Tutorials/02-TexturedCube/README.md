# Textured Cube Tutorial

| Windows (DirectX 12) | MacOS (Metal) |
| -------------------- | ------------- |
| ![Textured Cube on Windows](Screenshots/TexturedCubeWinDirectX12.jpg) | ![Textured Cube on MacOS](Screenshots/TexturedCubeMacMetal.jpg) |

This tutorial demonstrates textured cube rendering using Methane Kit.
Cube and light source are animated with rotation in opposite directions.

## Application Class Declaration

Let's start from the application header file [TexturedCubeApp.h](TexturedCubeApp.h) which declares
application class `TexturedCubeApp` derived from the base class `UserInterface::App<TexturedCubeFrame>`
and frame class `TexturedCubeFrame` derived from the base class `Graphics::AppFrame` 
similar to how it was done in [HelloTriangle](../01-HelloTriangle) tutorial.
The difference here is the [UserInterface::App](../../../Modules/UserInterface/App) base class used instead of
[Graphics::App](../../../Modules/Graphics/App) class for visualization of optional UI elements and rendering it in screen overlay. 

```cpp
#pragma once

#include <Methane/Kit.h>

namespace Methane::Tutorials
{

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

Methane Kit is designed to use [deferred rendering approach](https://docs.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-render-multi-thread-render)
 by default to effectively use triple buffering and minimize waiting for frame-buffers to get released in swap-chain.
In order to prepare graphics resource states ahead for next frames rendering, `TexturedCubeFrame` structure keeps 
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

Private section of the `TexturedCubeApp` class contains definition of `Constants` and `Uniforms` structures with data
saved in constants buffer `m_const_buffer_ptr` defined below and uniforms buffer `uniforms_buffer_ptr` defined in `TexturedCubeFrame` structure above.
Both structures layout is matching definition of the equally named structures in [HLSL shader code](#cube-rendering-shaders)
and use alignment and packing macroses to guarantee that. 

```cpp
class TexturedCubeApp final : public UserInterfaceApp
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

    struct SHADER_STRUCT_ALIGN Uniforms
    {
        SHADER_FIELD_ALIGN gfx::Vector4f  eye_position;
        SHADER_FIELD_ALIGN gfx::Vector3f  light_position;
        SHADER_FIELD_ALIGN gfx::Matrix44f mvp_matrix;
        SHADER_FIELD_ALIGN gfx::Matrix44f model_matrix;
    };

    const Constants       m_shader_constants;
    Uniforms              m_shader_uniforms { };
    gfx::Camera           m_camera;
    float                 m_cube_scale;
    Ptr<gfx::RenderState> m_render_state_ptr;
    Ptr<gfx::BufferSet>   m_vertex_buffer_set_ptr;
    Ptr<gfx::Buffer>      m_index_buffer_ptr;
    Ptr<gfx::Buffer>      m_const_buffer_ptr;
    Ptr<gfx::Texture>     m_cube_texture_ptr;
    Ptr<gfx::Sampler>     m_texture_sampler_ptr;

    const gfx::Resource::SubResources m_shader_uniforms_subresources{
        { reinterpret_cast<Data::ConstRawPtr>(&m_shader_uniforms), sizeof(Uniforms) }
    };
};
```

## Cube Rendering Shaders

```cpp
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

struct Constants
{
    float4 light_color;
    float  light_power;
    float  light_ambient_factor;
    float  light_specular_factor;
};

struct Uniforms
{
    float4   eye_position;
    float3   light_position;
    float4x4 mvp_matrix;
    float4x4 model_matrix;
};

ConstantBuffer<Constants> g_constants : register(b1);
ConstantBuffer<Uniforms>  g_uniforms  : register(b2);
Texture2D                 g_texture   : register(t0);
SamplerState              g_sampler   : register(s0);

PSInput CubeVS(VSInput input)
{
    const float4 position = float4(input.position, 1.0f);

    PSInput output;
    output.position       = mul(g_uniforms.mvp_matrix, position);
    output.world_position = mul(g_uniforms.model_matrix, position).xyz;
    output.world_normal   = normalize(mul(g_uniforms.model_matrix, float4(input.normal, 0.0)).xyz);
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