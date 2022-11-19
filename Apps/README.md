# Methane Applications

Cross-platform graphics applications built with Methane Kit.

## Tutorials

Start learning Methane Kit API with tutorial applications which are demonstrating different aspects of 
graphics rendering on simple examples using Methane Kit API in a cross-platform style.

| <pre><b>Name / Link</b></pre>                 | <pre><b>Screenshot</b></pre>                                                                         | <pre><b>Description</b>                                   </pre>                                  |
|-----------------------------------------------|------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------|
| 1. [Hello Triangle](01-HelloTriangle)         | ![Hello Triangle on Windows](01-HelloTriangle/Screenshots/HelloTriangleWinDirectX12.jpg)             | Colored triangle rendering in 130 lines of code.                                                  |
| 2. [Hello Cube](02-HelloCube)                 | ![Hello Cube on Windows](02-HelloCube/Screenshots/HelloCubeWinDirectX12.jpg)                         | Colored cube rendering in 220 lines of code.                                                      |
| 3. [Textured Cube](03-TexturedCube)           | ![Textured Cube on Windows](03-TexturedCube/Screenshots/TexturedCubeWinDirectX12.jpg)                | Textured cube introduces buffers and textures usage along with program bindings.                  |
| 4. [Shadow Cube](04-ShadowCube)               | ![Shadow Cube on Windows](04-ShadowCube/Screenshots/ShadowCubeWinDirectX12.jpg)                      | Shadow cube introduces multi-pass rendering with render passes.                                   |
| 5. [Typography](05-Typography)                | ![Typography on Windows](05-Typography/Screenshots/TypographyWinDirectX12.jpg)                       | Typography demonstrates animated text rendering with dynamic font atlas updates using Methane UI. |
| 6. [Cube-Map Array](06-CubeMapArray)          | ![Cube-Map Array on Windows](06-CubeMapArray/Screenshots/CubeMapArrayWinDirectX12.jpg)               | Cube-map array texturing along with sky-box rendering.                                            |
| 7. [Parallel Rendering](07-ParallelRendering) | ![Parallel Rendering on Windows](07-ParallelRendering/Screenshots/ParallelRenderingWinDirectX12.jpg) | Parallel rendering of the textured cube instances to the single render pass.                      |

## Common

Common applications source code beyond Methane Kit, used by multiple Tutorials to minimize code duplication.
- [AppSettings](Common/Include/Methane/Tutorials/AppSettings.h) - common application settings initialization helper;
- [TextureLabeler](Common/Include/Methane/Tutorials/TextureLabeler.h) - texture faces text labels renderer;
- [Shaders/Primitives.hlsl](Common/Shaders/Primitives.hlsl) - common shader pritimve functions.