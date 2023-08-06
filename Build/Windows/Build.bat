@REM Run 'Build.bat' with optional arguments:
@REM   --vs2019   - build with Visual Studio 2019 instead of Visual Studio 2019 by default
@REM   --win32    - 32-bit build instead of 64-bit by default
@REM   --debug    - Debug build instead of Release build by default
@REM   --vulkan   - use Vulkan graphics API instead of DirectX 12 by default
@REM   --graphviz - enable GraphViz cmake module diagrams generation in Dot and Png formats
@REM   --analyze SONAR_TOKEN - run local build with Sonar Scanner static analysis and submit results to the server using token login
@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION

SET BUILD_VERSION_MAJOR=0
SET BUILD_VERSION_MINOR=7
SET BUILD_VERSION_PATCH=3
SET BUILD_VERSION=%BUILD_VERSION_MAJOR%.%BUILD_VERSION_MINOR%.%BUILD_VERSION_PATCH%

SET OUTPUT_DIR=%~dp0..\Output
SET SOURCE_DIR=%~dp0..\..
SET START_DIR=%cd%

REM Parse command line options
:options_loop
IF NOT "%1"=="" (
    IF "%1"=="--vs2019" (
        SET USE_VS2019=1
    )
    IF "%1"=="--win32" (
        SET WIN32_BUILD=1
    )
    IF "%1"=="--debug" (
        SET DEBUG_BUILD=1
    )
    IF "%1"=="--vulkan" (
        SET VULKAN_API_ENABLED=ON
    )
    IF "%1"=="--graphviz" (
        SET GRAPHVIZ_ENABLED=1
    )
    IF "%1"=="--analyze" (
        SET ANALYZE_BUILD=1
        SET SONAR_TOKEN=%2
        SHIFT
    )
    SHIFT
    GOTO :options_loop
)

IF DEFINED VULKAN_API_ENABLED (
    SET GFX_API_NAME=Vulkan
    SET GFX_API=VK
) ELSE (
    SET VULKAN_API_ENABLED=OFF
    SET GFX_API_NAME=DirectX 12
    SET GFX_API=DX
)

IF DEFINED WIN32_BUILD (
    SET ARCH_TYPE=Win32
) ELSE (
    SET ARCH_TYPE=x64
)

IF DEFINED DEBUG_BUILD (
    SET BUILD_TYPE=Debug
) ELSE (
    SET BUILD_TYPE=Release
)

IF DEFINED USE_VS2019 (
    SET CMAKE_GENERATOR=Visual Studio 16 2019
) ELSE (
    SET CMAKE_GENERATOR=Visual Studio 17 2022
)

SET CONFIG_DIR=%OUTPUT_DIR%\VisualStudio\%ARCH_TYPE%-MSVC-%GFX_API%-%BUILD_TYPE%-SLN
SET INSTALL_DIR=%CONFIG_DIR%\Install

