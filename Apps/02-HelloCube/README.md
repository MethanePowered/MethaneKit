# Hello Cube Tutorial

| <pre><b>Windows (DirectX 12)       </pre></b>                   | <pre><b>Linux (Vulkan)             </pre></b>              | <pre><b>MacOS (Metal)              </pre></b>             | <pre><b>iOS (Metal)</pre></b>                           |
|-----------------------------------------------------------------|------------------------------------------------------------|-----------------------------------------------------------|---------------------------------------------------------|
| ![Hello Cube on Windows](Screenshots/HelloCubeWinDirectX12.jpg) | ![Hello Cube on Linux](Screenshots/HelloCubeLinVulkan.jpg) | ![Hello Cube on MacOS](Screenshots/HelloCubeMacMetal.jpg) | ![Hello Cube on iOS](Screenshots/HelloCubeIOSMetal.jpg) |

This tutorial demonstrates colored cube rendering implemented in just 220 lines of code using Methane Kit:
- [HelloCubeApp.cpp](HelloCubeApp.cpp)
- [Shaders/HelloCube.hlsl](Shaders/HelloCube.hlsl)
- [Shaders/HelloCubeUniforms.h](Shaders/HelloCubeUniforms.h)

Tutorial demonstrates the following Methane Kit features and techniques additionally to demonstrated in [Hello Triangle](../01-HelloTriangle):
- **Simple version** (when macros `UNIFORMS_BUFFER_ENABLED` is not defined):
  - Create vertex and index buffers on GPU and filling them with data from CPU.
  - Generate cube mesh vertices and indices data with custom vertex layout.
  - Use time animation for camera rotation.
  - Create camera view and projection matrices.
  - Transform cube vertices with camera matrices on CPU and update vertex buffers on GPU.
- **Uniforms version** (when macros `UNIFORMS_BUFFER_ENABLED` is defined):
  - Use uniform buffer to upload MVP matrix to GPU and transform vertices on GPU in vertex shader.
  - Use program bindings to bind uniform buffer to the graphics pipeline and make it available to shaders.

## Application Controls

