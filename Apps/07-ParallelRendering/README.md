# Parallel Rendering Tutorial

| Windows (DirectX 12) | MacOS (Metal) | Linux (Vulkan)                                                  |
| -------------------- | ------------- |-----------------------------------------------------------------|
| ![ParallelRendering on Windows](Screenshots/ParallelRenderingWinDirectX12.jpg) | ![ParallelRendering on MacOS](Screenshots/ParallelRenderingMacMetal.jpg) | ![ParallelRendering on Linux](Screenshots/ParallelRenderingLinVulkan.jpg) |

This tutorial demonstrates multi-threaded rendering with `ParallelRenderCommandList` using Methane Kit:
  - [ParallelRenderingApp.h](ParallelRenderingApp.h)
  - [ParallelRenderingApp.cpp](ParallelRenderingApp.cpp)
  - [ParallelRenderingAppController.h](ParallelRenderingAppController.h)
  - [ParallelRenderingAppController.cpp](ParallelRenderingAppController.cpp)
  - [Shaders/ParallelRendering.hlsl](Shaders/ParallelRendering.hlsl)
  - [Shaders/ParallelRenderingUniforms.h](Shaders/ParallelRenderingUniforms.h)

Tutorial demonstrates the following techniques:
  - Creating render target texture 2D array;
  - Rendering text labels to the faces of texture 2D array via separate render passes
    using helper class [TextureLabeler](/Apps/Common/Include/TextureLabeler.h);
  - Using [MeshBuffers](Modules/Graphics/Extensions/Include/Methane/Graphics/MeshBuffers.hpp) extension
    primitive to represent cube instances, create index and vertex buffers and render with a single Draw command.
  - Using single addressable uniforms buffer to store an array of uniform structures for
    all cube instance parameters at once and binding array elements in that buffer to the particular
    cube instance draws with byte offset in buffer memory.
  - Binding faces of the texture 2D array to the cube instances to display rendering thread number as text on cube faces.
  - Using [TaskFlow](https://github.com/taskflow/taskflow) library for task-based parallelism and parallel for loops.
  - Randomly distributing cubes between render threads and rendering them in parallel using `ParallelRenderCommandList` all to the screen render pass.
  - Use Methane instrumentation to profile application execution on CPU and GPU 
    using [Tracy](https://github.com/wolfpld/tracy) or [Intel GPA Trace Analyzer](https://software.intel.com/en-us/gpa/graphics-trace-analyzer).

## Application Controller

Keyboard actions are enabled with [ParallelRenderingAppController](ParallelRenderingAppController.h) class
derived from [Platform::Keyboard::ActionControllerBase](/Modules/Platform/Input/Include/Methane/Platform/KeyboardActionControllerBase.hpp):

| Parallel Rendering App Action | Keyboard Shortcut |
|-------------------------------|-------------------|
| Switch Parallel Rendering     | `P`               |
| Increase Cubes Grid Size      | `+`               |
| Decrease Cubes Grid Size      | `-`               |
| Increase Render Threads Count | `]`               |
| Decrease Render Threads Count | `[`               |