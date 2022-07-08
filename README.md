# Methane Kit <img src="https://github.com/egorodet/MethaneKit/blob/master/Docs/Images/Logo/MethaneLogoNameSmall.png" width=200 align="right" valign="middle">

[![Gitpod Ready-to-Code](https://img.shields.io/badge/Gitpod-ready--to--code-blue?logo=gitpod)](https://gitpod.io/#https://github.com/egorodet/MethaneKit)
[![Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=master)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=master)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Windows&metric=alert_status)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Windows)
[![CodeQL](https://github.com/egorodet/MethaneKit/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/egorodet/MethaneKit/actions/workflows/codeql-analysis.yml)

**Easy to use modern 3D graphics rendering abstraction API and cross-platform application framework:**
- **Builds on top of modern native 3D graphics APIs**: DirectX 12 on Windows, Metal on MacOS and Vulkan on Linux.
- **Simplifies modern graphics programming** with object-oriented medium-level graphics API inspired by simplicity of Apple Metal. Common shaders code in HLSL 6 is used on all platforms.
- **Provides cross-platform application framework** with CMake build toolchain, platform-independent application foundation classes and native-GUI layer for Windows, Linux and MacOS.

Download [release builds](https://github.com/egorodet/MethaneKit/releases) with pre-built samples, tutorials and tests to try them out. 
Check latest build status, tests, code coverage and analysis results or get build artifacts from [Azure Pipelines](https://egorodet.visualstudio.com/MethaneKit/_build?view=runs) CI and [Sonar Cloud](https://sonarcloud.io/organizations/egorodet-github).
See [Build Instructions](/Build/README.md) topic for manual build instructions and start learning [Methane Graphics Core](Modules/Graphics/Core) API with [Hello Triangle](/Apps/Tutorials/01-HelloTriangle) and other tutorials' documentation.

[![Open in Gitpod](https://gitpod.io/button/open-in-gitpod.svg)](https://gitpod.io/#https://github.com/egorodet/MethaneKit)

|     Platform     | Graphics API |  Master Build Status  |  Develop Build Status  |
| ---------------- | -------------| --------------------- | ---------------------- |
| <img src="https://github.com/egorodet/MethaneKit/blob/master/Docs/Images/Platforms/Windows.png" width=24 valign="middle"> **Windows x64** | <img src="https://github.com/egorodet/MethaneKit/blob/master/Docs/Images/GraphicsApi/DirectX12Small.png" width=24 valign="middle"> DirectX 12 | [![Windows x64 Master Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=master&jobName=Win64_DX_Release)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=master) | [![Windows x64 Develop Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=develop&jobName=Win64_DX_Release)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=develop) |
| <img src="https://github.com/egorodet/MethaneKit/blob/master/Docs/Images/Platforms/Windows.png" width=24 valign="middle"> **Windows x86** | <img src="https://github.com/egorodet/MethaneKit/blob/master/Docs/Images/GraphicsApi/DirectX12Small.png" width=24 valign="middle"> DirectX 12 | [![Windows x86 Master Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=master&jobName=Win32_DX_Release)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=master) | [![Windows x86 Develop Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=develop&jobName=Win32_DX_Release)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=develop) |
| <img src="https://github.com/egorodet/MethaneKit/blob/master/Docs/Images/Platforms/Windows.png" width=24 valign="middle"> **Windows x64** | <img src="https://github.com/egorodet/MethaneKit/blob/master/Docs/Images/GraphicsApi/VulkanSmall.png" width=24 valign="middle"> Vulkan | [![Windows x64 Master Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=master&jobName=Win64_VK_Release)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=master) | [![Windows x64 Develop Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=develop&jobName=Win64_VK_Release)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=develop) |
| <img src="https://github.com/egorodet/MethaneKit/blob/master/Docs/Images/Platforms/Windows.png" width=24 valign="middle"> **Windows x86** | <img src="https://github.com/egorodet/MethaneKit/blob/master/Docs/Images/GraphicsApi/VulkanSmall.png" width=24 valign="middle"> Vulkan | [![Windows x86 Master Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=master&jobName=Win32_VK_Release)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=master) | [![Windows x86 Develop Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=develop&jobName=Win32_VK_Release)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=develop) |
| <img src="https://github.com/egorodet/MethaneKit/blob/master/Docs/Images/Platforms/Ubuntu.png" width=24 valign="middle"> **Linux** | <img src="https://github.com/egorodet/MethaneKit/blob/master/Docs/Images/GraphicsApi/VulkanSmall.png" width=24 valign="middle"> Vulkan | [![Ubuntu Master Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=master&jobName=Ubuntu_VK_Release)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=master) | [![Ubuntu Develop Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=develop&jobName=Ubuntu_VK_Release)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=develop) |
| <img src="https://github.com/egorodet/MethaneKit/blob/master/Docs/Images/Platforms/MacOS.png" width=24 valign="middle"> **MacOS** | <img src="https://github.com/egorodet/MethaneKit/blob/master/Docs/Images/GraphicsApi/MetalSmall.png" width=24 valign="middle"> Metal | [![MacOS Master Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=master&jobName=MacOS_MTL_Release)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=master) | [![MacOS Develop Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=develop&jobName=MacOS_MTL_Release)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=develop) |

[Static code analysis](#static-code-analysis) scans are performed as a part of automated CI build process on master and develop branches
with up-to-date results published on [Sonar Cloud](https://sonarcloud.io/organizations/egorodet-github).

|     Platform     | Sonar Quality Gate |  Master Scan Status  |  Develop Scan Status  |
| ---------------- | ------------------ | -------------------- | --------------------- |
| <img src="https://github.com/egorodet/MethaneKit/blob/master/Docs/Images/Platforms/Windows.png" width=24 valign="middle"> **Windows** DirectX | [![Windows Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Windows&metric=alert_status)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Windows) | [![Windows Master Scan Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=master&jobName=Win64_DX_SonarScan)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=master) | [![Windows Develop Scan Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=develop&jobName=Win64_DX_SonarScan)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=develop) |
| <img src="https://github.com/egorodet/MethaneKit/blob/master/Docs/Images/Platforms/Ubuntu.png" width=24 valign="middle"> **Linux** Vulkan | [![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Linux&metric=alert_status)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Linux) | [![Linux Master Scan Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=master&jobName=Ubuntu_VK_SonarScan)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=master) | [![Linux Develop Scan Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=develop&jobName=Ubuntu_VK_SonarScan)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=develop) |
| <img src="https://github.com/egorodet/MethaneKit/blob/master/Docs/Images/Platforms/MacOS.png" width=24 valign="middle"> **MacOS** Metal | [![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_MacOS&metric=alert_status)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_MacOS) | [![MacOS Master Scan Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=master&jobName=MacOS_MTL_SonarScan)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=master) | [![MacOS Develop Scan Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=develop&jobName=MacOS_MTL_SonarScan)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=develop) |

[![Windows Maintainability Rating](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Windows&metric=sqale_rating)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Windows)
[![Windows Reliability Rating](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Windows&metric=reliability_rating)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Windows)
[![Windows Security Rating](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Windows&metric=security_rating)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Windows)
[![Windows Code Smells](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Windows&metric=code_smells)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Windows)
[![Windows Duplicated Lines (%)](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Windows&metric=duplicated_lines_density)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Windows)
[![Windows Coverage](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Windows&metric=coverage)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Windows)
[![Windows Lines of Code](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Windows&metric=ncloc)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Windows)
[![Total lines](https://tokei.rs/b1/github/egorodet/MethaneKit)](https://github.com/egorodet/MethaneKit)

![Asteroids sample on Windows](Apps/Samples/Asteroids/Screenshots/AsteroidsWinDirectX12.jpg)
<p align="center"><i><a href="https://github.com/egorodet/MethaneKit/tree/master/Apps/Samples/Asteroids">Asteroids sample</a> demonstrating multi-threaded rendering with Methane Graphics API</i></p>

## Getting Started

### High-Level Architecture

Methane Kit architecture is clearly distributing library modules between 5 layers from low to high level of abstraction.
![High Level Architecture](Docs/Diagrams/MethaneKit_HighLevel_Architecture.svg)

### Graphics Core Interfaces

[Methane Graphics Core](Modules/Graphics/Core) module implements a set of public object-oriented interfaces, 
which make modern graphics programming easy and convenient in a platform and API independent way.
![Graphics Core Interfaces](Docs/Diagrams/MethaneKit_GraphicsCore_Interfaces.svg)

### Tutorials

Start learning Methane Graphics API with [Hello Triangle](/Apps/Tutorials/01-HelloTriangle) tutorial documentation
and continue with others.

| <pre><b>Name / Link</b></pre>                                 | <pre><b>Screenshot</b></pre>                                                                                         | <pre><b>Description</b>                                         </pre>                                        |
|---------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------|
| 1. [Hello Triangle](/Apps/Tutorials/01-HelloTriangle)         | ![Hello Triangle on Windows](/Apps/Tutorials/01-HelloTriangle/Screenshots/HelloTriangleWinDirectX12.jpg)             | Colored triangle rendering in 100 lines of code.                                                              |
| 2. [Hello Cube](/Apps/Tutorials/02-HelloCube)                 | ![Hello Cube on Windows](/Apps/Tutorials/02-HelloCube/Screenshots/HelloCubeWinDirectX12.jpg)                         | Colored cube rendering in 200 lines of code with vertex and index buffers.                                    |
| 3. [Textured Cube](/Apps/Tutorials/03-TexturedCube)           | ![Textured Cube on Windows](/Apps/Tutorials/03-TexturedCube/Screenshots/TexturedCubeWinDirectX12.jpg)                | Textured cube introduces buffers, textures and samplers usage with Phong shading.                             |
| 4. [Shadow Cube](/Apps/Tutorials/04-ShadowCube)               | ![Shadow Cube on Windows](/Apps/Tutorials/04-ShadowCube/Screenshots/ShadowCubeWinDirectX12.jpg)                      | Shadow cube introduces multi-pass rendering with render passes.                                               |
| 5. [Typography](/Apps/Tutorials/05-Typography)                | ![Typography on Windows](/Apps/Tutorials/05-Typography/Screenshots/TypographyWinDirectX12.jpg)                       | Typography demonstrates animated text rendering with dynamic font atlas updates using Methane UI.             |
| 6. [Cube-Map Array](/Apps/Tutorials/06-CubeMapArray)          | ![Cube-Map Array on Windows](/Apps/Tutorials/06-CubeMapArray/Screenshots/CubeMapArrayWinDirectX12.jpg)               | Cube-map array texturing along with sky-box rendering is demonstrated in this tutorial.                       |
| 7. [Parallel Rendering](/Apps/Tutorials/07-ParallelRendering) | ![Parallel Rendering on Windows](/Apps/Tutorials/07-ParallelRendering/Screenshots/ParallelRenderingWinDirectX12.jpg) | Parallel rendering of the textured cube instances to the single render pass is demonstrated in this tutorial. |

### Samples

Methane samples demonstrate advanced techniques and usage scenarios with more complex implementation than tutorials above.

| <pre><b>Name / Link</b></pre> | <pre><b>Screenshot</b></pre> | <pre><b>Description</b>                                         </pre> |
| ----------------------------- | ---------------------------- | ---------------------------------------------------------------------- |
| [Asteroids](/Apps/Samples/Asteroids) | ![Asteroids on Windows](Apps/Samples/Asteroids/Screenshots/AsteroidsWinDirectX12.jpg) | Benchmark demonstrating parallel render commands encoding in a single render pass for the large number of heterogeneous asteroid objects processed in multiple threads. |

### Features

- **Cross-platform application & input classes**: Windows, MacOS and Linux are supported
  - **CMake modules** for convenient application build configuration, adding shaders and embedded resources
  - **HLSL-6 Shaders** serving all graphics APIs converted to native shader language and compiled in build time with SPIRV-Cross & DirectXCompiler
  - **HLSL++ Math** library with [HLSL-like syntax](https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dx-graphics-hlsl-reference) in C++
    and vector-instruction optimizations for different platforms
- **Modern Graphics API abstractions**: based on DirectX 12, Vulkan and Metal APIs
  - Render state and program configuration with compact initialization syntax
  - Program binding objects implement efficient binding of shader arguments to resources
  - Automatic resource state tracking used for automatic resource transition barriers setup
  - Resources are automatically retained from destroying while in use on GPU with shared pointers in command list state
  - Command list execution state tracking with optional GPU timestamps query on completion
  - Parallel render command list for multi-threaded render commands encoding in single render pass
  - Multiple command queues execution on GPU with synchronization using fences
  - Private GPU resources asynchronously updated through the upload command list and shared resource
  - Registry of named graphics objects enabling reuse of render states and graphics resources between renderer objects
- **Graphics primitives and extensions**:
  - Graphics application base class with per-frame resource management and frame buffers resizing enable effective triple buffering
  - Camera primitive and interactive arc-ball camera
  - Procedural mesh generation for quad, box, sphere, icosahedron and uber-mesh
  - Screen-quad and sky-box rendering extension classes
  - Texture loader (currently implemented with STB, planned for replacement with OpenImageIO)
- **User Interface**:
  - UI application base class with integrated HUD, logo badge and help/parameters text panels
  - Typography library for fonts loading, dynamic atlas updating, text rendering & layout
  - Widgets library (under development)
- **Platform Infrastructure**:
  - Base application with window management and input handling for Windows, MacOS and Linux
  - Events mechanism connecting emitters and receivers via callback interfaces
  - Animations subsystem
  - Embedded resource providers
- **Integrated debugging and profiling capabilities**:
  - Library instrumentation for performance analysis with [trace profiling tools](#trace-profiling-tools)
  - Debug names for all GPU objects and debug regions for graphics API calls for use with [frame profiling tools](#frame-profiling-and-debugging-tools)
- **Continuous integration** with automated multi-platform builds, unit-tests and
  [Sonar Cloud](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Windows) static code analysis
  in [Azure Pipelines](https://egorodet.visualstudio.com/MethaneKit/)

For detailed features description and development plans please refer to [Modules documentation](Modules).

# [Build Instructions](/Build/README.md)

- [Prerequisites](/Build/README.md#prerequisites)
- [Fetch Sources](/Build/README.md#fetch-sources)
  - [First time initialization](/Build/README.md#first-time-initialization)
  - [Update sources to latest revision](/Build/README.md#update-sources-to-latest-revision)
- [Building from Sources](/Build/README.md#building-from-sources)
  - [Windows Build with Visual Studio](/Build/README.md#-windows-build-with-visual-studio)
  - [MacOS Build with XCode](/Build/README.md#-macos-build-with-xcode)
  - [Linux Build with Unix Makefiles](/Build/README.md#-linux-build-with-unix-makefiles)
- [CMake Generator](/Build/README.md#cmake-generator)
  - [CMake Options](/Build/README.md#cmake-options)
  - [CMake Presets](/Build/README.md#cMake-presets)

## Supported Development Tools

### Development Environments

<a href="https://www.jetbrains.com/?from=MethaneKit" target="_blank"><img src="https://github.com/egorodet/MethaneKit/blob/master/Docs/Partners/JetBrains.png" width=200 align="right" valign="bottom"/></a>
- Microsoft Visual Studio 2019
  - Solutions and projects build (generate with [Build/Windows/Build.bat](/Build/Windows/Build.bat))
  - CMake native build support (pre-configured with [CMakePresets.json](/CMakePresets.json))
- Apple XCode
  - XCode workspace and projects (generate with [Build/Unix/Build.sh](/Build/Unix/Build.sh))
- Microsoft VS Code and [GitPod](https://gitpod.io/#https://github.com/egorodet/MethaneKit) (pre-configured with [CMakePresets.json](/CMakePresets.json) and [.vscode/settings.json](/.vscode/settings.json))
- Jet Brains CLion (pre-configured with [.idea](/.idea))
- Qt Creator with CMake native support

Methane Kit is being developed with support of [Jet Brains](https://www.jetbrains.com/?from=MethaneKit) development tools.
Open source project development license is provided free of charge to all key contributors of Methane Kit project.

### Static Code Analysis

Methane Kit comes with continuous C++ static code and code coverage analysis performed as a part of automated CI "Scan" builds
with up-to-date results published on [Sonar Cloud](https://sonarcloud.io/organizations/egorodet-github)
separately for all supported platforms.

| Master Scan Results  | Windows       | MacOS        | Linux        |     
| -------------------- | ------------- |------------- |------------- |
| Scan Build Status    | [![Windows Master Scan Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=master&jobName=Win64_DX_SonarScan)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=master) | [![MacOS Master Scan Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=master&jobName=MacOS_MTL_SonarScan)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=master) | [![Linux Master Scan Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=master&jobName=Ubuntu_VK_SonarScan)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=master) | 
| Quality Gate         | [![Windows Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Windows&metric=alert_status)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Windows) | [![MacOS Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_MacOS&metric=alert_status)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_MacOS) | [![Linux Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Linux&metric=alert_status)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Linux) |
| Maintainability      | [![Windows Maintainability Rating](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Windows&metric=sqale_rating)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Windows) | [![MacOS Maintainability Rating](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_MacOS&metric=sqale_rating)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_MacOS) | [![Linux Maintainability Rating](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Linux&metric=sqale_rating)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Linux) |
| Reliability          | [![Windows Reliability Rating](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Windows&metric=reliability_rating)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Windows) | [![MacOS Reliability Rating](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_MacOS&metric=reliability_rating)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_MacOS) | [![Linux Reliability Rating](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Linux&metric=reliability_rating)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Linux) | 
| Security             | [![Windows Security Rating](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Windows&metric=security_rating)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Windows) | [![MacOS Security Rating](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_MacOS&metric=security_rating)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_MacOS) | [![Linux Security Rating](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Linux&metric=security_rating)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Linux) | 
| Technical Debt       | [![Windows Technical Debt](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Windows&metric=sqale_index)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Windows) | [![MacOS Technical Debt](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_MacOS&metric=sqale_index)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_MacOS) | [![Linux Technical Debt](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Linux&metric=sqale_index)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Linux)
| Bugs                 | [![Windows Bugs](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Windows&metric=bugs)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Windows) | [![MacOS Bugs](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_MacOS&metric=bugs)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_MacOS) | [![Linux Bugs](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Linux&metric=bugs)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Linux) | 
| Vulnerabilities      | [![Windows Vulnerabilities](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Windows&metric=vulnerabilities)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Windows) | [![MacOS Vulnerabilities](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_MacOS&metric=vulnerabilities)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_MacOS) | [![Linux Vulnerabilities](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Linux&metric=vulnerabilities)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Linux)
| Code Smells          | [![Windows Code Smells](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Windows&metric=code_smells)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Windows) | [![MacOS Code Smells](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_MacOS&metric=code_smells)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_MacOS) | [![Linux Code Smells](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Linux&metric=code_smells)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Linux) |
| Duplicated Lines     | [![Windows Duplicated Lines (%)](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Windows&metric=duplicated_lines_density)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Windows) | [![MacOS Duplicated Lines (%)](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_MacOS&metric=duplicated_lines_density)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_MacOS) | [![Linux Duplicated Lines (%)](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Linux&metric=duplicated_lines_density)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Linux) | 
| Tests Coverage       | [![Windows Coverage](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Windows&metric=coverage)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Windows) | [![MacOS Coverage](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_MacOS&metric=coverage)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_MacOS) | [![Linux Coverage](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Linux&metric=coverage)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Linux) |
| Lines of Code        | [![Windows Lines of Code](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Windows&metric=ncloc)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Windows) | [![MacOS Lines of Code](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_MacOS&metric=ncloc)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_MacOS) | [![Linux Lines of Code](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit_Linux&metric=ncloc)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit_Linux) |

### Trace Profiling Tools

Methane Kit contains integrated instrumentation of all libraries for performance analysis with trace collection using following tools.
Please refer to [Methane Instrumentation](Modules/Common/Instrumentation) document for more details on trace collection instructions and related build options.

| [Tracy Frame Profiler](https://github.com/wolfpld/tracy) | [Intel Graphics Trace Analyzer](https://software.intel.com/en-us/gpa/graphics-trace-analyzer) |
| -------------------- | ----------------------------- |
| ![Asteroids Trace in Tracy](Apps/Samples/Asteroids/Screenshots/AsteroidsWinTracyProfiling.jpg) | ![Asteroids Trace in GPA Trace Analyzer](Apps/Samples/Asteroids/Screenshots/AsteroidsWinGPATraceAnalyzer.jpg) |

### Frame Profiling and Debugging Tools

- [Intel Graphics Frame Analyzer](https://software.intel.com/en-us/gpa/graphics-frame-analyzer)
- [Apple XCode Metal Debugger](https://developer.apple.com/documentation/metal/basic_tasks_and_concepts/viewing_your_gpu_workload_with_the_metal_debugger)
- [RenderDoc](https://renderdoc.org)
- [Microsoft PIX](https://devblogs.microsoft.com/pix/)
- [NVidia Nsight Graphics](https://developer.nvidia.com/nsight-graphics)

## External Dependencies

All external dependencies of Methane Kit are listed in [MethaneExternals](https://github.com/egorodet/MethaneExternals) repository. See [MethaneExternals/README.md](https://github.com/egorodet/MethaneExternals/blob/master/README.md) for more details.

## License

Methane Kit is distributed under [Apache 2.0 License](LICENSE): it is free to use and open for contributions!

*Copyright 2019-2021 © Evgeny Gorodetskiy* [![Follow](https://img.shields.io/twitter/follow/egorodet.svg?style=social)](https://twitter.com/egorodet)
