# Hello Triangle Tutorial

See how triangle rendering application is implemented in 140 lines of code using Methane Kit ([HelloTriangleAppSimple.cpp](/Apps/Tutorials/01-HelloTriangle/HelloTriangleAppSimple.cpp)):
```cpp
#include <Methane/Kit.h>

using namespace Methane;
using namespace Methane::Graphics;

struct HelloTriangleFrame final : AppFrame
{
    Ptr<RenderCommandList> sp_render_cmd_list;
    Ptr<CommandListSet>    sp_execute_cmd_list_set;
    using AppFrame::AppFrame;
};

using GraphicsApp = App<HelloTriangleFrame>;
class HelloTriangleApp final : public GraphicsApp
{
private:
    Ptr<RenderState> m_sp_render_state;
    Ptr<BufferSet>   m_sp_vertex_buffer_set;

public:
    HelloTriangleApp() : GraphicsApp(
        {                                               // Application settings:
            {                                           // platform_app:
                "Methane Hello Triangle",               // - name
                0.8, 0.8,                               // - width, height
            },                                          //
            {                                           // graphics_app:
                RenderPass::Access::None,               // - screen_pass_access
                false,                                  // - animations_enabled
            },                                          //
            {                                           // render_context:
                FrameSize(),                            // - frame_size placeholder: set in InitContext
                PixelFormat::BGRA8Unorm,                // - color_format
                PixelFormat::Unknown,                   // - depth_stencil_format
                Color4f(0.0f, 0.2f, 0.4f, 1.0f),        // - clear_color
            }
        })
    { }

    ~HelloTriangleApp() override
    {
        GetRenderContext().WaitForGpu(Context::WaitFor::RenderComplete);
    }

    void Init() override
    {
        GraphicsApp::Init();

        struct Vertex { Vector3f position; Vector3f color; };
        const std::array<Vertex, 3> triangle_vertices{ {
            { { 0.0f,   0.5f,  0.0f }, { 1.0f, 0.0f, 0.0f } },
            { { 0.5f,  -0.5f,  0.0f }, { 0.0f, 1.0f, 0.0f } },
            { { -0.5f, -0.5f,  0.0f }, { 0.0f, 0.0f, 1.0f } },
        } };

        const Data::Size vertex_buffer_size = static_cast<Data::Size>(sizeof(triangle_vertices));
        Ptr<Buffer> sp_vertex_buffer = Buffer::CreateVertexBuffer(GetRenderContext(), vertex_buffer_size, static_cast<Data::Size>(sizeof(Vertex)));
        sp_vertex_buffer->SetData(
            Resource::SubResources
            {
                Resource::SubResource { reinterpret_cast<Data::ConstRawPtr>(triangle_vertices.data()), vertex_buffer_size }
            }
        );
        m_sp_vertex_buffer_set = BufferSet::CreateVertexBuffers({ *sp_vertex_buffer });

        m_sp_render_state = RenderState::Create(GetRenderContext(),
            RenderState::Settings
            {
                Program::Create(GetRenderContext(),
                    Program::Settings
                    {
                        Program::Shaders
                        {
                            Shader::CreateVertex(GetRenderContext(), { Data::ShaderProvider::Get(), { "Triangle", "TriangleVS" } }),
                            Shader::CreatePixel(GetRenderContext(),  { Data::ShaderProvider::Get(), { "Triangle", "TrianglePS" } }),
                        },
                        Program::InputBufferLayouts
                        {
                            Program::InputBufferLayout
                            {
                                Program::InputBufferLayout::ArgumentSemantics { "POSITION", "COLOR" },
                            }
                        },
                        Program::ArgumentDescriptions { },
                        PixelFormats { GetRenderContext().GetSettings().color_format }
                    }
                )
            }
        );

        for (HelloTriangleFrame& frame : GetFrames())
        {
            frame.sp_render_cmd_list      = RenderCommandList::Create(GetRenderContext().GetRenderCommandQueue(), *frame.sp_screen_pass);
            frame.sp_execute_cmd_list_set = CommandListSet::Create({ *frame.sp_render_cmd_list });
        }

        GraphicsApp::CompleteInitialization();
    }

    bool Render() override
    {
        if (!GraphicsApp::Render())
            return false;

        HelloTriangleFrame& frame = GetCurrentFrame();
        frame.sp_render_cmd_list->Reset(m_sp_render_state);
        frame.sp_render_cmd_list->SetViewState(GetViewState());
        frame.sp_render_cmd_list->SetVertexBuffers(*m_sp_vertex_buffer_set);
        frame.sp_render_cmd_list->Draw(RenderCommandList::Primitive::Triangle, 3);
        frame.sp_render_cmd_list->Commit();

        GetRenderContext().GetRenderCommandQueue().Execute(*frame.sp_execute_cmd_list_set);
        GetRenderContext().Present();

        return true;
    }

    void OnContextReleased(Context& context) override
    {
        m_sp_vertex_buffer_set.reset();
        m_sp_render_state.reset();

        GraphicsApp::OnContextReleased(context);
    }
};

int main(int argc, const char* argv[])
{
    return HelloTriangleApp().Run({ argc, argv });
}
```

This tutorial uses simple HLSL shader [Shaders/Triangle.hlsl](/Apps/Tutorials/01-HelloTriangle/Shaders/Triangle.hlsl).
Note that semantic names of `VSInput` structure members, passed as argument to vertex shader function `TriangleVS(VSInput input)`, 
are matching to input buffer layout arguments `Program::InputBufferLayout::ArgumentSemantics { "POSITION", "COLOR" }`
passed in Settings of `Program::Create(...)` call:
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

PSInput TriangleVS(VSInput input)
{
    PSInput output;
    output.position = float4(input.position, 1.f);
    output.color    = float4(input.color, 1.f);
    return output;
}

float4 TrianglePS(PSInput input) : SV_TARGET
{
    return input.color;
}
```

Shaders configuration file [Shaders/Triangle.cfg](/Apps/Tutorials/01-HelloTriangle/Shaders/Triangle.cfg) 
is created in pair with every shaders file and describes shader types along with entry points and 
optional sets of macro definitions used to prebuild shaders to bytecode at build time:
```ini
frag=TrianglePS
vert=TriangleVS
```

Finally CMake build configuration [CMakeLists.txt](/Apps/Tutorials/01-HelloTriangle/CMakeLists.txt) of the application
is powered by the included Methane CMake modules:
- [MethaneApplications.cmake](CMake/MethaneApplications.cmake) - defines function ``add_methane_application``;
- [MethaneShaders.cmake](CMake/MethaneShaders.cmake) - defines function ``add_methane_shaders``;
- [MethaneResources.cmake](CMake/MethaneResources.cmake) - defines functions ``add_methane_embedded_textures`` and ``add_methane_copy_textures``.
```cmake
include(MethaneApplications)
include(MethaneShaders)

add_methane_application(MethaneHelloTriangle
    "HelloTriangleAppSimple.cpp"
    "${RESOURCES_DIR}"
    "Apps/Tutorials"
    "Methane Hello Triangle"
)

add_methane_shaders(MethaneHelloTriangle
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Triangle.hlsl"
    "6_0"
)
```

Now you have all in one application executable/bundle running on Windows & MacOS, which is rendering colored triangle in window with support of resizing the frame buffer.
