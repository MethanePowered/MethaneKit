# Methane Kit Build Instructions 

- [Prerequisites](#prerequisites)
- [Fetch Sources](#fetch-sources)
  - [First time initialization](#first-time-initialization)
  - [Update sources to latest revision](#update-sources-to-latest-revision)
- [Building from Sources](#building-from-sources)
  - [Windows Build with Visual Studio](#-windows-build-with-visual-studio)
  - [MacOS Build with XCode](#-macos-build-with-xcode)
  - [Linux Build with Unix Makefiles](#-linux-build-with-unix-makefiles)
- [CMake Generator](#cmake-generator)
  - [CMake Options](#cmake-options)
  - [CMake Presets](#cMake-presets)

## Prerequisites

- **Common**
  - Git (required to pull sub-modules)
  - CMake 3.18 or later
- **Windows**
  - Windows 10 RS5 (build 1809) or later
  - Visual Studio 2019/22 with MSVC v142 or later
  - Windows 10 SDK latest
- **MacOS**
  - MacOS 10.15 "Catalina" or later
  - XCode 11 or later with command-line tools
- **Linux**
  - Ubuntu 20.04 or later
  - GCC 9 or later
  - LCov, X11 & XCB libraries
  ```console
  sudo apt-get update && sudo apt-get install cmake g++ lcov xcb libx11-dev libx11-xcb-dev libxcb-randr0-dev
  ```

## Fetch Sources

**IMPORTANT!**
- <ins>Do not download source code via Zip archive</ins>, since it does not include content of 
[Externals](https://github.com/egorodet/MethaneExternals/tree/master) submodules.
Use `git clone` command as described below.
- Consider using <ins>short path for repository location on Windows</ins> (for example `c:\Git`),
which may be required to resolve problem with support of paths longer than 260 symbols in some Microsoft build tools.

#### First time initialization

```console
git clone --recurse-submodules https://github.com/egorodet/MethaneKit.git
cd MethaneKit
```

#### Update sources to latest revision

```console
cd MethaneKit
git pull && git submodule update --init --recursive
```

## Building from Sources

### <img src="https://github.com/egorodet/MethaneKit/blob/master/Docs/Images/Platforms/Windows.png" width=24 valign="middle"> Windows Build with Visual Studio

Start Command Prompt, go to `MethaneKit` root directory (don't forget to pull dependent submodules as [described above](#fetch-sources))
and either start auxiliary build script [Build/Windows/Build.bat](/Build/Windows/Build.bat) or build with CMake command line
to generate Visual Studio 2019/22 solution:

```console
set OUTPUT_DIR=Build\Output\VisualStudio\Win64-DX
cmake -S . -B %OUTPUT_DIR%\Build -G "Visual Studio 16 2019" -A x64 -DCMAKE_INSTALL_PREFIX="%cd%\%OUTPUT_DIR%\Install"
cmake --build %OUTPUT_DIR%\Build --config Release --target install
```

Alternatively root [CMakeLists.txt](/CMakeLists.txt) can be opened directly in Visual Studio or 
[any other IDE with native CMake support](#development-environments) and [built using CMake presets](#cmake-presets).

[Methane Graphics Core](/Modules/Graphics/Core) is built using **DirectX 12** graphics API by default on Windows. 
Vulkan graphics API can be used instead by adding cmake generator option `-DMETHANE_GFX_VULKAN_ENABLED:BOOL=ON` or 
by running `Build/Windows/Build.bat --vulkan`.

Run built applications from the installation directory `Build\Output\VisualStudio\Win64-DX\Install\Apps`

### <img src="https://github.com/egorodet/MethaneKit/blob/master/Docs/Images/Platforms/MacOS.png" width=24 valign="middle"> MacOS Build with XCode

Start Terminal, go to `MethaneKit` root directory (don't forget to pull dependent submodules as [described above](#fetch-sources))
and either start auxiliary build script [Build/Unix/Build.sh](/Build/Unix/Build.sh) or build with CMake command line:

```console
OUTPUT_DIR=Build/Output/XCode
cmake -S . -B $OUTPUT_DIR/Build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_INSTALL_PREFIX="$(pwd)/$OUTPUT_DIR/Install"
cmake --build $OUTPUT_DIR/Build --config Release --target install
```

Note that starting with XCode 12 and Clang 12 build architectures have to be specified explicitly
using CMake generator command line option `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` to build the fat binary.
This option should be omitted with earlier versions of Clang on macOS.

Alternatively root [CMakeLists.txt](/CMakeLists.txt) can be opened directly in Visual Studio or 
[any other IDE with native CMake support](#development-environments) and [built using CMake presets](#cmake-presets).

[Methane Graphics Core](/Modules/Graphics/Core) is built using **Metal** graphics API on MacOS by default.
Vulkan graphics API can be used instead by adding cmake generator option `-DMETHANE_GFX_VULKAN_ENABLED:BOOL=ON` or
by running `Build/Unix/Build.sh --vulkan`, but it requires Vulkan SDK installation with MoltenVK driver implementation
on top of Metal, which is not currently supporting all extensions required by Methane Kit.

Run built applications from the installation directory `Build/Output/XCode/Install/Apps`.

### <img src="https://github.com/egorodet/MethaneKit/blob/master/Docs/Images/Platforms/Ubuntu.png" width=24 valign="middle"> Linux Build with Unix Makefiles

Start Terminal, go to `MethaneKit` root directory (don't forget to pull dependent submodules as [described above](#fetch-sources))
and either start auxiliary build script [Build/Unix/Build.sh](/Build/Unix/Build.sh) or build with CMake command line:

```console
OUTPUT_DIR=Build/Output/Linux
cmake -S . -B $OUTPUT_DIR/Build -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX="$(pwd)/$OUTPUT_DIR/Install"
cmake --build $OUTPUT_DIR/Build --config Release --target install --parallel 8
```

[Methane Graphics Core](/Modules/Graphics/Core) is built using **Vulkan** graphics API on Linux.

Alternatively root [CMakeLists.txt](/CMakeLists.txt) can be opened directly in 
[any IDE with native CMake support](#development-environments) and [built using CMake presets](#cmake-presets).

Run built applications from the installation directory `Build/Output/Linux/Install/Apps`.
Note that in Ubuntu Linux even GUI applications should be started from "Terminal" app, 
because of `noexec` permission set on user's home directory by security reasons.

## CMake Generator

### CMake Options

Build options listed in table below can be used in cmake generator command line:

```console
cmake -G [Generator] ... -D[BUILD_OPTION_NAME]:BOOL=[ON|OFF]
```

| Build Option Name                               | Initial Value           | Default Preset          | Profiling Preset        | Description             |
| ----------------------------------------------- |-------------------------| ----------------------- | ----------------------- | ----------------------- |
| <sub>METHANE_GFX_VULKAN_ENABLED</sub>           | <sub><b>OFF</b></sub>   | <sub><b>...</b></sub>   | <sub><b>...</b></sub>   | <sub>Enable Vulkan graphics API instead of platform native API</sub> |
| <sub>METHANE_APPS_BUILD_ENABLED</sub>           | <sub><b>ON</b></sub>    | <sub><b>ON</b></sub>    | <sub><b>ON</b></sub>    | <sub>Enable applications build</sub> |
| <sub>METHANE_TESTS_BUILD_ENABLED</sub>          | <sub><b>ON</b></sub>    | <sub><b>ON</b></sub>    | <sub><b>OFF</b></sub>   | <sub>Enable tests build</sub> |
| <sub>METHANE_CHECKS_ENABLED</sub>               | <sub><b>ON</b></sub>    | <sub><b>ON</b></sub>    | <sub><b>ON</b></sub>    | <sub>Enable runtime checks of input arguments</sub> |
| <sub>METHANE_RUN_TESTS_DURING_BUILD</sub>       | <sub><b>ON</b></sub>    | <sub><em>OFF</em></sub> | <sub><em>OFF</em></sub> | <sub>Enable test auto-run after module build</sub> |
| <sub>METHANE_UNITY_BUILD_ENABLED</sub>          | <sub><b>ON</b></sub>    | <sub><b>ON</b></sub>    | <sub><b>ON</b></sub>    | <sub>Enable unity build speedup for some modules</sub> |
| <sub>METHANE_CODE_COVERAGE_ENABLED</sub>        | <sub><em>OFF</em></sub> | <sub><em>OFF</em></sub> | <sub><em>OFF</em></sub> | <sub>Enable code coverage data collection with GCC and Clang</sub>  |
| <sub>METHANE_SHADERS_CODEVIEW_ENABLED</sub>     | <sub><em>OFF</em></sub> | <sub><b>ON</b></sub>    | <sub><b>ON</b></sub>    | <sub>Enable shaders code symbols viewing in debug tools</sub>  |
| <sub>METHANE_SHADERS_VALIDATION_ENABLED</sub>   | <sub><em>ON</em></sub>  | <sub><b>ON</b></sub>    | <sub><b>ON</b></sub>    | <sub>Enable shaders DXIL code signature validation</sub>  |
| <sub>METHANE_OPEN_IMAGE_IO_ENABLED</sub>        | <sub><em>OFF</em></sub> | <sub><em>OFF</em></sub> | <sub><em>OFF</em></sub> | <sub>Enable using OpenImageIO library for images loading</sub> |
| <sub>METHANE_COMMAND_DEBUG_GROUPS_ENABLED</sub> | <sub><em>OFF</em></sub> | <sub><b>ON</b></sub>    | <sub><b>ON</b></sub>    | <sub>Enable command list debug groups with frame markup</sub>  |
| <sub>METHANE_LOGGING_ENABLED</sub>              | <sub><em>OFF</em></sub> | <sub><em>OFF</em></sub> | <sub><em>OFF</em></sub> | <sub>Enable debug logging</sub> |
| <sub>METHANE_SCOPE_TIMERS_ENABLED</sub>         | <sub><em>OFF</em></sub> | <sub><em>OFF</em></sub> | <sub><b>ON</b></sub>    | <sub>Enable low-overhead profiling with scope-timers</sub> |
| <sub>METHANE_ITT_INSTRUMENTATION_ENABLED</sub>  | <sub><em>OFF</em></sub> | <sub><b>ON</b></sub>    | <sub><b>ON</b></sub>    | <sub>Enable ITT instrumentation for trace capture with Intel GPA or VTune</sub> |
| <sub>METHANE_ITT_METADATA_ENABLED</sub>         | <sub><em>OFF</em></sub> | <sub><em>OFF</em></sub> | <sub><b>ON</b></sub>    | <sub>Enable ITT metadata for tasks and events like function source locations</sub> |
| <sub>METHANE_GPU_INSTRUMENTATION_ENABLED</sub>  | <sub><em>OFF</em></sub> | <sub><em>OFF</em></sub> | <sub><b>ON</b></sub>    | <sub>Enable GPU instrumentation to collect command list execution timings</sub> |
| <sub>METHANE_TRACY_PROFILING_ENABLED</sub>      | <sub><em>OFF</em></sub> | <sub><em>OFF</em></sub> | <sub><b>ON</b></sub>    | <sub>Enable realtime profiling with Tracy</sub> |
| <sub>METHANE_TRACY_PROFILING_ON_DEMAND</sub>    | <sub><em>OFF</em></sub> | <sub><em>OFF</em></sub> | <sub><b>ON</b></sub>    | <sub>Enable Tracy data collection on demand, after client connection</sub> |

### CMake Presets

[CMake Presets](/CMakePresets.json) can be used to configure and build project with a set of predefined options (CMake 3.20 is required):
```console
cmake --preset [ConfigPresetName]
cmake --build --preset [BuildPresetName] --target install
```

Configure preset names `[ConfigPresetName]` can be listed with `cmake --list-presets` and are constructed according to the next schema using compatible kets according to preset matrix:
```console
[ConfigPresetName] = [VS2019|Xcode|Make|Ninja]-[Win64|Win32|Win|Lin|Mac]-[DX|VK|MTL]-[Default|Profile|Scan]
```

| Preset Matrix | VS2019    | Xcode     | Make      | Ninja     |   
|---------------|-----------|-----------|-----------|-----------|
| Win64         | DX / VK   | -         | -         | -         |
| Win32         | DX / VK   | -         | -         | -         |
| Win           | -         | -         | -         | DX / VK   |
| Mac           | -         | MTL       | -         | MTL       |
| Lin           | -         | -         | VK        | VK        |

Build preset names `[BuildPresetName]` can be listed with `cmake --list-presets build` and are constructed according to the same schema, but `Default` suffix should be replaced with `Debug` or `Release` configuration name. Only compatible configure and build presets can be used together either with the same name, or with `Debug` or `Release` instead of `Default`. `Ninja` presets should be used from 
"x64/x86 Native Tools Command Prompt for VS2019" command line environment on Windows or directly from Visual Studio.

[Azure Pipelines](https://egorodet.visualstudio.com/MethaneKit/_build?view=runs) CI builds are configured with these CMake presets.
CMake presets can be also used in [VS2019 and VS Code](https://devblogs.microsoft.com/cppblog/cmake-presets-integration-in-visual-studio-and-visual-studio-code/)
to reproduce CI builds on the development system with a few configuration options in IDE UI.
