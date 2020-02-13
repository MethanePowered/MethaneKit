# Methane Kit <img src="https://github.com/egorodet/MethaneKit/blob/master/Resources/Images/Logo/MethaneLogoNameSmall.png" width=200 align="right" valign="middle">

[![Open in Gitpod](https://gitpod.io/button/open-in-gitpod.svg)](https://gitpod.io/#https://github.com/egorodet/MethaneKit)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![Contributions Welcome](https://img.shields.io/badge/contributions-welcome-purple.svg?style=flat)](https://github.com/egorodet/MethaneKit/issues)
[![Join the chat at https://gitter.im/MethaneKit/community](https://badges.gitter.im/MethaneKit/community.svg)](https://gitter.im/MethaneKit/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

**Easy to use modern 3D graphics abstraction API in C++17 for cross-platform applications development:**
- **Built on top of modern native 3D graphics APIs**: DirectX 12 on Windows and Metal on MacOS, Vulkan on Linux will be enabled soon.
- **Simplifies modern graphics programming** with object-oriented graphics API inspired by simplicity of Apple's Metal.
- **Provides cross-platform application infrastructure** from CMake-based toolchain to application and user input classes.

Click **"Open in Gitpod" button** above to explore Methane Kit codebase right away in a familiar VSCode-like IDE environment in your web-browser with navigation by symbols and even cloud-build.

|     Platform     |  Master Build Status  |  Develop Build Status  |
| ---------------- | --------------------- | --------------------- |
| <img src="https://github.com/egorodet/MethaneKit/blob/master/Resources/Images/Platforms/Windows.png" width=24 valign="middle"> **Windows x64** | [![Windows x64 Master Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=master&jobName=Windows_x64)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=master) | [![Windows x64 Develop Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=develop&jobName=Windows_x64)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=develop) |
| <img src="https://github.com/egorodet/MethaneKit/blob/master/Resources/Images/Platforms/Windows.png" width=24 valign="middle"> **Windows x86** | [![Windows x86 Master Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=master&jobName=Windows_x86)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=master) | [![Windows x86 Develop Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=develop&jobName=Windows_x86)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=develop) |
| <img src="https://github.com/egorodet/MethaneKit/blob/master/Resources/Images/Platforms/MacOS.png" width=24 valign="middle"> **MacOS** | [![MacOS Master Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=master&jobName=macOS)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=master) | [![MacOS Develop Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=develop&jobName=macOS)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=develop) |
| <img src="https://github.com/egorodet/MethaneKit/blob/master/Resources/Images/Platforms/Ubuntu.png" width=24 valign="middle"> **Ubuntu Linux** | [![Ubuntu Master Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=master&jobName=Ubuntu)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=master) | [![Ubuntu Develop Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=develop&jobName=Ubuntu)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=develop) |

[![Codacy](https://api.codacy.com/project/badge/Grade/25ae34bade994076bf636290791b3e0f)](https://www.codacy.com/app/egorodet/MethaneKit?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=egorodet/MethaneKit&amp;utm_campaign=Badge_Grade)
[![CodeFactor](https://www.codefactor.io/repository/github/egorodet/methanekit/badge)](https://www.codefactor.io/repository/github/egorodet/methanekit)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit&metric=alert_status)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit)
[![Maintainability Rating](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit&metric=sqale_rating)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit)
[![Total lines](https://tokei.rs/b1/github/egorodet/MethaneKit)](https://github.com/egorodet/MethaneKit)

![Asteroids sample on Windows](Apps/Samples/Asteroids/Screenshots/AsteroidsWinDirectX12.jpg)
<p align="center"><i><a href="#asteroids">Asteroids sample</a> demonstrating multi-threaded rendering with Methane Graphics API</i></p>

## Table of Contents

1. [Features](#features)
2. [Development Plans](#development-plans)
3. [Getting Started](#getting-started)
   1. [Prerequisites](#prerequisites)
   2. [Fetch Sources](#fetch-sources)
   3. [Build](#build)
      1. [Windows Build](#windows-build)
      2. [MacOS Build](#macos-build)
      3. [Linux Build](#linux-build)
4. [Demo Applications](#demo-applications)
   1. [Tutorials](#tutorials)
      1. [Hello Triangle](#hello-triangle)
      2. [Textured Cube](#textured-cube)
      3. [Shadow Cube](#shadow-cube)
   2. [Samples](#samples)
      1. [Asteroids](#asteroids)
5. [Development Tools](#development-tools)
6. [External Dependencies](#external-dependencies)
7. [License](#license)

## Features

- **Cross-platform**
  - Supported platforms:
    - Windows 10 with DirectX 12 API
    - MacOS 10.13 with Metal API
  - Application infrastructure:
    - [Methane CMake modules](/CMake) implement toolchain for cross-platform graphics applications build setup.
    - [Base application class](/Modules/Platform/App/Include/Methane/Platform/AppBase.h) and platform-specific implementations are completely GLFW free!
    - [Mouse](/Modules/Platform/Input/Include/Methane/Platform/Mouse.h) and [Keyboard](/Modules/Platform/Input/Include/Methane/Platform/Keyboard.h) input classes with platform abstractions of input states.
    - [Resources Provider](Modules/Data/Primitives/Include/Methane/Data/ResourceProvider.hpp) allows loading shaders and textures from embedded application resources.
    - [Animations](Modules/Data/Animation/Include/Methane/Data/Animation.h) execution infrastructure.
    - [Paralel](/Modules/Data/Primitives/Include/Methane/Data/Parallel.hpp) execution primitives.
    - [Graphics application base template class](/Modules/Graphics/App/Include/Methane/Graphics/App.hpp) with basic multi-frame swap-chain management.
- **One shader code for all APIs**
  - Shaders are written in HLSL 5.1
  - Shaders are converted to native API shading language (Metal on MacOS) at build time with SPIRV-Cross toolchain
  - Shaders are compiled at build time and embedded in application resources as bytecode (with all variations of preprocessor definitions)
- **Easy to use object-oriented graphics API**
  - [Core graphics interfaces](/Modules/Graphics/Core/Include/Methane/Graphics):
    - [RenderContext](/Modules/Graphics/Core/Include/Methane/Graphics/RenderContext.h) and [Device](/Modules/Graphics/Core/Include/Methane/Graphics/Device.h) classes with frame-buffers management and swap-chain logic.
    - [Resource](/Modules/Graphics/Core/Include/Methane/Graphics/Resource.h) derived interfaces [Buffer](/Modules/Graphics/Core/Include/Methane/Graphics/Buffer.h), [Texture](/Modules/Graphics/Core/Include/Methane/Graphics/Texture.h), [Sampler](/Modules/Graphics/Core/Include/Methane/Graphics/Sampler.h) to work with GPU memory resources
    - [Shader](/Modules/Graphics/Core/Include/Methane/Graphics/Shader.h) and [Program](/Modules/Graphics/Core/Include/Methane/Graphics/Program.h) providing unified access to compiled shaders with input layout and uniform variables reflection
    - [ProgramBindings](/Modules/Graphics/Core/Include/Methane/Graphics/Program.h) simplifies binding resources to programs by uniform variable names and enables fast bindings switching at runtime
    - [RenderState](/Modules/Graphics/Core/Include/Methane/Graphics/RenderState.h) and [RenderPass](/Modules/Graphics/Core/Include/Methane/Graphics/RenderPass.h) used for inputs and outputs configuration of the graphics pipeline
    - [RenderCommandList](/Modules/Graphics/Core/Include/Methane/Graphics/RenderCommandList.h) and [CommandQueue](/Modules/Graphics/Core/Include/Methane/Graphics/CommandQueue.h) for render commands encoding and execution
    - [ParallelRenderCommandList](/Modules/Graphics/Core/Include/Methane/Graphics/ParallelRenderCommandList.h) for parallel render commands encoding contributing into single render pass.
  - [Core interface extensions](/Modules/Graphics/Extensions/Include/Methane/Graphics) like: ImageLoader for loading images to textures, SkyBox renderer, etc.
  - [Common 3D graphics helpers](/Modules/Graphics/Helpers/Include/Methane/Graphics) like: Camera, Timer, Mesh generator, etc.
- **Lightweight**:
  - No heavy external dependencies: almost all libraries are header only.
  - Fast application startup: thanks to embedded prebuilt shaders.
- **Performance oriented**:
  - Triple frame buffering swap-chain by default
  - Builtin API instrumentation with [Intel ITT API](https://software.intel.com/en-us/vtune-amplifier-help-instrumentation-and-tracing-technology-apis) for performance analysis with [Intel Vtune Amplifier](https://software.intel.com/en-us/vtune) and [Intel Graphics Trace Analyzer](https://software.intel.com/en-us/gpa/graphics-trace-analyzer)

## Development Plans

- [x] Continous integration system
- [x] Application user input with mouse and keyboard
- [x] Parallel command lists
- [ ] Mesh loader (Assimp)
- [ ] Text rendering
- [ ] Improved shaders toolset (use DXC & HLSL 6)
- [ ] Post-processing pipeline
- [ ] User interface library
- [ ] Dynamic linking support
- [ ] Compute pipeline

## Getting Started

### Prerequisites

- **Common**
  - Git (required to pull dependent sub-modules)
  - CMake 3.12 or later added to PATH
- **Windows**
  - Windows 10 RS5 (build 1809) or later
  - Visual Studio 2017 or later
  - MSVC v141 or later
  - Windows 10 SDK
- **MacOS**
  - MacOS 10.13 "El Capitan" or later
  - XCode 9 or later with Command Line tools
- **Linux**
  - Ubuntu 18.04 or later

### Fetch Sources

**IMPORTANT:** Do not download source code via Zip archive, since it does not include content of External submodules.
Use git as described below.

- **First time initialization**
```console
git clone --recurse-submodules --depth 1 https://github.com/egorodet/MethaneKit.git
```
- **Update sources to latest version**
```console
git pull && git submodule update --init --depth 1 --recursive
```

### Build

#### Windows Build

Start **"x64 Native Tools Command Prompt for VS2017"** (it initializes environment with VS path to Windows SDK, etc), then go to MethaneKit root directory (see instructions above to initialize repository and get latest code with submodules) and either start auxiliary build script [Build/Windows/Build.bat](Build/Windows/Build.bat) or build with cmake manually:
```console
mkdir Build\Output\VisualStudio\Build && cd Build\Output\VisualStudio\Build
cmake -G "Visual Studio 15 2017 Win64" -DCMAKE_INSTALL_PREFIX="%cd%\..\Install" "..\..\..\.."
cmake --build . --config Release --target install
```

Alternatively you can open root [CMakeLists.txt](CMakeLists.txt) directly in Visual Studio 2017 (or any other CMake IDE of your choice) and build it using "Ninja" generator and provided configuration file [CMakeSettings.json](CMakeSettings.json).

Run applications from the installation directory `Build\Output\VisualStudio\Install\Apps`.

#### MacOS Build

Start terminal, then go to MethaneKit root directory (see instructions above to initialize repository and get latest code with submodules) and either start auxiliary build script [Build/Posix/Build.sh](Build/Posix/Build.sh) or build with cmake manually:
```console
mkdir -p Build/Output/XCode/Build && cd Build/Output/XCode/Build
cmake -H../../../.. -B. -G Xcode -DCMAKE_INSTALL_PREFIX="$(pwd)/../Install"
cmake --build . --config Release --target install
```

Alternatively you can open root [CMakeLists.txt](CMakeLists.txt) directly in VSCode, CLion or QtCreatoror of choice and build it from there.

Run applications from the installation directory `Build/Output/XCode/Install/Apps`

#### Linux Build

Build on Linux with GCC works fine using "Unix Makefiles" generator, but the platform abstraction layer and graphics API implementation are currently stubbed.  So in spite of it builds, do not expect anything to work on Linux now.
```console
mkdir -p Build/Output/Linux/Build && cd Build/Output/Linux/Build
cmake -H../../../.. -B. -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX="$(pwd)/../Install"
cmake --build . --config Release --target install
```

## Demo Applications

### Tutorials

#### Hello Triangle

| Windows (DirectX 12) | MacOS (Metal) |
| -------------------- | ------------- |
| ![Hello Triangle on Windows](/Apps/Tutorials/01-HelloTriangle/Screenshots/HelloTriangleWinDirectX12.jpg) | ![Hello Triangle on MacOS](/Apps/Tutorials/01-HelloTriangle/Screenshots/HelloTriangleMacMetal.jpg) |

[Full source code](/Apps/Tutorials/01-HelloTriangle)

See how triangle rendering application is implemented in 120 lines of C++ code using Methane Kit ([HelloTriangleAppSimple.cpp](/Apps/Tutorials/01-HelloTriangle/HelloTriangleAppSimple.cpp)):
```cpp
#include <Methane/Graphics/Kit.h>

using namespace Methane;
using namespace Methane::Graphics;

struct HelloTriangleFrame final : AppFrame
{
    Ptr<RenderCommandList> sp_cmd_list;
    using AppFrame::AppFrame;
};

using GraphicsApp = App<HelloTriangleFrame>;
class HelloTriangleApp final : public GraphicsApp
{
private:
    Ptr<RenderState> m_sp_state;
    Ptr<Buffer>      m_sp_vertex_buffer;

public:
    HelloTriangleApp() : GraphicsApp(
        {                                        // Application settings:
            {                                    // platform_app:
                "Methane Hello Triangle",        // - name
                0.8, 0.8,                        // - width, height
            },                                   //
            {                                    // graphics_app:
                RenderPass::Access::None,        // - screen_pass_access
                false,                           // - animations_enabled
                true,                            // - show_hud_in_window_title
                false,                           // - show_logo_badge
            },                                   //
            {                                    // render_context:
                FrameSize(),                     // - frame_size placeholder: actual size is set in InitContext
                PixelFormat::BGRA8Unorm,         // - color_format
                PixelFormat::Unknown,            // - depth_stencil_format
                Color4f(0.0f, 0.2f, 0.4f, 1.0f), // - clear_color
            }
        })
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
        m_sp_vertex_buffer->SetData(
            Resource::SubResources
            {
                Resource::SubResource { reinterpret_cast<Data::ConstRawPtr>(triange_vertices.data()), vertex_buffer_size }
            }
        );

        m_sp_state = RenderState::Create(*m_sp_context,
            RenderState::Settings
            {
                Program::Create(*m_sp_context,
                    Program::Settings
                    {
                        Program::Shaders
                        {
                            Shader::CreateVertex(*m_sp_context, { Data::ShaderProvider::Get(), { "Triangle", "TriangleVS" } }),
                            Shader::CreatePixel(*m_sp_context,  { Data::ShaderProvider::Get(), { "Triangle", "TrianglePS" } }),
                        },
                        Program::InputBufferLayouts
                        {
                            Program::InputBufferLayout
                            {
                                Program::InputBufferLayout::Arguments
                                {
                                    { "input_position", "POSITION" },
                                    { "input_color",    "COLOR"    },
                                }
                            }
                        },
                        Program::ArgumentDescriptions { },
                        PixelFormats { m_sp_context->GetSettings().color_format }
                    }
                ),
                Viewports    { GetFrameViewport(m_sp_context->GetSettings().frame_size)    },
                ScissorRects { GetFrameScissorRect(m_sp_context->GetSettings().frame_size) },
            }
        );

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

    bool Render() override
    {
        if (!m_sp_context->ReadyToRender() || !GraphicsApp::Render())
            return false;

        m_sp_context->WaitForGpu(Context::WaitFor::FramePresented);
        HelloTriangleFrame& frame = GetCurrentFrame();

        frame.sp_cmd_list->Reset(m_sp_state);
        frame.sp_cmd_list->SetVertexBuffers({ *m_sp_vertex_buffer });
        frame.sp_cmd_list->Draw(RenderCommandList::Primitive::Triangle, 3);
        frame.sp_cmd_list->Commit(true);

        m_sp_context->GetRenderCommandQueue().Execute({ *frame.sp_cmd_list });
        m_sp_context->Present();

        return true;
    }

    void OnContextReleased() override
    {
        m_sp_vertex_buffer.reset();
        m_sp_state.reset();

        GraphicsApp::OnContextReleased();
    }
};

int main(int argc, const char* argv[])
{
    return HelloTriangleApp().Run({ argc, argv });
}
```

Also you need a simple HLSL shader [Shaders/Triangle.hlsl](/Apps/Tutorials/01-HelloTriangle/Shaders/Triangle.hlsl).
Note how members of `VSInput` structure passed as argument of vertex shader function `TriangleVS(...)` are matching to input buffer layout description passed in Settings of `Program::Create(...)` call:
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

Shaders configuration file [Shaders/Triangle.cfg](/Apps/Tutorials/01-HelloTriangle/Shaders/Triangle.cfg) is created in pair with every shaders file and describes shader types along with entry points and optional sets of macro definitions used to prebuild shaders to bytecode at build time:
```
frag=TrianglePS
vert=TriangleVS
```

Finally CMake build configuration [CMakeLists.txt](/Apps/Tutorials/01-HelloTriangle/CMakeLists.txt) of the application is powered by included Methane Cmake modules:
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
)
```

Now you have all in one application executable/bundle running on Windows/MacOS, which is rendering colored triangle in window with support of resizing the frame buffer.

#### Textured Cube

| Windows (DirectX 12) | MacOS (Metal) |
| -------------------- | ------------- |
| ![Textured Cube on Windows](/Apps/Tutorials/02-TexturedCube/Screenshots/TexturedCubeWinDirectX12.jpg) | ![Textured Cube on MacOS](/Apps/Tutorials/02-TexturedCube/Screenshots/TexturedCubeMacMetal.jpg) |

[Full source code](/Apps/Tutorials/02-TexturedCube)

#### Shadow Cube

| Windows (DirectX 12) | MacOS (Metal) |
| -------------------- | ------------- |
| ![Shadow Cube on Windows](/Apps/Tutorials/03-ShadowCube/Screenshots/ShadowCubeWinDirectX12.jpg) | ![Shadow Cube on MacOS](/Apps/Tutorials/03-ShadowCube/Screenshots/ShadowCubeMacMetal.jpg) |

[Full source code](/Apps/Tutorials/03-ShadowCube)

### Samples

#### Asteroids

| Windows (DirectX 12) | MacOS (Metal) |
| -------------------- | ------------- |
| ![Asteroids on Windows](/Apps/Samples/Asteroids/Screenshots/AsteroidsWinDirectX12.jpg) | ![Asteroids on MacOS](/Apps/Samples/Asteroids/Screenshots/AsteroidsMacMetal.jpg) |

[Full source code](/Apps/Samples/Asteroids)

Asteroids sample demontstrates multi-threaded rendering of large number of heterogenous objects with [ParallelRenderCommandList](/Modules/Graphics/Core/Include/Methane/Graphics/ParallelRenderCommandList.h).
Thousands of unique asteroid instances (1000-50000) are drawn with individual Draw-calls in parallel with a random combination of:
- random-generated mesh (from array of up to 1000 unique meshes)
- random generated perlin-noise array texture each with 3 projections (from array of up to 50 unique textures)
- random combination of coloring (from 72 color combinations)

Default parameters of asteroids simulation are selected depending on CPU HW cores count and can be displayed by `F2` key.
Overall complexity can be reduced / increased by pressing `[` / `]` keys.
Sample renders galaxy background using [SkyBox](Modules/Graphics/Extensions/Include/Methane/Graphics/SkyBox.h)
graphics extension and planet using generated [Sphere mesh](/Modules/Graphics/Helpers/Include/Methane/Graphics/Mesh.h) with spheric texture coordinates.
It also uses interactive [Arc-Ball camera](/Modules/Graphics/Helpers/Include/Methane/Graphics/ArcBallCamera.h)
rotated with mouse `LMB` and light rotated with `RMB` with keyboard shotcuts also available by pressing `F1` key.

Sample includes the following optimizations and features:
- Asteroid meshes use **dynamically selected LODs** depending on estimated screen size.
This allows to greatly reduce GPU overhead. Use `L` key to enable LODs coloring and `'` / `;` keys to increase / reduce overall LOD level and mesh detalization.
- **Parallel rendering** of asteroids array with individual draw-calls allows to be less CPU bound.
Multi-threading can be switched off for comparing with single-threaded rendering by pressing `P` key.
- All asteroid textures are bound to program uniform all at once as an **array of textures** to minimize number of program binding calls between draws.
Particular texture is selected on each draw call using index parameter in constants buffer.
Note that each asteroid texture is a texture 2d array itself with 3 mip-mapped textures used for triplanar projection.
- **Inverted depth buffer** (with values from 1 in foreground to 0 in background and greater-or-equal compare function)
is used to minimize frame buffer overdrawing by rendering in order from foreground to background: asteroids array with planet
are drawen first and sky-box afterwards.

Methane Asteroids sample was inspired by [Intel Asteroids D3D12](https://github.com/GameTechDev/asteroids_d3d12),
but the whole implementation was re-written from scratch using Methane Kit in cross-platform style.

## Development Tools

**Supported development environments**:<a href="https://www.jetbrains.com/?from=MethaneKit" target="_blank"><img src="https://github.com/egorodet/MethaneKit/blob/master/Resources/Images/Partners/JetBrains.png" width=200 align="right" valign="bottom"/></a>
- Microsoft Visual Studio 2017-2019
  - Solutions and projects build (generate with [Build.bat](/Build/Windows/Build.bat))
  - Ninja build with CMake native support (pre-configured with [CMakeSettings.json](/CMakeSettings.json))
- Apple XCode 10, 11
  - XCode workspace and projects (generate with [Build.sh](/Build/Posix/Build.sh))
- Microsoft VS Code (pre-configured with [.vscode/settings.json](/.vscode/settings.json))
- Jet Brains CLion (pre-configured with [.idea](/.idea))
- Jet Brains ReSharper C++ (pre-configured with [Folder.DotSettings](/Folder.DotSettings))
- Qt Creator with CMake native support

**NOTE:** Methane Kit is being developed with support of [Jet Brains](https://www.jetbrains.com/?from=MethaneKit) development tools.
Open source project development license is provided free of charge for all key contributors of Methane Kit project.

## External Dependencies

All external dependencies of Methane Kit are listed in [MethaneExternals](https://github.com/egorodet/MethaneExternals) repository. See [MethaneExternals/README.md](https://github.com/egorodet/MethaneExternals/blob/master/README.md) for more details.

## License

Methane Kit is distributed under [Apache 2.0 License](LICENSE): it is free to use and open for contributions!

*Copyright 2019-2020 © Evgeny Gorodetskiy* [![Follow](https://img.shields.io/twitter/follow/egorodet.svg?style=social)](https://twitter.com/egorodet)
