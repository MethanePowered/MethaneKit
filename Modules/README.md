# Methane Kit Modules

## High-Level Architecture

Methane Kit architecture is clearly distributing library modules between 5 layers from low to high level of abstraction:
- [External](../Externals) external libraries and tools
- [Common](Common) libraries
- [Data](Data) processing libraries
- [Platform](Platform) abstraction libraries
- [Graphics](Graphics) API abstraction libraries
- [User Interface](UserInterface) libraries

![High Level Architecture](../Docs/Diagrams/MethaneKit_HighLevel_Architecture.svg)

## Modules Relations Diagram

Diagram generated with CMake using integrated GraphViz support shows detailed relations between CMake modules.
![Modules Relations Diagram](../Docs/Diagrams/MethaneKit_Modules_Relations.png)

## Features

- **Cross-platform**
  - Supported platforms:
    - Windows 10 with DirectX 12 graphics API
    - MacOS 10.13 with Metal graphics API
    - Ubuntu Linux with Vulkan graphics API (builds, but graphics api is not supported yet)
  - Application infrastructure is GLFW free:
    - [Methane CMake modules](/CMake) implement toolchain for cross-platform graphics applications build configuration.
    - [AppBase](/Modules/Platform/App/Include/Methane/Platform/AppBase.h) base application class and [App](/Modules/Platform/App/Include/Methane/Platform/App.h) platform-specific implementations.
    - [Mouse](/Modules/Platform/Input/Include/Methane/Platform/Mouse.h) and [Keyboard](/Modules/Platform/Input/Include/Methane/Platform/Keyboard.h) input classes with platform abstractions of input states.
    - [Resources Provider](Modules/Data/Primitives/Include/Methane/Data/ResourceProvider.hpp) allows loading shaders and textures from embedded application resources.
    - [Animations](Modules/Data/Animation/Include/Methane/Data/Animation.h) execution infrastructure.
    - [Graphics application](/Modules/Graphics/App/Include/Methane/Graphics/App.hpp) base template class with graphics app implementation:
