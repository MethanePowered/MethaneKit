# Methane Kit <img src="https://github.com/egorodet/MethaneKit/blob/master/Resources/Images/Logo/MethaneLogoNameSmall.png" width=200 align="right" valign="middle">

**Easy to use modern 3D graphics abstraction API written in C++ for cross-platform applications development:**
- **Built on top of modern native 3D graphics APIs**: DirectX 12 on Windows and Metal on MacOS.
- **Provides object-oriented graphics API** simple but capable of high-performance graphics rendering on modern GPUs.
- **Supplies completely cross-platform application infrastructure**: clean C++ code without nasty preprocessor stuff.

Note that project is in **Alpha / [MVP](https://en.wikipedia.org/wiki/Minimum_viable_product)** stage and is under active development.

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
![Languages](https://img.shields.io/badge/Languages-C++%20%7C%20ObjC-orange.svg)
[![LoC](https://tokei.rs/b1/github/egorodet/MethaneKit)](https://github.com/egorodet/MethaneKit)
[![Codacy](https://api.codacy.com/project/badge/Grade/25ae34bade994076bf636290791b3e0f)](https://www.codacy.com/app/egorodet/MethaneKit?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=egorodet/MethaneKit&amp;utm_campaign=Badge_Grade)
[![CodeFactor](https://www.codefactor.io/repository/github/egorodet/methanekit/badge)](https://www.codefactor.io/repository/github/egorodet/methanekit)
[![Contributions Welcome](https://img.shields.io/badge/contributions-welcome-brightgreen.svg?style=flat)](https://github.com/egorodet/MethaneKit/issues)
[![Join the chat at https://gitter.im/MethaneKit/community](https://badges.gitter.im/MethaneKit/community.svg)](https://gitter.im/MethaneKit/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![HitCount](http://hits.dwyl.io/egorodet/MethaneKit.svg)](http://hits.dwyl.io/egorodet/MethaneKit)

|     Platform     |  Master Build Status  |
| ---------------- | --------------------- |
| <img src="https://github.com/egorodet/MethaneKit/blob/master/Resources/Images/Platforms/Windows.png" width=24 valign="middle"> **Windows x64** | [![Windows Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=master&jobName=Windows_x64)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=master) |
| <img src="https://github.com/egorodet/MethaneKit/blob/master/Resources/Images/Platforms/Windows.png" width=24 valign="middle"> **Windows x86** | [![Windows Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=master&jobName=Windows_x86)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=master) |
| <img src="https://github.com/egorodet/MethaneKit/blob/master/Resources/Images/Platforms/MacOS.png" width=24 valign="middle"> **MacOS** | [![MacOS Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=master&jobName=macOS)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=master) |

## Features

- **Cross-platform**
  - Supported platforms:
    - Windows 10 with DirectX 12
    - MacOS (El Capitan or later) with Metal API
  - Application infrastructure:
    - [Methane CMake module](/CMake/Methane.cmake) is provided to simplify configuration of the cross-platform build
    - [Base application class](/Modules/Platform/App/Include/Methane/Platform/AppBase.h) and platform-specific implementations are completely GLFW free
    - [Graphics application base template class](/Modules/Graphics/App/Include/Methane/Graphics/App.hpp) with basic multi-frame swap-chain management 
- **One shader code for all APIs**
  - Shaders are written in HLSL 5.1
  - Shaders are converted to native API shading language at build time with SPIRV-Cross toolchain
  - Shaders are prebuilt and embedded in application resources as bytecode (with preprocessor definitions support)
- **Easy to use object-oriented graphics API**
  - [Core graphics interfaces](/Modules/Graphics/Core/Include/Methane/Graphics):
    - [Context](/Modules/Graphics/Core/Include/Methane/Graphics/Context.h) unifies Device and Swap-Chain under one umbrella
    - [Resource](/Modules/Graphics/Core/Include/Methane/Graphics/Resource.h) derived interfaces [Buffer](/Modules/Graphics/Core/Include/Methane/Graphics/Buffer.h), [Texture](/Modules/Graphics/Core/Include/Methane/Graphics/Texture.h), [Sampler](/Modules/Graphics/Core/Include/Methane/Graphics/Sampler.h) to work with GPU memory resources
    - [Shader](/Modules/Graphics/Core/Include/Methane/Graphics/Shader.h) and [Program](/Modules/Graphics/Core/Include/Methane/Graphics/Program.h) providing unified access to compiled shaders with input layout and uniform variables reflection
    - [Program::ResourceBindings](/Modules/Graphics/Core/Include/Methane/Graphics/Program.h) simplifies binding resources to programs by uniform variable names and enables fast bindings switching at runtime
    - [RenderState](/Modules/Graphics/Core/Include/Methane/Graphics/RenderState.h) and [RenderPass](/Modules/Graphics/Core/Include/Methane/Graphics/RenderPass.h) used for inputs and outputs configuration of the graphics pipeline
    - [RenderCommandList](/Modules/Graphics/Core/Include/Methane/Graphics/RenderCommandList.h) and [CommandQueue](/Modules/Graphics/Core/Include/Methane/Graphics/CommandQueue.h) for render commands encoding and execution
  - [Core interface extensions](/Modules/Graphics/Extensions/Include/Methane/Graphics) like: ImageLoader for creating textures from common image formats.
  - [Common 3D graphics helpers](/Modules/Graphics/Helpers/Include/Methane/Graphics) like: Camera, Timer, Mesh generator, etc.
- **Lightweight**:
  - No heavy external depedencies (almost all libraries are header only)
  - Fast application startup (thanks to embedded prebuilt shaders)
- **Performance oriented**:
  - Triple frame buffering swap-chain by default
  - Builtin API instrumentation with [Intel ITT API](https://software.intel.com/en-us/vtune-amplifier-help-instrumentation-and-tracing-technology-apis) for performance analysis with [Intel Vtune Amplifier](https://software.intel.com/en-us/vtune) and [Intel Graphics Trace Analyzer](https://software.intel.com/en-us/gpa/graphics-trace-analyzer)

## Development plans

- [x] Continous integration setup
- [ ] Application user input with mouse and keyboard
- [ ] Text rendering
- [ ] User interface library
- [ ] Improved shader conversion
- [ ] Dynamic linking support
- [ ] Compute pipeline
- [ ] Parallel command lists

## Getting Started

### Prerequisites

- **Common**
  - Git
  - CMake 3.12 or later installed and availabled from any location
- **Windows**
  - Windows 10 OS or later
  - Visual Studio 2017 or later
  - MSVC v141 or later
  - Windows 10 SDK
- **MacOS**
  - MacOS El Capitan OS or later
  - XCode with Command Line tools

### Build

#### Fetch Sources

**IMPORTANT:** Do not download source code via Zip archive, since it will not allow to properly initalize External submodules. Use git as described below.

- **First time initialization**
```console
mkdir <MethaneKit-Root>
cd <MethaneKit-Root>
git clone --recurse-submodules --depth 1 https://github.com/egorodet/MethaneKit.git
```
- **Update sources to latest version**
```console
cd <MethaneKit-Root>
git pull --recurse-submodules
```
- **Update linked submodules to latest version** (for development purposes only)
```console
cd <MethaneKit-Root>
git submodule update --init --depth 1 --recursive
git pull --recurse-submodules
```

#### Windows Build

Start **"x64 Native Tools Command Prompt for VS2017"** (it initializes environment with VS path to Windows SDK, etc), then go to MethaneKit root directory (see instructions above to initialize repository and get latest code with submodules) and either start auxilarry build script [Build/Windows/Build.bat](Build/Windows/Build.bat) or build with cmake manually:
```console
mkdir Build\Output\VisualStudio\Build
cd Build\Output\VisualStudio\Build
cmake -G "Visual Studio 15 2017 Win64" -DCMAKE_INSTALL_PREFIX="%cd%\..\Install" "..\..\..\.."
cmake --build . --config Release --target install
```

Alternatively you can open root [CMakeLists.txt](CMakeLists.txt) directly in Visual Studio 2017 and build it with VS CMake Tools using "Ninja" generator and provided configuration file [CMakeSettings.json](CMakeSettings.json).

Run tutorials from the installation directory `Build\Output\VisualStudio\Install\Apps\Tutorials`.

#### MacOS Build

Start terminal, then go to MethaneKit root directory (see instructions above to initialize repository and get latest code with submodules) and either start auxilarry build script [Build/MacOS/Build.sh](Build/MacOS/Build.sh) or build with cmake manually:
```console
mkdir -p Build/Output/XCode/Build
cd Build/Output/XCode/Build
cmake -H../../../.. -B. -G Xcode -DCMAKE_INSTALL_PREFIX="$(pwd)/../Install"
cmake --build . --config Release --target install
```

Alternatively you can open root [CMakeLists.txt](CMakeLists.txt) directly in QtCreator or VSCode or any other IDE of choice and build it from there.

Run tutorials from the installation directory `Build/Output/XCode/Install/Apps/Tutorials`

### Tutorials

| Tutorial / Platform | Windows (DirectX 12) | MacOS (Metal) |
| ------------------- | -------------------- | ------------- |
| [Hello Triangle](/Apps/Tutorials/01-HelloTriangle) | ![Hello Triangle on Windows](/Apps/Tutorials/01-HelloTriangle/Screenshots/HelloTriangleWinDirectX12.jpg) | ![Hello Triangle on MacOS](/Apps/Tutorials/01-HelloTriangle/Screenshots/HelloTriangleMacMetal.jpg) |
| [Textured Cube](/Apps/Tutorials/02-TexturedCube) | ![Textured Cube on Windows](/Apps/Tutorials/02-TexturedCube/Screenshots/TexturedCubeWinDirectX12.jpg) | ![Textured Cube on MacOS](/Apps/Tutorials/02-TexturedCube/Screenshots/TexturedCubeMacMetal.jpg) |
| [Shadow Cube](/Apps/Tutorials/03-ShadowCube) | ![Shadow Cube on Windows](/Apps/Tutorials/03-ShadowCube/Screenshots/ShadowCubeWinDirectX12.jpg) | ![Shadow Cube on MacOS](/Apps/Tutorials/03-ShadowCube/Screenshots/ShadowCubeMacMetal.jpg) |

### Hello Triangle

See how triangle rendering application is implemented in 120 lines of C++ code using Methane Kit ([HelloTriangleAppSimple.cpp](/Apps/Tutorials/01-HelloTriangle/HelloTriangleAppSimple.cpp)):
```cpp
#include <Methane/Kit.h>

using namespace Methane;
using namespace Methane::Graphics;

struct HelloTriangleFrame final : AppFrame
{
    RenderCommandList::Ptr sp_cmd_list;
    using AppFrame::AppFrame;
};

using GraphicsApp = App<HelloTriangleFrame>;
class HelloTriangleApp final : public GraphicsApp
{
private:
    RenderState::Ptr m_sp_state;
    Buffer::Ptr      m_sp_vertex_buffer;

public:
    HelloTriangleApp() : GraphicsApp(
        {                                      // Application settings:
            {                                  // app:
                "Methane Hello Triangle",      // - name
                0.8, 0.8,                      // - width, height 
            },                                 //
            {                                  // context:
                FrameSize(),                   // - frame_size placeholder: actual size is set in InitContext
                PixelFormat::BGRA8Unorm,       // - color_format
                PixelFormat::Unknown,          // - depth_stencil_format
                Color(0.0f, 0.2f, 0.4f, 1.0f), // - clear_color
            },                                 //
            true                               // show_hud_in_window_title
        },
        RenderPass::Access::None)              // screen_pass_access (program access to resources)
    { }

    ~HelloTriangleApp() override
    {
        m_sp_context->WaitForGpu(Context::WaitFor::RenderComplete);
    }

    void Init() override
    {
        GraphicsApp::Init();

        struct Vertex { Vector3f position; Vector3f color; };
        const std::array<Vertex, 3> triange_vertices = { {
            { { 0.0f,   0.5f,  0.0f }, { 1.0f, 0.0f, 0.0f } },
            { { 0.5f,  -0.5f,  0.0f }, { 0.0f, 1.0f, 0.0f } },
            { { -0.5f, -0.5f,  0.0f }, { 0.0f, 0.0f, 1.0f } },
        } };

        const Data::Size vertex_buffer_size = static_cast<Data::Size>(sizeof(triange_vertices));
        m_sp_vertex_buffer = Buffer::CreateVertexBuffer(*m_sp_context, vertex_buffer_size, static_cast<Data::Size>(sizeof(Vertex)));
        m_sp_vertex_buffer->SetData(reinterpret_cast<Data::ConstRawPtr>(triange_vertices.data()), vertex_buffer_size);

        m_sp_state = RenderState::Create(*m_sp_context,
        {
            Program::Create(*m_sp_context, {
                {
                    Shader::CreateVertex(*m_sp_context, { { "VSMain", "vs_5_1" } }),
                    Shader::CreatePixel(*m_sp_context,  { { "PSMain", "ps_5_1" } }),
                },
                { { {
                    { "in_position", "POSITION" },
                    { "in_color",    "COLOR"    },
                } } },
                { },
                { m_sp_context->GetSettings().color_format }
            }),
            { GetFrameViewport(m_sp_context->GetSettings().frame_size) },
            { GetFrameScissorRect(m_sp_context->GetSettings().frame_size) },
        });

        for (HelloTriangleFrame& frame : m_frames)
        {
            frame.sp_cmd_list = RenderCommandList::Create(m_sp_context->GetRenderCommandQueue(), *frame.sp_screen_pass);
        }

        m_sp_context->CompleteInitialization();
    }

    bool Resize(const FrameSize& frame_size, bool is_minimized) override
    {
        if (!GraphicsApp::Resize(frame_size, is_minimized))
            return false;

        m_sp_state->SetViewports(    { GetFrameViewport(frame_size)    } );
        m_sp_state->SetScissorRects( { GetFrameScissorRect(frame_size) } );
        return true;
    }

    void Update() override { }

    void Render() override
    {
        if (HasError() || !m_sp_context->ReadyToRender())
            return;

        m_sp_context->WaitForGpu(Context::WaitFor::FramePresented);

        HelloTriangleFrame & frame = GetCurrentFrame();
        frame.sp_cmd_list->Reset(*m_sp_state);
        frame.sp_cmd_list->SetVertexBuffers({ *m_sp_vertex_buffer });
        frame.sp_cmd_list->Draw(RenderCommandList::Primitive::Triangle, 3, 1);
        frame.sp_cmd_list->Commit(true);

        m_sp_context->GetRenderCommandQueue().Execute({ *frame.sp_cmd_list });
        m_sp_context->Present();

        GraphicsApp::Render();
    }
};

int main(int argc, const char* argv[])
{
    return HelloTriangleApp().Run({ argc, argv });
}
```

Also you need a simple HLSL shader [Shaders/Shaders.hlsl](/Apps/Tutorials/01-HelloTriangle/Shaders/Shaders.hlsl).
Note how arguments of verftex shader function `VSMain(...)` are matching to input buffer layout description passed in Settings of `Program::Create(...)` call:
```cpp
struct PSInput
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
};

PSInput VSMain(float3 in_position : POSITION, 
               float3 in_color    : COLOR)
{
    PSInput output;
    output.position = float4(in_position, 1.0f);
    output.color    = float4(in_color, 1.0f);
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
```

The configuration file [Shaders/Shaders.cfg](/Apps/Tutorials/01-HelloTriangle/Shaders/Shaders.cfg) describes shader types along with entry points and optional sets of macro definitions used to prebuild shaders to bytecode:
```
frag=PSMain
vert=VSMain
```

Finally add build configuration [CMakeLists.txt](/Apps/Tutorials/01-HelloTriangle/CMakeLists.txt) powered by included module [Methane.cmake](CMake/Methane.cmake):
```cmake
include(Methane)

add_methane_application(MethaneHelloTriangle
    "Methane Hello Triangle"
    "HelloTriangleAppSimple.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Shaders.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Shaders.cfg"
    ""               # RESOURCES_DIR
    ""               # EMBEDDED_TEXTURES_DIR
    ""               # EMBEDDED_TEXTURES
    ""               # COPY_TEXTURES
    "Apps/Tutorials" # INSTALL_DIR
)
```

Now you have all in one application executable/bundle running on Windows/MacOS, which is rendering colored triangle in window with support of resizing the frame buffer.

## External Dependencies

All external dependencies of Methane Kit are gathered in [MethaneExternals](https://github.com/egorodet/MethaneExternals) repository. See [MethaneExternals/README.md](https://github.com/egorodet/MethaneExternals/blob/master/README.md) for more details.

## License

Methane Kit is distributed under [Apache 2.0 License](LICENSE): it is free to use and open for contribution!

*Copyright 2019 © Evgeny Gorodetskiy* [![Follow](https://img.shields.io/twitter/follow/egorodet.svg?style=social)](https://twitter.com/egorodet)