SET CMAKE_FLAGS= ^
    -A %ARCH_TYPE% ^
    -DMETHANE_VERSION_MAJOR=%BUILD_VERSION_MAJOR% ^
    -DMETHANE_VERSION_MINOR=%BUILD_VERSION_MINOR% ^
    -DMETHANE_VERSION_PATCH=%BUILD_VERSION_PATCH% ^
    -DMETHANE_GFX_VULKAN_ENABLED:BOOL=%VULKAN_API_ENABLED% ^
    -DMETHANE_APPS_BUILD_ENABLED:BOOL=ON ^
    -DMETHANE_TESTS_BUILD_ENABLED:BOOL=ON ^
    -DMETHANE_UNITY_BUILD_ENABLED:BOOL=ON ^
    -DMETHANE_RHI_PIMPL_INLINE_ENABLED:BOOL=ON ^
    -DMETHANE_CHECKS_ENABLED:BOOL=ON ^
    -DMETHANE_SHADERS_CODEVIEW_ENABLED:BOOL=ON ^
    -DMETHANE_PRECOMPILED_HEADERS_ENABLED:BOOL=ON ^
    -DMETHANE_RUN_TESTS_DURING_BUILD:BOOL=OFF ^
    -DMETHANE_CODE_COVERAGE_ENABLED:BOOL=OFF ^
    -DMETHANE_COMMAND_DEBUG_GROUPS_ENABLED:BOOL=ON ^
    -DMETHANE_LOGGING_ENABLED:BOOL=OFF ^
    -DMETHANE_OPEN_IMAGE_IO_ENABLED:BOOL=OFF ^
    -DMETHANE_SCOPE_TIMERS_ENABLED:BOOL=OFF ^
    -DMETHANE_ITT_INSTRUMENTATION_ENABLED:BOOL=OFF ^
    -DMETHANE_ITT_METADATA_ENABLED:BOOL=OFF ^
    -DMETHANE_GPU_INSTRUMENTATION_ENABLED:BOOL=OFF ^
    -DMETHANE_TRACY_PROFILING_ENABLED:BOOL=OFF ^
    -DMETHANE_TRACY_PROFILING_ON_DEMAND:BOOL=OFF

IF DEFINED GRAPHVIZ_ENABLED (
    SET GRAPHVIZ_DIR=%CONFIG_DIR%\GraphViz
    SET GRAPHVIZ_DOT_DIR=!GRAPHVIZ_DIR!\dot
    SET GRAPHVIZ_IMG_DIR=!GRAPHVIZ_DIR!\img
    SET GRAPHVIZ_FILE=MethaneKit.dot
    SET GRAPHVIZ_DOT_EXE=dot.exe
    SET CMAKE_FLAGS=%CMAKE_FLAGS% --graphviz="!GRAPHVIZ_DOT_DIR!\!GRAPHVIZ_FILE!"
)

IF DEFINED ANALYZE_BUILD (

    SET BUILD_DIR=%CONFIG_DIR%\Analyze
    SET SONAR_TOKEN=%~2
    SET SONAR_SCANNER_VERSION="4.4.0.2170"
    SET SONAR_SCANNER_DIR=%OUTPUT_DIR%\SonarScanner
    SET SONAR_BUILD_WRAPPER_EXE=!SONAR_SCANNER_DIR!\build-wrapper-win-x86\build-wrapper-win-x86-64.exe
    SET SONAR_SCANNER_BAT=!SONAR_SCANNER_DIR!\sonar-scanner-!SONAR_SCANNER_VERSION!-windows\bin\sonar-scanner.bat

    ECHO =========================================================
    ECHO Code analysis for build Methane %GFX_API_NAME% %ARCH_TYPE% %BUILD_TYPE%
    ECHO =========================================================
    ECHO  * Build in: '!BUILD_DIR!'
    ECHO =========================================================

) ELSE (

    SET BUILD_DIR=%CONFIG_DIR%\Build

    ECHO =========================================================
    ECHO Clean build and install Methane %GFX_API_NAME% %ARCH_TYPE% %BUILD_TYPE%
    ECHO =========================================================
    ECHO  * Build in:   '!BUILD_DIR!'
    ECHO  * Install to: '%INSTALL_DIR%'
    IF DEFINED GRAPHVIZ_ENABLED (
        ECHO  * Graphviz in: '%GRAPHVIZ_DIR%'
    )
    ECHO =========================================================
)

RD /S /Q "%CONFIG_DIR%"
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

MKDIR "%BUILD_DIR%"
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

