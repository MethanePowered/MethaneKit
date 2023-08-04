# Methane Kit Build Instructions 

- [Prerequisites](#prerequisites)
- [Fetch Sources](#fetch-sources)
  - [First time initialization](#first-time-initialization)
  - [Update sources to latest revision](#update-sources-to-latest-revision)
- [Building from Sources](#building-from-sources)
  - [Windows Build with Visual Studio](#windows-build-with-visual-studio)
  - [Linux Build with Unix Makefiles](#linux-build-with-unix-makefiles)
  - [MacOS Build with XCode](#macos-build-with-xcode)
  - [iOS and tvOS Build with XCode](#ios-and-tvos-build-with-xcode)
- [CMake Generator](#cmake-generator)
  - [CMake Options](#cmake-options)
  - [CMake Presets](#cMake-presets)

## Prerequisites

- **Common**
  - [CMake](https://cmake.org/download/) 3.18 or later
  - [Git](https://git-scm.com/downloads) (is required to fetch external dependent repositories using CPM)
- **Windows**
  - Windows 10 RS5 (build 1809) or later
  - Visual Studio 2019/22 with MSVC v142 or later
  - Windows 10 SDK (latest)
  - Visual C++ ATL Support (required by DXC)
- **Linux**
  - Ubuntu 20.04 or later
  - GCC 9 or later
  - X11, XCB, LCov (optional for code coverage) libraries
  ```console
  sudo apt-get update && sudo apt-get install build-essential git cmake lcov xcb libx11-dev libx11-xcb-dev libxcb-sync-dev libxcb-randr0-dev
  ```
- **MacOS**
  - MacOS 13 "Ventura" or later
  - XCode 14 or later with command-line tools
- **iOS / tvOS**
  - All MacOS prerequisites from above
  - iOS or tvOS simulator for running app in virtual environment
  - iOS or tvOS device with Apple Development Certificate (can be created with Apple ID from Xcode)
  
## Fetch Sources

### Notes
- Since v0.6 Methane Kit does not use Git submodules anymore and switches to [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake)
to fetch dependent repositories during CMake configuration stage, so it should be both possible to acquire sources
with `git clone` command or to download as ZIP-archive using `Code > Download ZIP` button.
- All [External](/Externals) dependencies are fetched to the `Build/Output/ExternalsCache/...` directory,
which can be changed by adding `-DCPM_SOURCE_CACHE=<cache_path>` to the CMake configuration command. 
- Consider enabling [paths longer than 260 symbols in Windows](https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation?tabs=powershell) to allow CMake generate build subdirectories normally. Long paths can be enabled with this command run from Administrator PowerShell terminal:
```powershell
New-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem" -Name "LongPathsEnabled" -Value 1 -PropertyType DWORD -Force
```

### First time initialization

```console
git clone https://github.com/MethanePowered/MethaneKit.git
cd MethaneKit
```

### Update sources to latest revision

```console
cd MethaneKit
git pull
```

## Building from Sources

### <img src="https://github.com/MethanePowered/MethaneKit/blob/master/Docs/Images/Platforms/Windows.png" width=24 valign="middle"> Windows Build with Visual Studio

Start Command Prompt, go to `MethaneKit` root directory and either start auxiliary build script
 or build with CMake command line
to generate Visual Studio 2019/22 solution:

```console
set OUTPUT_DIR=Build\Output\VisualStudio\Win64-DX
cmake -S . -B %OUTPUT_DIR%\Build -G "Visual Studio 16 2019" -A x64 -DCMAKE_INSTALL_PREFIX="%cd%\%OUTPUT_DIR%\Install"
cmake --build %OUTPUT_DIR%\Build --config Release --target install
```

[Methane Graphics RHI](/Modules/Graphics/RHI) is built using **DirectX 12** graphics API by default on Windows.
Vulkan graphics API can be used instead by adding cmake generator option `-DMETHANE_GFX_VULKAN_ENABLED:BOOL=ON`.
Auxiliary build script [Build/Windows/Build.bat](/Build/Windows/Build.bat) can make it more simple for you:

```console
./Build/Windows/Build.bat [--vs2022] [--win32] [--debug] [--vulkan] [--graphviz] [--analyze SONAR_TOKEN]
```

Alternatively root [CMakeLists.txt](/CMakeLists.txt) can be opened directly in Visual Studio or 
[any other IDE with native CMake support](/README.md#development-environments) and [built using CMake presets](#cmake-presets).

Run built applications from the installation directory `Build\Output\VisualStudio\Win64-DX\Install\Apps`

### <img src="https://github.com/MethanePowered/MethaneKit/blob/master/Docs/Images/Platforms/Linux.png" valign="middle"> Linux Build with Unix Makefiles

Start Terminal, go to `MethaneKit` root directory, generate Unix Makefiles and build them with CMake command line:

```console
OUTPUT_DIR=Build/Output/Linux
cmake -S . -B $OUTPUT_DIR/Build -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX="$(pwd)/$OUTPUT_DIR/Install"
cmake --build $OUTPUT_DIR/Build --config Release --target install --parallel 8
```

[Methane Graphics RHI](/Modules/Graphics/RHI) is built using **Vulkan** graphics API on Linux.
Auxiliary build script [Build/Unix/Build.sh](/Build/Unix/Build.sh) can make it more simple for you:

```console
./Build/Unix/Build.sh [--debug] [--graphviz] [--analyze SONAR_TOKEN]
```

Alternatively root [CMakeLists.txt](/CMakeLists.txt) can be opened directly in
[any IDE with native CMake support](/README.md#development-environments) and [built using CMake presets](#cmake-presets).

Run built applications from the installation directory `Build/Output/Linux/Install/Apps`.
Note that in Ubuntu Linux even GUI applications should be started from "Terminal" app,
because of `noexec` permission set on user's home directory by security reasons.

### <img src="https://github.com/MethanePowered/MethaneKit/blob/master/Docs/Images/Platforms/MacOS.png" width=24 valign="middle"> MacOS Build with XCode

Start Terminal, go to `MethaneKit` root directory, generate XCode workspace and build it with CMake command line:

```console
OUTPUT_DIR=Build/Output/XCode/macOS
cmake -S . -B $OUTPUT_DIR/Build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_INSTALL_PREFIX="$(pwd)/$OUTPUT_DIR/Install"
cmake --build $OUTPUT_DIR/Build --config Release --target install
```

Note that starting with XCode 12 and Clang 12 build architectures have to be specified explicitly
using CMake generator command line option `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` to build the fat binary.
This option should be omitted with earlier versions of Clang on macOS.

Auxiliary build script [Build/Unix/Build.sh](/Build/Unix/Build.sh) can make it more simple for you:

```console
./Build/Unix/Build.sh [--debug] [--graphviz] [--vulkan VULKAN_SDK_PATH] [--analyze SONAR_TOKEN]
```

Alternatively root [CMakeLists.txt](/CMakeLists.txt) can be opened directly in Visual Studio or 
[any other IDE with native CMake support](/README.md#development-environments) and [built using CMake presets](#cmake-presets).

[Methane Graphics RHI](/Modules/Graphics/RHI) is built using **Metal** graphics API on MacOS by default.
Vulkan graphics API can be used instead by adding cmake generator option `-DMETHANE_GFX_VULKAN_ENABLED:BOOL=ON` or
by running `Build/Unix/Build.sh --vulkan`, but it requires Vulkan SDK installation with MoltenVK driver implementation
on top of Metal.

Run built applications from the installation directory `Build/Output/XCode/Install/Apps`.

### <img src="https://github.com/MethanePowered/MethaneKit/blob/master/Docs/Images/Platforms/iOS.png" width=24 valign="middle"> iOS and tvOS Build with XCode

Start Terminal, go to `MethaneKit` root directory, generate XCode workspace and build it with CMake command line:

```console
OUTPUT_DIR=Build/Output/XCode/iOS
cmake -S . -B $OUTPUT_DIR/Build -G Xcode -DCMAKE_TOOLCHAIN_FILE="Externals/iOS-Toolchain.cmake" -DPLATFORM=[SIMULATORARM64|OS64|SIMULATOR_TVOS|TVOS] -DDEPLOYMENT_TARGET=15.0 -DENABLE_ARC:BOOL=ON [-DAPPLE_DEVELOPMENT_TEAM=12345X6ABC] -DCMAKE_INSTALL_PREFIX="$(pwd)/$OUTPUT_DIR/Install"
cmake --build $OUTPUT_DIR/Build --config Release --target install -- -allowProvisioningUpdates
```

Please pay attention to correctly setting the following options:
- `CMAKE_TOOLCHAIN_FILE` should be set to [Externals/iOS-Toolchain.cmake](/Externals/iOS-Toolchain.cmake) for iOS/tvOS build
- `PLATFORM` defines Apple target platform where application will run:
  - `OS64` - build for 64-bit iOS device: iPhone or iPad
  - `TVOS` - build for tvOS device
  - `SIMULATORARM64` or `SIMULATOR64` - build for iOS simulator with Arm64 or x64 architecture (depending on your Mac CPU)
  - `SIMULATOR_TVOS` - build for tvOS simulator
- `DEPLOYMENT_TARGET` defines minimum version of the target platform
- `APPLE_DEVELOPMENT_TEAM` defines development team identifier used for code signing,
which is required to build for running on physical device (not required for simulator build).
This identifier depends on your signing preferences in Xcode and should look like this `12345X6ABC`.
It can be found inside the pre-configured for signing `.xcodeproj` file contents (which can be opened with VSCode)
by searching for the value of parameter named `DEVELOPMENT_TEAM`.
- `ENABLE_ARC` set to `ON` to enable automatic reference counting in Apple Clang build.
- Note that `cmake --build` command contains extra argument `-allowProvisioningUpdates` passed to the underlying 
`xcodebuild` command line tool, which allows Xcode to update or add missing provisioning profiles for code signing.

Auxiliary build script [Build/Unix/Build.sh](/Build/Unix/Build.sh) can make it more simple for you:

```console
./Build/Unix/Build.sh --apple-platform [SIMULATORARM64|OS64|SIMULATOR_TVOS|TVOS] [--apple-dev-team 12345X6ABC] [--apple-deploy-target 15.1] [--debug]
```

Please open generated Xcode workspace, select application schema and run it on iOS / tvOS device or simulator from the Xcode IDE.

## CMake Generator

### CMake Options

Build options listed in table below can be used in cmake generator command line:

```console
cmake -G [Generator] ... -D[BUILD_OPTION_NAME]:BOOL=[ON|OFF]
```

| Build Option Name                               | Initial Value                     | Default Preset                    | Profiling Preset                 | Description                                                                         |
|-------------------------------------------------|-----------------------------------|-----------------------------------|----------------------------------|-------------------------------------------------------------------------------------|
| <sub>METHANE_GFX_VULKAN_ENABLED</sub>           | <sub><b>OFF</b></sub>             | <sub><b>...</b></sub>             | <sub><b>...</b></sub>            | <sub>Enable Vulkan graphics API instead of platform native API</sub>                |
| <sub>METHANE_APPS_BUILD_ENABLED</sub>           | <sub><b>ON</b></sub>              | <sub><b>ON</b></sub>              | <sub><b>ON</b></sub>             | <sub>Enable applications build</sub>                                                |
| <sub>METHANE_TESTS_BUILD_ENABLED</sub>          | <sub><b>ON</b></sub>              | <sub><b>ON</b></sub>              | <sub><b>OFF</b></sub>            | <sub>Enable tests build</sub>                                                       |
| <sub>METHANE_RHI_PIMPL_INLINE_ENABLED</sub>     | <sub><b>ON (in Release)</b></sub> | <sub><b>ON (in Release)</b></sub> | <sub><b>ON</b></sub>             | <sub>Enable RHI PIMPL implementation inlining</sub>                                 |
| <sub>METHANE_PRECOMPILED_HEADERS_ENABLED</sub>  | <sub><b>ON (not Apple)</b></sub>  | <sub><b>ON (not Apple)</b></sub>  | <sub><b>ON (not Apple)</b></sub> | <sub>Enable precompiled headers</sub>                                               |
| <sub>METHANE_CHECKS_ENABLED</sub>               | <sub><b>ON</b></sub>              | <sub><b>ON</b></sub>              | <sub><b>ON</b></sub>             | <sub>Enable runtime checks of input arguments</sub>                                 |
| <sub>METHANE_RUN_TESTS_DURING_BUILD</sub>       | <sub><b>ON</b></sub>              | <sub><em>OFF</em></sub>           | <sub><em>OFF</em></sub>          | <sub>Enable test auto-run after module build</sub>                                  |
| <sub>METHANE_UNITY_BUILD_ENABLED</sub>          | <sub><b>ON</b></sub>              | <sub><b>ON</b></sub>              | <sub><b>ON</b></sub>             | <sub>Enable unity build speedup for some modules</sub>                              |
| <sub>METHANE_CODE_COVERAGE_ENABLED</sub>        | <sub><em>OFF</em></sub>           | <sub><em>OFF</em></sub>           | <sub><em>OFF</em></sub>          | <sub>Enable code coverage data collection with GCC and Clang</sub>                  |
| <sub>METHANE_SHADERS_CODEVIEW_ENABLED</sub>     | <sub><em>OFF</em></sub>           | <sub><b>ON</b></sub>              | <sub><b>ON</b></sub>             | <sub>Enable shaders code symbols viewing in debug tools</sub>                       |
| <sub>METHANE_OPEN_IMAGE_IO_ENABLED</sub>        | <sub><em>OFF</em></sub>           | <sub><em>OFF</em></sub>           | <sub><em>OFF</em></sub>          | <sub>Enable using OpenImageIO library for images loading</sub>                      |
| <sub>METHANE_COMMAND_DEBUG_GROUPS_ENABLED</sub> | <sub><em>OFF</em></sub>           | <sub><b>ON</b></sub>              | <sub><b>ON</b></sub>             | <sub>Enable command list debug groups with frame markup</sub>                       |
| <sub>METHANE_LOGGING_ENABLED</sub>              | <sub><em>OFF</em></sub>           | <sub><em>OFF</em></sub>           | <sub><em>OFF</em></sub>          | <sub>Enable debug logging</sub>                                                     |
| <sub>METHANE_SCOPE_TIMERS_ENABLED</sub>         | <sub><em>OFF</em></sub>           | <sub><em>OFF</em></sub>           | <sub><b>ON</b></sub>             | <sub>Enable low-overhead profiling with scope-timers</sub>                          |
| <sub>METHANE_ITT_INSTRUMENTATION_ENABLED</sub>  | <sub><em>OFF</em></sub>           | <sub><b>ON</b></sub>              | <sub><b>ON</b></sub>             | <sub>Enable ITT instrumentation for trace capture with Intel GPA or VTune</sub>     |
| <sub>METHANE_ITT_METADATA_ENABLED</sub>         | <sub><em>OFF</em></sub>           | <sub><em>OFF</em></sub>           | <sub><b>ON</b></sub>             | <sub>Enable ITT metadata for tasks and events like function source locations</sub>  |
| <sub>METHANE_GPU_INSTRUMENTATION_ENABLED</sub>  | <sub><em>OFF</em></sub>           | <sub><em>OFF</em></sub>           | <sub><b>ON</b></sub>             | <sub>Enable GPU instrumentation to collect command list execution timings</sub>     |
| <sub>METHANE_TRACY_PROFILING_ENABLED</sub>      | <sub><em>OFF</em></sub>           | <sub><em>OFF</em></sub>           | <sub><b>ON</b></sub>             | <sub>Enable realtime profiling with Tracy</sub>                                     |
| <sub>METHANE_TRACY_PROFILING_ON_DEMAND</sub>    | <sub><em>OFF</em></sub>           | <sub><em>OFF</em></sub>           | <sub><b>ON</b></sub>             | <sub>Enable Tracy data collection on demand, after client connection</sub>          |
| <sub>METHANE_MEMORY_SANITIZER_ENABLED</sub>     | <sub><em>OFF</em></sub>           | <sub><em>OFF</em></sub>           | <sub><b>OFF</b></sub>            | <sub>Enable memory address sanitizer in compiler and linker</sub>                   |
| <sub>METHANE_APPLE_CODE_SIGNING_ENABLED</sub>   | <sub><em>OFF</em></sub>           | <sub><em>OFF</em></sub>           | <sub><b>OFF</b></sub>            | <sub>Enable code signing on Apple platforms (requires APPLE_DEVELOPMENT_TEAM)</sub> |

### CMake Presets

[CMake Presets](/CMakePresets.json) can be used to configure and build project with a set of predefined options (CMake 3.20 is required):
```console
cmake --preset [ConfigPresetName]
cmake --build --preset [BuildPresetName] --target install
```

Configure preset names `[ConfigPresetName]` can be listed with `cmake --list-presets` and are constructed according to the next schema using compatible kets according to preset matrix:
```console
[ConfigPresetName] = [VS2019|Xcode|Make|Ninja]-[Win64|Win32|Win|Lin|Mac|iOS|tvOS]-[Sim]-[DX|VK|MTL]-[Default|Profile|Scan]
```

| Presets Matrix | VS2019    | Xcode     | Make      | Ninja   |   
|----------------|-----------|-----------|-----------|---------|
| Win64          | DX / VK   | -         | -         | -       |
| Win32          | DX / VK   | -         | -         | -       |
| Win            | -         | -         | -         | DX / VK |
| Lin            | -         | -         | VK        | VK      |
| Mac            | -         | MTL       | -         | MTL     |
| iOS [-Sim]     | -         | MTL       | -         | -       |
| tvOS [-Sim]    | -         | MTL       | -         | -       |

Build preset names `[BuildPresetName]` can be listed with `cmake --list-presets build` and are constructed according to the same schema, but `Default` suffix should be replaced with `Debug` or `Release` configuration name. Only compatible configure and build presets can be used together either with the same name, or with `Debug` or `Release` instead of `Default`. `Ninja` presets should be used from 
"x64/x86 Native Tools Command Prompt for VS2019" command line environment on Windows or directly from Visual Studio.

[GitHub Actions](https://github.com/MethanePowered/MethaneKit/actions) CI builds are configured with these CMake presets.
CMake presets can be also used in [VS2019 and VS Code](https://devblogs.microsoft.com/cppblog/cmake-presets-integration-in-visual-studio-and-visual-studio-code/)
to reproduce CI builds on the development system with a few configuration options in IDE UI.
