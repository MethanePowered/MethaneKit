@REM To build execute 'Build.bat' from 'Visual Studio Native Tools Command Prompt'
@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION

SET PLATFORM_TYPE=Win64
SET ARCH_TYPE=x64
SET BUILD_TYPE=Release
SET BUILD_VERSION=0.5
SET CMAKE_FLAGS= ^
    -DMETHANE_SHADERS_CODEVIEW_ENABLED:BOOL=ON ^
    -DMETHANE_RUN_TESTS_DURING_BUILD:BOOL=OFF ^
    -DMETHANE_COMMAND_DEBUG_GROUPS_ENABLED:BOOL=ON ^
    -DMETHANE_LOGGING_ENABLED:BOOL=OFF ^
    -DMETHANE_USE_OPEN_IMAGE_IO:BOOL=OFF ^
    -DMETHANE_SCOPE_TIMERS_ENABLED:BOOL=OFF ^
    -DMETHANE_ITT_INSTRUMENTATION_ENABLED:BOOL=ON ^
    -DMETHANE_ITT_METADATA_ENABLED:BOOL=OFF ^
    -DMETHANE_GPU_INSTRUMENTATION_ENABLED:BOOL=OFF ^
    -DMETHANE_TRACY_PROFILING_ENABLED:BOOL=OFF ^
    -DMETHANE_TRACY_PROFILING_ON_DEMAND:BOOL=OFF

SET OUTPUT_DIR=%~dp0..\Output
SET CONFIG_DIR=%OUTPUT_DIR%\VisualStudio\%PLATFORM_TYPE%-%BUILD_TYPE%
SET INSTALL_DIR=%CONFIG_DIR%\Install
SET SOURCE_DIR=%~dp0..\..
SET START_DIR=%cd%

SET GRAPHVIZ_DIR=%CONFIG_DIR%\GraphViz
SET GRAPHVIZ_DOT_DIR=%GRAPHVIZ_DIR%\dot
SET GRAPHVIZ_IMG_DIR=%GRAPHVIZ_DIR%\img
SET GRAPHVIZ_FILE=MethaneKit.dot
SET GRAPHVIZ_DOT_EXE=dot.exe

IF "%~1"=="--vs2017" SET USE_VS2017=1
IF "%~2"=="--vs2017" SET USE_VS2017=1

IF DEFINED USE_VS2017 (
    SET CMAKE_GENERATOR=Visual Studio 15 2017 %PLATFORM_TYPE%
) ELSE (
    SET CMAKE_GENERATOR=Visual Studio 16 2019
    SET CMAKE_FLAGS=-A %ARCH_TYPE% %CMAKE_FLAGS%
)

IF "%~1"=="--analyze" (

    SET IS_ANALYZE_BUILD=1
    SET BUILD_DIR=%CONFIG_DIR%\Analyze
    SET SONAR_TOKEN=%~2
    SET SONAR_SCANNER_VERSION="4.4.0.2170"
    SET SONAR_SCANNER_DIR=%OUTPUT_DIR%\SonarScanner
    SET SONAR_BUILD_WRAPPER_EXE=!SONAR_SCANNER_DIR!\build-wrapper-win-x86\build-wrapper-win-x86-64.exe
    SET SONAR_SCANNER_BAT=!SONAR_SCANNER_DIR!\sonar-scanner-!SONAR_SCANNER_VERSION!-windows\bin\sonar-scanner.bat

    ECHO =========================================================
    ECHO Code analysis for build Methane %PLATFORM_TYPE% %BUILD_TYPE%
    ECHO =========================================================
    ECHO  * Build in: '!BUILD_DIR!'
    ECHO =========================================================

) ELSE (

    SET IS_ANALYZE_BUILD=0
    SET BUILD_DIR=%CONFIG_DIR%\Build

    ECHO =========================================================
    ECHO Clean build and install Methane %PLATFORM_TYPE% %BUILD_TYPE%
    ECHO =========================================================
    ECHO  * Build in:   '!BUILD_DIR!'
    ECHO  * Install to: '%INSTALL_DIR%'
    ECHO =========================================================
)

RD /S /Q "%CONFIG_DIR%"
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

MKDIR "%BUILD_DIR%"
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

IF %IS_ANALYZE_BUILD% EQU 1 (

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
         cmake --build . --config Debug
    IF %ERRORLEVEL% NEQ 0 GOTO ERROR

    ECHO ----------
    ECHO Analyzing build with SonarScanner and submitting results...
    CD "%SOURCE_DIR%"
    CALL "%SONAR_SCANNER_BAT%"^
        -D sonar.organization="egorodet-github"^
        -D sonar.projectKey="egorodet_MethaneKit_Windows"^
        -D sonar.branch.name="!GITBRANCH!"^
        -D sonar.projectVersion="%BUILD_VERSION%"^
        -D sonar.projectBaseDir="%SOURCE_DIR%"^
        -D sonar.sources="Apps,Modules"^
        -D sonar.host.url="https://sonarcloud.io"^
        -D sonar.login="%SONAR_TOKEN%"^
        -D sonar.cfamily.build-wrapper-output="%BUILD_DIR%"^
        -D sonar.cfamily.cache.path="%SONAR_SCANNER_DIR%\Cache"^
        -D sonar.cfamily.threads=16^
        -D sonar.cfamily.cache.enabled=true
    IF %ERRORLEVEL% NEQ 0 GOTO ERROR

) ELSE (
    CD "%BUILD_DIR%"

    ECHO Generating build files for %CMAKE_GENERATOR%...
    cmake -G "%CMAKE_GENERATOR%" --graphviz="%GRAPHVIZ_DOT_DIR%\%GRAPHVIZ_FILE%" -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% %CMAKE_FLAGS% "%SOURCE_DIR%"
    IF %ERRORLEVEL% NEQ 0 GOTO ERROR

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

    ECHO ----------
    ECHO Building with %CMAKE_GENERATOR%...
    cmake --build . --config %BUILD_TYPE% --target install
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