- **One shader code for all graphics APIs on all platforms**
  - Shaders are written in HLSL 6.x
  - Shaders are converted to native API shading language (Metal on MacOS) at build time with [DirectXShaderCompiler](https://github.com/microsoft/DirectXShaderCompiler) and [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross) tools
  - Shaders are compiled at build time and embedded in application resources as bytecode (with all variations of preprocessor definitions configured by developer)
- **Easy to use object-oriented graphics API**
  - [Core graphics interfaces](/Modules/Graphics/Core/Include/Methane/Graphics):
    - [RenderContext](/Modules/Graphics/Core/Include/Methane/Graphics/RenderContext.h) and [Device](/Modules/Graphics/Core/Include/Methane/Graphics/Device.h) classes with frame-buffers management and swap-chain logic.
    - [Resource](/Modules/Graphics/Core/Include/Methane/Graphics/Resource.h) derived interface: [Buffer](/Modules/Graphics/Core/Include/Methane/Graphics/Buffer.h), [Texture](/Modules/Graphics/Core/Include/Methane/Graphics/Texture.h), [Sampler](/Modules/Graphics/Core/Include/Methane/Graphics/Sampler.h) to work with resources in GPU memory.
    - [Shader](/Modules/Graphics/Core/Include/Methane/Graphics/Shader.h) and [Program](/Modules/Graphics/Core/Include/Methane/Graphics/Program.h) providing unified access to compiled shaders with automatic input layout and uniform variables reflection.
    - [ProgramBindings](/Modules/Graphics/Core/Include/Methane/Graphics/ProgramBindings.h) simplifies binding resources to programs by uniform variable names, enabling fast bindings switching at runtime.
    - [RenderState](/Modules/Graphics/Core/Include/Methane/Graphics/RenderState.h) and [RenderPass](/Modules/Graphics/Core/Include/Methane/Graphics/RenderPass.h) used for inputs and outputs configuration of the graphics pipeline.
    - [RenderCommandList](/Modules/Graphics/Core/Include/Methane/Graphics/RenderCommandList.h) and [ParallelRenderCommandList](/Modules/Graphics/Core/Include/Methane/Graphics/ParallelRenderCommandList.h) for render commands encoding in one thread or in parallel contributing into single render pass.
    - [TransferCommandList](/Modules/Graphics/Core/Include/Methane/Graphics/TransferCommandList.h) for data transfer operations with GPU memory textures and buffers (WIP).
    - [CommandQueue](/Modules/Graphics/Core/Include/Methane/Graphics/CommandQueue.h) for execution of render commands encoded with command lists on graphics device. 
  - [Core interface extensions](/Modules/Graphics/Extensions/Include/Methane/Graphics):
    - [ImageLoader](/Modules/Graphics/Extensions/Include/Methane/Graphics/ImageLoader.h) class for loading images from image files or application resources into graphics textures.
    - [MeshBuffers.hpp](/Modules/Graphics/Extensions/Include/Methane/Graphics/MeshBuffers.hpp) template class used for unified vertex and index buffers management of the mesh object along with its textures.
    - [ScreenQuad](/Modules/Graphics/Extensions/Include/Methane/Graphics/ScreenQuad.h) class for rendering quad with texture on screen.
    - [SkyBox](/Modules/Graphics/Extensions/Include/Methane/Graphics/SkyBox.h) class for rendering sky-box with a cube-map texture.
  - [Common 3D graphics helpers](/Modules/Graphics/Helpers/Include/Methane/Graphics):
    - [FpsCounter](/Modules/Graphics/Helpers/Include/Methane/Graphics/FpsCounter.h) implements FPS calculation with moving-average algorithm.
    - [Camera](/Modules/Graphics/Helpers/Include/Methane/Graphics/Camera.h) and [ArcBallCamera](/Modules/Graphics/Helpers/Include/Methane/Graphics/ArcBallCamera.h) implement static scene camera and interactive arc-ball camera.
    - [Mesh](/Modules/Graphics/Helpers/Include/Methane/Graphics/Mesh) generators for Quad, Cube, Sphere, Icosahedron and Uber meshes with customizable vertex fields and layout.
    - Multi-octave Perlin [Noise](/Modules/Graphics/Helpers/Include/Methane/Graphics/Noise.h) generator.
- **User Interface** libraries:
  - UI application base class with integrated HUD, logo badge and help/parameters text panels
  - Typography library for fonts loading, rendering & text layout
  - Widgets library (under development)
- **Lightweight**: no heavy external dependencies, almost all external libraries are static or header only.
- **Performance oriented**:
  - Fast application startup, thanks to prebuilt shaders code.
  - [Deferred rendering](https://docs.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-render-multi-thread-render) approach with triple frame buffering by default.
  - Builtin API instrumentation with
    - [Tracy Realtime Frame Profiler](https://github.com/wolfpld/tracy)
    - [Intel ITT API](https://software.intel.com/en-us/vtune-amplifier-help-instrumentation-and-tracing-technology-apis) for performance analysis with [Intel Vtune Amplifier](https://software.intel.com/en-us/vtune) and [Intel Graphics Trace Analyzer](https://software.intel.com/en-us/gpa/graphics-trace-analyzer).
  
## Development Plans

- [x] Continuous integration system
- [x] Application user input with mouse and keyboard
- [x] Parallel command lists
- [x] Improved shaders toolset (use DXC & HLSL 6 instead of FXC, GLSLang & HLSL 5.1)
- [x] Text rendering
- [ ] Mesh loader
- [ ] Compute pipeline
- [ ] Vulkan API implementation
- [ ] Linux platform implementation
- [ ] Post-processing pipeline
- [ ] User-interface library
- [ ] Dynamic linking support