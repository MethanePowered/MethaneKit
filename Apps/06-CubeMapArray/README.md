# Cube Map Array Tutorial

| <pre><b>Windows (DirectX 12)       </pre></b>                        | <pre><b>Linux (Vulkan)             </pre></b>                   | <pre><b>MacOS (Metal)              </pre></b>                  | <pre><b>iOS (Metal)</pre></b>                                                       |
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

## Application Controls

Common keyboard controls are enabled by the `Platform`, `Graphics` and `UserInterface` application controllers:
- [Methane::Platform::AppController](/Modules/Platform/App/README.md#platform-application-controller)
- [Methane::Graphics::AppController, AppContextController](/Modules/Graphics/App/README.md#graphics-application-controllers)
- [Methane::UserInterface::AppController](/Modules/UserInterface/App/README.md#user-interface-application-controllers)

## Continue learning

Continue learning Methane Graphics programming in the next tutorial [ParallelRendering](../07-ParallelRendering),
which is demonstrating multi-threaded rendering with `ParallelRenderCommandList`.