Common keyboard controls are enabled by the `Platform` and `Graphics` application controllers:
- [Methane::Platform::AppController](/Modules/Platform/App/README.md#platform-application-controller)
- [Methane::Graphics::AppController, AppContextController](/Modules/Graphics/App/README.md#graphics-application-controllers)

## Simple Cube Vertices Transformation on CPU

### Application and Frame Class Members

Application Frame class `HelloCubeFrame` is derived from the base class `Graphics::AppFrame` and extends it
with render command list `render_cmd_list`, command list set `execute_cmd_list_set` and volatile 
vertex buffer set `vertex_buffer_set` used for cube drawing.

```cpp
struct HelloCubeFrame final : AppFrame
{
    Rhi::BufferSet         vertex_buffer_set;
    Rhi::RenderCommandList render_cmd_list;
    Rhi::CommandListSet    execute_cmd_list_set;

    using AppFrame::AppFrame;
};
```

Application class `HelloCubeApp` is derived from the base template class [Graphics::App<HelloCubeFrame>](/Modules/Graphics/App).
- `m_cube_mesh` member of type `CubeMesh<CubeVertex>` generates and holds cube model vertices defined by `CubeVertex` struct with layout which contains positions and colors.
- `m_model_matrix` matrix is used for cube model scaling.
- `m_camera` is perspective camera model which is used to generate view and projection matrices.
- `m_proj_vertices` vertices vector is used to store transformed vertices in projection coordinates,
  which are then transferred to GPU vertex buffer.

```cpp
using GraphicsApp = App<HelloCubeFrame>;
class HelloCubeApp final : public GraphicsApp
{
private:
    struct CubeVertex
    {
        Mesh::Position position;
        Mesh::Color    color;

        inline static const Mesh::VertexLayout layout{
            Mesh::VertexField::Position,
            Mesh::VertexField::Color
        };
    };

    const CubeMesh<CubeVertex> m_cube_mesh{ CubeVertex::layout };
    const hlslpp::float4x4     m_model_matrix = hlslpp::float4x4::scale(15.F);
    Camera                     m_camera;
    std::vector<CubeVertex>    m_proj_vertices;

    ...
};
```

`HelloCubeApp` constructor calls base constructor `GraphicsApp` with `Graphics::CombinedAppSettings` initialized
using helper function `Tutorials::GetGraphicsTutorialAppSettings` which is called with predefined flags to enable color buffer 
without depth and enable animations. `m_proj_vertices` is initialized with vertices data taken from cube mesh generator.

Initial camera orientation is set with `Camera::ResetOrientation` via `Camera::Orientation` struct with
`eye`, `aim` and `up` vectors. Then we setup camera rotation animation which automatically updates the orientation
before every iteration of update/render. Animation is setup simply by adding `Data::TimeAnimation` object with
lamda function taking `delta_seconds` argument used to calculate the rotation delta angle around camera's `up` axis.

```cpp
HelloCubeApp()
    : GraphicsApp(
        []() {
            Graphics::CombinedAppSettings settings = Tutorials::GetGraphicsTutorialAppSettings(g_app_name, Tutorials::AppOptions::GetDefaultWithColorOnlyAndAnim());
            settings.graphics_app.SetScreenPassAccess({});
            return settings;
        }(),
        "Tutorial demonstrating colored rotating cube rendering with Methane Kit.")
{
    m_camera.ResetOrientation({ { 13.0F, 13.0F, 13.0F }, { 0.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } });

    // Setup camera rotation animation
    GetAnimations().emplace_back(std::make_shared<Data::TimeAnimation>(
        [this](double, double delta_seconds)
        {
            m_camera.Rotate(m_camera.GetOrientation().up, static_cast<float>(delta_seconds * 360.F / 8.F));
            return true;
        }));
}

```

### Graphics Resources Initialization

Application class `HelloCubeApp` keeps frame independent resources in class members: render state `m_render_state`, 
per-frame vertex buffers and index buffer used for triangle rendering `m_index_buffer` are initialized in `HelloCubeApp::Init` method.
Also camera view is resized according to frame size to maintain the correct aspect ratio of the projection.

Render state is created with `GetRenderContext().CreateRenderState(...)` function similarly as in [HelloTriangle](../01-HelloTriangle) tutorial.
Program is created as a part of the state with `GetRenderContext().CreateProgram(...)` function taking `Rhi::Program::Settings`, which differs
from program settings in [HelloTriangle](../01-HelloTriangle) in configuration of `Rhi::ProgramInputBufferLayouts`. In this tutorial
we use single input vertex buffer with interleaved positions and colors, which is described by `Rhi::Program::InputBufferLayout` with
an array of HLSL vertex shader input semantics, corresponding to `CubeVertex` struct layout.

```cpp
class HelloCubeApp final : public GraphicsApp
{
private:
    ...

    Rhi::CommandQueue m_render_cmd_queue;
    Rhi::RenderState  m_render_state;
    Rhi::Buffer       m_index_buffer;
    
public:
    ...

    void Init() override
    {
        GraphicsApp::Init();

        m_camera.Resize(GetRenderContext().GetSettings().frame_size);

        // Create render state with program
        const Rhi::Shader::MacroDefinitions vertex_shader_definitions;
        m_render_state = GetRenderContext().CreateRenderState(
            Rhi::RenderState::Settings
            {
                GetRenderContext().CreateProgram(
                    Rhi::Program::Settings
                    {
                        Rhi::Program::ShaderSet
                        {
                            { Rhi::ShaderType::Vertex, { Data::ShaderProvider::Get(), { "HelloCube", "CubeVS" }, vertex_shader_definitions } },
                            { Rhi::ShaderType::Pixel,  { Data::ShaderProvider::Get(), { "HelloCube", "CubePS" } } },
                        },
                        Rhi::ProgramInputBufferLayouts
                        {
                            Rhi::ProgramInputBufferLayout
                            {
                                Rhi::Program::InputBufferLayout::ArgumentSemantics { "POSITION" , "COLOR" }
                            }
                        },
                        Rhi::ProgramArgumentAccessors{ },
                        GetScreenRenderPattern().GetAttachmentFormats()
                    }
                ),
                GetScreenRenderPattern()
            }
        );

        ...
    }

    ...
};
```

Constant index buffer is created with `GetRenderContext().CreateBuffer()` and settings initialized using 
`Rhi::BufferSettings::ForIndexBuffer(...)` function which takes index data size in bytes and index format. 
The data of index buffer in set with `IBuffer::SetData` call which takes an array of sub-resources.
In case of index buffer we need to provide only one default sub-resource with data pointer and data size.

Volatile vertex buffers are created for each frame with `GetRenderContext().CreateBuffer(...)` and settings initialized using 
`Rhi::BufferSettings::ForVertexBuffer(...)` so that they can be updated independently:
while one vertex buffer data is updated for current frame, other frames are rendered using their own dedicated vertex buffers.
Buffers are created in volatile mode, which enables more effective synchronous data updates (aka map-updates).
Each vertex buffer is encapsulated in the buffer set with `Rhi::BufferSet(Rhi::BufferType::Vertex, { vertex_buffer })`,
used for render command list encoding in `SetVertexBuffers(...)`.

Render command lists are created for each frame using `m_render_cmd_queue.CreateRenderCommandList(...)` function,
same as in [HelloTriangle](../01-HelloTriangle) tutorial.

```cpp
class HelloTriangleApp final : public GraphicsApp
{
    ...

    void Init() override
    {
        ...
        
        m_render_cmd_queue = GetRenderContext().GetRenderCommandKit().GetQueue();

        // Create index buffer for cube mesh
        m_index_buffer = GetRenderContext().CreateBuffer(Rhi::BufferSettings::ForIndexBuffer(m_cube_mesh.GetIndexDataSize(), GetIndexFormat(m_cube_mesh.GetIndex(0))));
        m_index_buffer.SetData(
            { { reinterpret_cast<Data::ConstRawPtr>(m_cube_mesh.GetIndices().data()), m_cube_mesh.GetIndexDataSize() } },
            m_render_cmd_queue
        );

        // Create per-frame command lists
        for(HelloCubeFrame& frame : GetFrames())
        {
            // Create vertex buffers for each frame
            Rhi::Buffer vertex_buffer = GetRenderContext().CreateBuffer(Rhi::BufferSettings::ForVertexBuffer(m_cube_mesh.GetVertexDataSize(), m_cube_mesh.GetVertexSize(), true));
            frame.vertex_buffer_set = Rhi::BufferSet(Rhi::BufferType::Vertex, { vertex_buffer });

            // Create command list for rendering
            frame.render_cmd_list = m_render_cmd_queue.CreateRenderCommandList(frame.screen_pass);
            frame.execute_cmd_list_set = Rhi::CommandListSet({ frame.render_cmd_list.GetInterface() }, frame.index);
        }

        GraphicsApp::CompleteInitialization();
    }
}
```

### Frame Rendering Cycle

Each rendering cycle is started with `GraphicsApp::Update()` method used for updating the data for frame rendering.
This method is called just before `GraphicsApp::Render()` so that frame data is prepared while previous frames are rendered on GPU,
just before waiting for current frame buffer is released from previous rendering cycle. For the purpose of simplicity,
in this tutorial we transform vertex positions from model to projection coordinates on CPU, while usually it is done on GPU.
This allows to avoid using uniform buffers and program bindings for educational purposes. Vertex positions are updated
using model-view-projection (MVP) matrix calculated as multiplication of model scaling matrix and camera view-projection matrix.
Projected positions in `m_proj_vertices` are calculated as multiplication of original positions to the MVP matrix and 
normalization by W-coordinate.

```cpp
class HelloCubeApp final : public GraphicsApp
{
    ...

    bool Update() override
    {
        if (!GraphicsApp::Update())
            return false;

        // Update vertex buffer with camera Model-View-Projection matrix applied on CPU
        const hlslpp::float4x4 mvp_matrix = hlslpp::mul(m_model_matrix, m_camera.GetViewProjMatrix());
        for(size_t vertex_index = 0; vertex_index < m_proj_vertices.size(); ++vertex_index)
        {
            const hlslpp::float4 orig_position_vec(m_cube_mesh.GetVertices()[vertex_index].position.AsHlsl(), 1.F);
            const hlslpp::float4 proj_position_vec = hlslpp::mul(orig_position_vec, mvp_matrix);
            m_proj_vertices[vertex_index].position = Mesh::Position(proj_position_vec.xyz / proj_position_vec.w);
        }

        return true;
    }

    ...
};
```

Rendering is performed in overridden `HelloCubeApp::Render` method. Scene rendering is started strictly after
the base graphics application rendering logic is completed in `GraphicsApp::Render()`, because it waits for
previous iteration of rendering cycle completion and availability of all frame resources. Then current frame resources 
are requested with `GraphicsApp::GetCurrentFrame()` and used for render commands encoding.

We update volatile vertex buffers with projected vertex data, which was calculated in the previous
method call `HelloCubeApp::Update()`. Cube rendering is done similar to the triangle rendering in previous
tutorial with the only difference of setting vertex and index buffers with `Rhi::RenderCommandList::SetVertexBuffers(...)`
and `Rhi::RenderCommandList::SetIndexBuffers(...)` encoded before `Rhi::RenderCommandList::DrawIndexed(...)` call. 
Note that number of vertices is not passed explicitly for `DrawIndexed`, but is taken from the number of indies in index buffer.

Execution of GPU rendering is started with `Rhi::CommandQueue::Execute(...)` method called on the same command queue
which was used to create the command list submitted for execution. Frame buffer with the result image is presented by
swap-chain with `Rhi::RenderContext::Present()` method call.

```cpp
class HelloCubeApp final : public GraphicsApp
{
    ...

    bool Render() override
    {
        if (!GraphicsApp::Render())
            return false;

        const HelloCubeFrame& frame = GetCurrentFrame();
        
        // Update vertex buffer with vertices in camera's projection view
        frame.vertex_buffer_set[0].SetData(
            { { reinterpret_cast<Data::ConstRawPtr>(m_proj_vertices.data()), m_cube_mesh.GetVertexDataSize() } },
            m_render_cmd_queue
        );

        // Issue commands for cube rendering
        META_DEBUG_GROUP_VAR(s_debug_group, "Cube Rendering");
        frame.render_cmd_list.ResetWithState(m_render_state, &s_debug_group);
        frame.render_cmd_list.SetViewState(GetViewState());
        frame.render_cmd_list.SetVertexBuffers(frame.vertex_buffer_set);
        frame.render_cmd_list.SetIndexBuffer(m_index_buffer);
        frame.render_cmd_list.DrawIndexed(Rhi::RenderPrimitive::Triangle);
        frame.render_cmd_list.Commit();

        // Execute command list on render queue and present frame to screen
        m_render_cmd_queue.Execute(frame.execute_cmd_list_set);
        GetRenderContext().Present();

        return true;
    }
};
```

Graphics render loop is started from `main(...)` entry function using `GraphicsApp::Run(...)` method which is also parsing command line arguments.

```cpp
int main(int argc, const char* argv[])
{
    return HelloTriangleApp().Run({ argc, argv });
}
```

### Frame Resizing

Additionally, camera view needs to be resized along with the window in the override method `HelloCubeApp::Resize(...)` 
so that projected image aspect ratio will be the same as for window. 

```cpp
class HelloCubeApp final : public GraphicsApp
{
    ...

    bool Resize(const FrameSize& frame_size, bool is_minimized) override
    {
        // Resize screen color and depth textures
        if (!GraphicsApp::Resize(frame_size, is_minimized))
            return false;

        m_camera.Resize(frame_size);
        return true;
    }

    ...
}
```

### Simple Cube Rendering Shaders

This tutorial uses simple HLSL shader [Shaders/HelloCube.hlsl](Shaders/HelloCube.hlsl).
Note that semantic names of `VSInput` structure members, passed as argument to vertex shader function `CubeVS(VSInput input)`,
are matching to input buffer layout arguments `Rhi::ProgramInputBufferLayout::ArgumentSemantics{ "POSITION", "COLOR" }`
passed in Settings of `CreateProgram(...)` call and also correspond to `CubeVertex` struct layout.

```cpp
struct VSInput
{
    float3 position : POSITION;
    float3 color    : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
};

PSInput CubeVS(VSInput input)
{
    PSInput output;
    output.position = float4(input.position, 1.F);
    output.color    = float4(input.color, 1.F);
    return output;
}

float4 CubePS(PSInput input) : SV_TARGET
{
    return input.color;
}
```

## Using Uniforms Buffer to Transform Cube Vertices on GPU

### Add Uniform Buffer in Vertex Shader

Now we modify cube rendering using uniform buffer containing single `struct Uniforms` with model-view-projection (MVP)
`float4x4` matrix, which is defined in header file [Shaders/HelloCubeUniforms.h](Shaders/HelloCubeUniforms.h).

```cpp
struct Uniforms
{
    float4x4 mvp_matrix;
};
```

Shaders source file [Shaders/HelloCube.hlsl](Shaders/HelloCube.hlsl) includes uniforms header and
defines global variable `g_uniforms` of type `ConstantBuffer<Uniforms>` which represents our uniform buffer.
MVP matrix from this buffer is used in vertex shader `CubeVS`  to transform vertices from model to projection 
coordinates on GPU.

```cpp
#ifdef UNIFORMS_BUFFER_ENABLED
#include "HelloCubeUniforms.h"
#endif

...

#ifdef UNIFORMS_BUFFER_ENABLED
ConstantBuffer<Uniforms> g_uniforms : register(b1);
#endif

PSInput CubeVS(VSInput input)
{
    PSInput output;
#ifdef UNIFORMS_BUFFER_ENABLED
    output.position = mul(float4(input.position, 1.F), g_uniforms.mvp_matrix);
#else
    output.position = float4(input.position, 1.F);
#endif
    output.color    = float4(input.color, 1.F);
    return output;
}
```

In order to compile vertex shader with `UNIFORMS_BUFFER_ENABLED` macro definition, we add it to shader types
description in `CMakeLists.txt` inside `add_methane_shaders_source` function.

```cmake
add_methane_shaders_source(
    TARGET MethaneHelloCubeUniforms
    SOURCE Shaders/HelloCube.hlsl
    VERSION 6_0
    TYPES
        vert=CubeVS:UNIFORMS_BUFFER_ENABLED
        frag=CubePS
)
```

### Add Uniforms Buffer in Application Code

In C++ source file [HelloCubeApp.cpp](HelloCubeApp.cpp) uniform header is included inside namespace `hlslpp` to reuse
matrix type definitions from `hlsl++.h` and is wrapped in `#pragma pack(push, 16)` to use the same structure fields
memory alignment as in HLSL shaders.

```cpp
namespace hlslpp
{
#pragma pack(push, 16)
#include "Shaders/HelloCubeUniforms.h"
#pragma pack(pop)
}
```

Uniform `Rhi::Buffer` is declared inside the frame structure along with `Rhi::ProgramBindings` required to bind buffer to graphics pipeline.

```cpp
struct HelloCubeFrame final : AppFrame
{
    Rhi::Buffer>          uniforms_buffer;
    Rhi::ProgramBindings> program_bindings;
    
    ...
}
```

`Uniforms` field `m_shader_uniforms` is allocated on stack of application class `HelloCubeApp` along with a helper structure
`Rhi::SubResources` with a pointer and size of `m_shader_uniforms` field. It will be used later to set data of the
uniform buffer on GPU.

```cpp
class HelloCubeApp final : public GraphicsApp
{
private:
    ...
    
    hlslpp::Uniforms              m_shader_uniforms { };
    const IResource::SubResources m_shader_uniforms_subresources{
        { reinterpret_cast<Data::ConstRawPtr>(&m_shader_uniforms), sizeof(hlslpp::Uniforms) }
    };
    Rhi::BufferSet m_vertex_buffer_set;
}
```

The compiled vertex shader is loaded from resources with respect to macro definition `UNIFORMS_BUFFER_ENABLED`,
which was used to compile it at build-time using CMake function `add_methane_shaders_source`.
Also `ProgramArgumentAccessors` now describe new program argument `g_uniforms`: it contains shader type where it is used
and type of access to the argument `ProgramArgumentAccessor::Type::FrameConstant`, which means that only one buffer
is bound to this program argument at each frame (argument is constant per frame). Other argument accessor types include 
`Constant` (bound to single resource all the time) and `Mutable` (can be bound to different resources).

```cpp
class HelloCubeApp final : public GraphicsApp
{
    ...
    
    void Init() override
    {
        ...

        const Rhi::Shader::MacroDefinitions vertex_shader_definitions{ { "UNIFORMS_BUFFER_ENABLED", "" } };
        m_render_state = GetRenderContext().CreateRenderState(
            Rhi::RenderState::Settings
            {
                GetRenderContext().CreateProgram(
                    Rhi::Program::Settings
                    {
                        Rhi::Program::ShaderSet
                        {
                            { Rhi::ShaderType::Vertex, { Data::ShaderProvider::Get(), { "HelloCube", "CubeVS" }, vertex_shader_definitions } },
                            { Rhi::ShaderType::Pixel,  { Data::ShaderProvider::Get(), { "HelloCube", "CubePS" } } },
                        },
                        ...
                        Rhi::ProgramArgumentAccessors
                        {
                            { { Rhi::ShaderType::Vertex, "g_uniforms" }, Rhi::ProgramArgumentAccessType::FrameConstant }
                        },
                        ...
                    }
                ),
                GetScreenRenderPattern()
            }
        );
        
        ...
    }
}
```

We create single vertex buffer `vertex_buffer`, as it's content will not change during frames rendering,
fill it with cube vertices data and place it in vertex buffer set `m_vertex_buffer_set`.
Separate uniform buffers are created for each frame `frame.uniforms_buffer` because their content will be changed 
on every frame, and we want to make these updates fully independent of other frames rendering in flight.
And finally, we create `Rhi::ProgramBindings` object to bind `frame.uniforms_buffer` to vertex buffer argument `g_uniforms`.

```cpp
class HelloCubeApp final : public GraphicsApp
{
    ...
    
    void Init() override
    {
        ...

        // Create constant vertex buffer
        Rhi::Buffer vertex_buffer = GetRenderContext().CreateBuffer(Rhi::BufferSettings::ForVertexBuffer(m_cube_mesh.GetVertexDataSize(), m_cube_mesh.GetVertexSize()));
        vertex_buffer.SetData(
            { { reinterpret_cast<Data::ConstRawPtr>(m_cube_mesh.GetVertices().data()), m_cube_mesh.GetVertexDataSize() } },
            m_render_cmd_queue
        );
        m_vertex_buffer_set = Rhi::BufferSet(Rhi::BufferType::Vertex, { vertex_buffer });

        const auto uniforms_data_size = static_cast<Data::Size>(sizeof(m_shader_uniforms));

        // Create per-frame command lists
        for(HelloCubeFrame& frame : GetFrames())
        {
            // Create uniforms buffer with volatile parameters for frame rendering
            frame.uniforms_buffer = GetRenderContext().CreateBuffer(Rhi::BufferSettings::ForConstantBuffer(uniforms_data_size, false, true));

            // Configure program resource bindings
            frame.program_bindings = m_render_state.GetProgram().CreateBindings({
                { { Rhi::ShaderType::Vertex, "g_uniforms"  }, { { frame.uniforms_buffer.GetInterface() } } }
            }, frame.index);
            
            ...
        }
    }
    
    ...
}
```

### Use Uniforms Buffer for Cube Rendering

In the `HelloCubeApp::Update()` method we update the MVP matrix in the `m_shader_uniforms`
structure, so that it can be uploaded to the uniform buffer on GPU with `frame.uniforms_buffer.SetData(...)`
in `Render()` method. Note that matrix is transposed in order to match the HLSL default row-vector, column-major matrix layout.
Uniform buffer is bound to graphics pipeline with `frame.render_cmd_list.SetProgramBindings(...)` call
using previously created program bindings object `frame.program_bindings`.

```cpp
class HelloCubeApp final : public GraphicsApp
{
    ...
    
    bool Update() override
    {
        if (!GraphicsApp::Update())
            return false;
        
        const hlslpp::float4x4 mvp_matrix = hlslpp::mul(m_model_matrix, m_camera.GetViewProjMatrix());
        m_shader_uniforms.mvp_matrix = hlslpp::transpose(mvp_matrix);
    }
    
    bool Render() override
    {
        if (!GraphicsApp::Render())
            return false;

        const HelloCubeFrame& frame = GetCurrentFrame();

        // Update uniforms buffer on GPU and apply model-view-projection transformation in vertex shader on GPU
        frame.uniforms_buffer.SetData(m_shader_uniforms_subresources, m_render_cmd_queue);

        // Issue commands for cube rendering
        META_DEBUG_GROUP_VAR(s_debug_group, "Cube Rendering");
        frame.render_cmd_list.ResetWithState(m_render_state, &s_debug_group);
        frame.render_cmd_list.SetViewState(GetViewState());
        frame.render_cmd_list.SetProgramBindings(frame.program_bindings);
        frame.render_cmd_list.SetVertexBuffers(m_vertex_buffer_set);
        frame.render_cmd_list.SetIndexBuffer(m_index_buffer);
        frame.render_cmd_list.DrawIndexed(Rhi::RenderPrimitive::Triangle);
        frame.render_cmd_list.Commit();

        // Execute command list on render queue and present frame to screen
        m_render_cmd_queue.Execute(frame.execute_cmd_list_set);
        GetRenderContext().Present();

        return true;
    }
}
```

Instead of transforming cube vertices on the CPU and uploading all of them to GPU, now we calculate and upload 
single MVP matrix to GPU using uniform buffer, bind it to render pipeline using program bindings and use it to 
transform cube vertices to projection coordinates on the GPU in vertex shader.

## CMake Build Configuration

CMake build configuration [CMakeLists.txt](CMakeLists.txt) of the application.

```cmake
add_methane_application(
    TARGET MethaneHelloCube
    NAME "Methane Hello Cube"
    DESCRIPTION "Tutorial demonstrating colored rotating cube rendering with Methane Kit."
    INSTALL_DIR "Apps"
    SOURCES
        HelloCubeApp.cpp
)

add_methane_shaders_source(
    TARGET MethaneHelloCube
    SOURCE Shaders/HelloCube.hlsl
    VERSION 6_0
    TYPES
        vert=CubeVS
        frag=CubePS
)

add_methane_shaders_library(MethaneHelloCube)

target_link_libraries(MethaneHelloCube
    PRIVATE
        MethaneAppsCommon
)
```

## Continue learning

Continue learning Methane Graphics programming in the next tutorial [Textured Cube](../03-TexturedCube), 
which is demonstrating textured cube rendering with phong lighting in pixel shader.