IF DEFINED ANALYZE_BUILD (

    IF NOT EXIST "%SONAR_SCANNER_DIR%" (
        ECHO Downloading and unpacking SonarScanner binaries...
        CALL powershell -ExecutionPolicy Bypass -Command "& '%START_DIR%\SonarDownload.ps1' '%SONAR_SCANNER_VERSION%' '%SONAR_SCANNER_DIR%'" 
        IF %ERRORLEVEL% NEQ 0 GOTO ERROR
    )

    SET GITBRANCH=
    FOR /F "tokens=* USEBACKQ" %%F IN (`git rev-parse --abbrev-ref HEAD`) DO (
        IF NOT "%%F" == "" SET GITBRANCH=%%F
    )
    IF %ERRORLEVEL% NEQ 0 GOTO ERROR

    ECHO ----------
    ECHO Analyzing code with Sonar Scanner on branch !GITBRANCH!...

    IF NOT EXIST "%BUILD_DIR%" MKDIR "%BUILD_DIR%"
    CD "%BUILD_DIR%"

    ECHO ----------
    ECHO Generating build files for %CMAKE_GENERATOR%...
    cmake -G "%CMAKE_GENERATOR%" %CMAKE_FLAGS% "%SOURCE_DIR%"
    IF %ERRORLEVEL% NEQ 0 GOTO ERROR

    ECHO ----------
    ECHO Building with %CMAKE_GENERATOR% and SonarScanner build wrapper...
    "%SONAR_BUILD_WRAPPER_EXE%" --out-dir "%BUILD_DIR%"^
         cmake --build . --config Debug --parallel
    IF %ERRORLEVEL% NEQ 0 GOTO ERROR

    ECHO ----------
    ECHO Analyzing build with SonarScanner and submitting results...
    CD "%SOURCE_DIR%"
    CALL "%SONAR_SCANNER_BAT%"^
        -D sonar.organization="methane-powered"^
        -D sonar.projectKey="methane-powered-kit-windows"^
        -D sonar.branch.name="!GITBRANCH!"^
        -D sonar.projectVersion="%BUILD_VERSION%"^
        -D sonar.projectBaseDir="%SOURCE_DIR%"^
        -D sonar.sources="Apps,Modules"^
        -D sonar.host.url="https://sonarcloud.io"^
        -D sonar.login="%SONAR_TOKEN%"^
        -D sonar.cfamily.build-wrapper-output="%BUILD_DIR%"
    IF %ERRORLEVEL% NEQ 0 GOTO ERROR

) ELSE (
    CD "%BUILD_DIR%"

    ECHO Generating build files for %CMAKE_GENERATOR%...
    cmake -G "%CMAKE_GENERATOR%" -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% %CMAKE_FLAGS% "%SOURCE_DIR%"
    IF %ERRORLEVEL% NEQ 0 GOTO ERROR

    IF DEFINED GRAPHVIZ_ENABLED (
        ECHO ----------
        ECHO Locating GraphViz dot converter...
        where %GRAPHVIZ_DOT_EXE%
        IF %ERRORLEVEL% EQU 0 (
            ECHO Converting GraphViz diagram to image...
            MKDIR "%GRAPHVIZ_IMG_DIR%"
            IF %ERRORLEVEL% NEQ 0 GOTO ERROR
            FOR %%f in ("%GRAPHVIZ_DOT_DIR%\*.*") do (
                ECHO Writing image "%GRAPHVIZ_IMG_DIR%\%%~nxf.png"
                "%GRAPHVIZ_DOT_EXE%" -Tpng "%%f" -o "%GRAPHVIZ_IMG_DIR%\%%~nxf.png"
                IF %ERRORLEVEL% NEQ 0 GOTO ERROR
            )
        ) ELSE (
            ECHO "GraphViz `dot` executable was not found. Skipping graph images generation."
        )
    )

    ECHO ----------
    ECHO Building with %CMAKE_GENERATOR%...
    cmake --build . --config %BUILD_TYPE% --target install --parallel
    IF %ERRORLEVEL% NEQ 0 GOTO ERROR

    ECHO ----------
    ECHO Running tests...
    ctest --build-config %BUILD_TYPE% --output-on-failure
    IF %ERRORLEVEL% NEQ 0 GOTO ERROR
)

GOTO STOP

:ERROR
ECHO Error occurred %ERRORLEVEL%. Script execution was stopped.
GOTO STOP

:STOP
CD "%START_DIR%"
ENDLOCAL
ECHO ON