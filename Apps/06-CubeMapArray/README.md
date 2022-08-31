# Cube Map Array Tutorial

| Windows (DirectX 12)                                                 | Linux (Vulkan)                                                  | MacOS (Metal)                                                  | iOS (Metal)                                                                         |
|----------------------------------------------------------------------|-----------------------------------------------------------------|----------------------------------------------------------------|-------------------------------------------------------------------------------------|
| ![CubeMapArray on Windows](Screenshots/CubeMapArrayWinDirectX12.jpg) | ![CubeMapArray on Linux](Screenshots/CubeMapArrayLinVulkan.jpg) | ![CubeMapArray on MacOS](Screenshots/CubeMapArrayMacMetal.jpg) | ![CubeMapArray on iOS](Screenshots/CubeMapArrayIOSMetal.jpg) * No simulator support |

This tutorial demonstrates cube-map array texturing and sky-box rendering with Methane Kit:
  - [CubeMapArrayApp.h](CubeMapArrayApp.h)
  - [CubeMapArrayApp.cpp](CubeMapArrayApp.cpp)
  - [Shaders/CubeMapArray.hlsl](Shaders/CubeMapArray.hlsl)
  - [Shaders/CubeMapArrayUniforms.h](Shaders/CubeMapArrayUniforms.h)

Tutorial demonstrates the following techniques:
  - Loading face images to cube map texture and using it for sky-box rendering in background;
  - Creating cube-map array render target texture;
  - Rendering text labels to the faces of cube-map array texture via separate render passes 
    using helper class [TextureLabeler](/Apps/Common/Include/TextureLabeler.h);
  - Instanced rendering of multiple cubes displaying all faces of the pre-rendered cube-map array texture;
  - Using Sky-box rendering extension with panoramic cube-map texture loaded from image files.

## Continue learning

Continue learning Methane Graphics programming in the next tutorial [ParallelRendering](../07-ParallelRendering),
which is demonstrating multi-threaded rendering with `ParallelRenderCommandList`.