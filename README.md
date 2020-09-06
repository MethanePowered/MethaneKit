# Methane Kit <img src="https://github.com/egorodet/MethaneKit/blob/master/Resources/Images/Logo/MethaneLogoNameSmall.png" width=200 align="right" valign="middle">

[![Open in Gitpod](https://gitpod.io/button/open-in-gitpod.svg)](https://gitpod.io/#https://github.com/egorodet/MethaneKit)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![Contributions Welcome](https://img.shields.io/badge/contributions-welcome-purple.svg?style=flat)](https://github.com/egorodet/MethaneKit/issues)
[![Join the chat at https://gitter.im/MethaneKit/community](https://badges.gitter.im/MethaneKit/community.svg)](https://gitter.im/MethaneKit/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

**Easy to use modern 3D graphics abstraction API in C++17 for cross-platform applications development:**
- **Builds on top of modern native 3D graphics APIs**: DirectX 12 on Windows and Metal on MacOS, Vulkan on Linux will be enabled soon.
- **Simplifies modern graphics programming** with object-oriented higher-level graphics API inspired by simplicity of Apple's Metal and common shaders code in HLSL 6.
- **Provides cross-platform application infrastructure** from CMake-based toolchain to platform independent application and user input classes.

Click **"Open in Gitpod" button** above to explore Methane Kit codebase right away in a familiar VSCode-like IDE environment in your web-browser with navigation by symbols and cloud-build.
Download [release builds](https://github.com/egorodet/MethaneKit/releases) with built samples and tutorials to try them out.
Also you can get [Azure Pipelines](https://egorodet.visualstudio.com/MethaneKit/_build?view=runs) **build artifacts** to test latest versions from `develop` branch.  

|     Platform     |   Graphics API   |  Master Build Status  |  Develop Build Status  |
| ---------------- | ---------------- | --------------------- | ---------------------- |
| <img src="https://github.com/egorodet/MethaneKit/blob/master/Resources/Images/Platforms/Windows.png" width=24 valign="middle"> **Windows x64** | <img src="https://github.com/egorodet/MethaneKit/blob/master/Resources/Images/GraphicsApi/DirectX12Small.png" width=24 valign="middle"> DirectX 12 | [![Windows x64 Master Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=master&jobName=Windows_x64)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=master) | [![Windows x64 Develop Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=develop&jobName=Windows_x64)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=develop) |
| <img src="https://github.com/egorodet/MethaneKit/blob/master/Resources/Images/Platforms/Windows.png" width=24 valign="middle"> **Windows x86** | <img src="https://github.com/egorodet/MethaneKit/blob/master/Resources/Images/GraphicsApi/DirectX12Small.png" width=24 valign="middle"> DirectX 12 | [![Windows x86 Master Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=master&jobName=Windows_x86)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=master) | [![Windows x86 Develop Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=develop&jobName=Windows_x86)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=develop) |
| <img src="https://github.com/egorodet/MethaneKit/blob/master/Resources/Images/Platforms/MacOS.png" width=24 valign="middle"> **MacOS** | <img src="https://github.com/egorodet/MethaneKit/blob/master/Resources/Images/GraphicsApi/MetalSmall.png" width=24 valign="middle"> Metal | [![MacOS Master Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=master&jobName=MacOS)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=master) | [![MacOS Develop Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=develop&jobName=MacOS)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=develop) |
| <img src="https://github.com/egorodet/MethaneKit/blob/master/Resources/Images/Platforms/Ubuntu.png" width=24 valign="middle"> **Ubuntu Linux** | <img src="https://github.com/egorodet/MethaneKit/blob/master/Resources/Images/GraphicsApi/VulkanSmall.png" width=24 valign="middle"> Vulkan<sup><sup>soon</sup></sup> | [![Ubuntu Master Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=master&jobName=Ubuntu)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=master) | [![Ubuntu Develop Build Status](https://egorodet.visualstudio.com/MethaneKit/_apis/build/status/egorodet.MethaneKit?branchName=develop&jobName=Ubuntu)](https://egorodet.visualstudio.com/MethaneKit/_build/latest?definitionId=5&branchName=develop) |

[![Codacy](https://api.codacy.com/project/badge/Grade/25ae34bade994076bf636290791b3e0f)](https://www.codacy.com/app/egorodet/MethaneKit?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=egorodet/MethaneKit&amp;utm_campaign=Badge_Grade)
[![CodeFactor](https://www.codefactor.io/repository/github/egorodet/methanekit/badge)](https://www.codefactor.io/repository/github/egorodet/methanekit)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit&metric=alert_status)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit)
[![Maintainability Rating](https://sonarcloud.io/api/project_badges/measure?project=egorodet_MethaneKit&metric=sqale_rating)](https://sonarcloud.io/dashboard?id=egorodet_MethaneKit)
[![Total lines](https://tokei.rs/b1/github/egorodet/MethaneKit)](https://github.com/egorodet/MethaneKit)

![Asteroids sample on Windows](Apps/Samples/Asteroids/Screenshots/AsteroidsWinDirectX12.jpg)
<p align="center"><i><a href="#asteroids">Asteroids sample</a> demonstrating multi-threaded rendering with Methane Graphics API</i></p>

## Getting Started

## High-Level Architecture

Methane Kit architecture is clearly distributing library modules between 5 layers from low to high level of abstraction.
![High Level Architecture](Docs/Diagrams/MethaneKit_HighLevel_Architecture.svg)
<p align="center"><i>High-level architecture diagram module and layer headers contains clickable hyperlinks to source code.</i></p>

### Features

For detailed features description and development plans please refer to [Modules documentation](Modules).

### Tutorials

**NOTE**: Use name link to read tutorial documentation.

| Name  | Screenshot | Description |
| ----- | ---------- | ----------- |
| [Hello Triangle Tutorial](/Apps/Tutorials/01-HelloTriangle) | ![Hello Triangle on Windows](Apps/Tutorials/01-HelloTriangle/Screenshots/HelloTriangleWinDirectX12.jpg) | Colored triangle rendering in just 120 lines of code! |
| [Textured Cube Tutorial](/Apps/Tutorials/02-TexturedCube) | ![Textured Cube on Windows](Apps/Tutorials/02-TexturedCube/Screenshots/TexturedCubeWinDirectX12.jpg) | Textured cube introduces buffers and textures usage along with program bindings. |
| [Shadow Cube Tutorial](/Apps/Tutorials/03-ShadowCube) | ![Shadow Cube on Windows](Apps/Tutorials/03-ShadowCube/Screenshots/ShadowCubeWinDirectX12.jpg) | Shadow cube introduces multi-pass rendering with render passes. |
| [Typography Tutorial](/Apps/Tutorials/04-Typography) | ![Typography on Windows](Apps/Tutorials/04-Typography/Screenshots/TypographyWinDirectX12.jpg) | Typography demonstrates animated text rendering with dynamic font atlas updates using Methane UI. |

### Samples

**NOTE**: Use name link to read sample documentation.

| Name  | Screenshot | Description |
| ----- | ---------- | ----------- |
| [Asteroids Sample](/Apps/Samples/Asteroids) | ![Asteroids on Windows](Apps/Samples/Asteroids/Screenshots/AsteroidsWinDirectX12.jpg) | Demonstrate parallel multi-threaded rendering of the large number of heterogenous asteroid meshes. |

## Building from Sources 

### Prerequisites

- **Common**
  - Git (required to pull sub-modules)
  - CMake 3.15 or later
- **Windows**
  - Windows 10 RS5 (build 1809) or later
  - Visual Studio 2019 or later
  - MSVC v141 or later
  - Windows 10 SDK
- **MacOS**
  - MacOS 10.13 "El Capitan" or later
  - XCode 9 or later with command-line tools
- **Linux**
  - Ubuntu 18.04 or later

### Fetch Sources

**IMPORTANT:** Do not download source code via Zip archive, 
since it does not include content of [External](https://github.com/egorodet/MethaneExternals/tree/master) submodules.
Use `git clone` command as described below.

#### First time initialization

```console
git clone --recurse-submodules https://github.com/egorodet/MethaneKit.git
cd MethaneKit
```

#### Update sources to latest version

```console
cd MethaneKit
git pull && git submodule update --init --recursive
```

### Build

#### Windows Build with Visual Studio 2019

Start Command Prompt, then go to MethaneKit root directory (don't forget to pull dependent submodules as [described above](#fetch-sources))
and either start auxiliary build script [Build/Windows/Build.bat (--vs2019)](Build/Windows/Build.bat) or build with cmake manually:

```console
mkdir Build\Output\VisualStudio\Build && cd Build\Output\VisualStudio\Build
cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_INSTALL_PREFIX="%cd%\..\Install" "..\..\..\.."
cmake --build . --config Release --target install
```

Alternatively you can open root [CMakeLists.txt](CMakeLists.txt) directly in Visual Studio or [any other IDE of choice](#development-tools)
with native CMake support and build it using "Ninja" generator using provided configuration file [CMakeSettings.json](CMakeSettings.json).

Run built applications from the installation directory `Build\Output\VisualStudio\Install\Apps`

#### MacOS Build with XCode

Start terminal, then go to MethaneKit root directory (don't forget to pull dependent submodules as [described above](#fetch-sources))
and either start auxiliary build script [Build/Posix/Build.sh](Build/Posix/Build.sh) or build with cmake manually:
```console
mkdir -p Build/Output/XCode/Build && cd Build/Output/XCode/Build
cmake -H../../../.. -B. -G Xcode -DCMAKE_INSTALL_PREFIX="$(pwd)/../Install"
cmake --build . --config Release --target install
```

Alternatively you can open root [CMakeLists.txt](CMakeLists.txt) and build it from [any IDE with native CMake support](#development-tools).

Run applications from the installation directory `Build/Output/XCode/Install/Apps`

#### Linux Build with Unix Makefiles

Build on Linux works fine using "Unix Makefiles" generator, but the platform abstraction layer and graphics API implementation are currently stubbed.
So in spite of it builds, do not expect anything to work on Linux now.
```console
mkdir -p Build/Output/Linux/Build && cd Build/Output/Linux/Build
cmake -H../../../.. -B. -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX="$(pwd)/../Install"
cmake --build . --config Release --target install
```

## Supported Development Tools

### Development Environments

<a href="https://www.jetbrains.com/?from=MethaneKit" target="_blank"><img src="https://github.com/egorodet/MethaneKit/blob/master/Resources/Images/Partners/JetBrains.png" width=200 align="right" valign="bottom"/></a>
- Microsoft Visual Studio 2019
  - Solutions and projects build (generate with [Build.bat](/Build/Windows/Build.bat))
  - Ninja build with CMake native support (pre-configured with [CMakeSettings.json](/CMakeSettings.json))
- Apple XCode 11
  - XCode workspace and projects (generate with [Build.sh](/Build/Posix/Build.sh))
- Microsoft VS Code and [GitPod](https://gitpod.io/#https://github.com/egorodet/MethaneKit) (pre-configured with [.vscode/settings.json](/.vscode/settings.json))
- Jet Brains CLion (pre-configured with [.idea](/.idea))
- Jet Brains ReSharper C++ (pre-configured with [Folder.DotSettings](/Folder.DotSettings))
- Qt Creator with CMake native support

Methane Kit is being developed with support of [Jet Brains](https://www.jetbrains.com/?from=MethaneKit) development tools.
Open source project development license is provided free of charge to all key contributors of Methane Kit project.

### Profiling Tools

- [Tracy Profiler](https://github.com/wolfpld/tracy)
- [Intel Graphics Performance Analyzers](https://software.intel.com/en-us/gpa/graphics-trace-analyzer)

| Tracy Frame Profiler | Intel Graphics Trace Analyzer |
| -------------- | ----------------------------- |
| ![Asteroids Trace in Tracy](Apps/Samples/Asteroids/Screenshots/AsteroidsWinTracyProfiling.jpg) | ![Asteroids Trace in GPA Trace Analyzer](Apps/Samples/Asteroids/Screenshots/AsteroidsWinGPATraceAnalyzer.jpg) |

## External Dependencies

All external dependencies of Methane Kit are listed in [MethaneExternals](https://github.com/egorodet/MethaneExternals) repository. See [MethaneExternals/README.md](https://github.com/egorodet/MethaneExternals/blob/master/README.md) for more details.

## License

Methane Kit is distributed under [Apache 2.0 License](LICENSE): it is free to use and open for contributions!

*Copyright 2019-2020 © Evgeny Gorodetskiy* [![Follow](https://img.shields.io/twitter/follow/egorodet.svg?style=social)](https://twitter.com/egorodet)
