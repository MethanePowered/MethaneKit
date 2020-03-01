@REM To build execute 'Build.bat' from 'Visual Studio Native Tools Command Prompt'
@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION

SET PLATFORM_TYPE=Win64
SET ARCH_TYPE=x64
SET BUILD_TYPE=Release
SET BUILD_VERSION=0.2
SET CMAKE_FLAGS= ^
    -DMETHANE_VERSION=%BUILD_VERSION% ^
    -DMETHANE_SHADERS_CODEVIEW_ENABLED:BOOL=ON ^
    -DMETHANE_ITT_INSTRUMENTATION_ENABLED:BOOL=ON ^
    -DMETHANE_SCOPE_TIMERS_ENABLED:BOOL=OFF ^
    -DMETHANE_RUN_TESTS_DURING_BUILD:BOOL=OFF ^
    -DMETHANE_USE_OPEN_IMAGE_IO:BOOL=OFF ^
    -DMETHANE_COMMAND_EXECUTION_LOGGING:BOOL=OFF

SET CONFIG_DIR=%~dp0..\Output\VisualStudio\%PLATFORM_TYPE%-%BUILD_TYPE%
SET INSTALL_DIR=%CONFIG_DIR%\Install
SET SOURCE_DIR=%~dp0..\..
SET START_DIR=%cd%

IF "%~1"=="--vs2019" (
SET CMAKE_GENERATOR=Visual Studio 16 2019
SET CMAKE_FLAGS=-A %ARCH_TYPE% %CMAKE_FLAGS%
) ELSE (
IF "%~2"=="--vs2019" (
SET CMAKE_GENERATOR=Visual Studio 16 2019
SET CMAKE_FLAGS=-A %ARCH_TYPE% %CMAKE_FLAGS% 
) ELSE (
SET CMAKE_GENERATOR=Visual Studio 15 2017 %PLATFORM_TYPE%
) )

IF "%~1"=="--analyze" (

    SET IS_ANALYZE_BUILD=1
    SET BUILD_DIR=%CONFIG_DIR%\Analyze
    SET SONAR_SCANNER_DIR=%SOURCE_DIR%\Externals\SonarScanner\binaries\Windows
    SET SONAR_SCANNER_ZIP=!SONAR_SCANNER_DIR!.zip
    SET SONAR_BUILD_WRAPPER_EXE=!SONAR_SCANNER_DIR!\build-wrapper-win-x86-64.exe
    SET SONAR_SCANNER_MSBUILD_EXE=!SONAR_SCANNER_DIR!\SonarScanner.MSBuild.exe

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

ECHO Pulling latest changes from submodules...
git submodule update --init --recursive
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

RD /S /Q "%BUILD_DIR%"
MKDIR "%BUILD_DIR%"

ECHO ---

IF %IS_ANALYZE_BUILD% EQU 1 (
    ECHO Unpacking Sonar Scanner binaries...
    IF EXIST "%SONAR_SCANNER_DIR%" RD /S /Q "%SONAR_SCANNER_DIR%"
    CALL powershell -Command "Expand-Archive %SONAR_SCANNER_ZIP% -DestinationPath %SONAR_SCANNER_DIR%"
    IF %ERRORLEVEL% NEQ 0 GOTO ERROR

    SET GITBRANCH=
    FOR /F "tokens=* USEBACKQ" %%F IN (`git rev-parse --abbrev-ref HEAD`) DO (
        IF NOT "%%F" == "" SET GITBRANCH=%%F
    )
    IF %ERRORLEVEL% NEQ 0 GOTO ERROR

    ECHO ---
    ECHO Analyzing code with Sonar Scanner on branch !GITBRANCH!...

    IF NOT EXIST "%BUILD_DIR%" MKDIR "%BUILD_DIR%"
    CD "%BUILD_DIR%"

    "%SONAR_BUILD_WRAPPER_EXE%" --out-dir "%BUILD_DIR%"^
         cmake -G "%CMAKE_GENERATOR%" %CMAKE_FLAGS% "%SOURCE_DIR%"
    IF %ERRORLEVEL% NEQ 0 GOTO ERROR

    CD "%SOURCE_DIR%"
    "%SONAR_SCANNER_MSBUILD_EXE%" begin^
        /k:egorodet_MethaneKit^
        /o:egorodet-github^
        /v:%BUILD_VERSION%^
        /d:sonar.branch.name="!GITBRANCH!"^
        /d:sonar.sources="%SOURCE_DIR%"^
        /d:sonar.projectBaseDir="%SOURCE_DIR%"^
        /d:sonar.cfamily.build-wrapper-output="%BUILD_DIR%"^
        /d:sonar.host.url="https://sonarcloud.io"^
        /d:sonar.login=6e1dbce6af614f59d75f1d78f0609aaaa60caee1
    IF %ERRORLEVEL% NEQ 0 GOTO ERROR

    MSBuild.exe "%BUILD_DIR%\MethaneKit.sln" /t:Rebuild
    IF %ERRORLEVEL% NEQ 0 GOTO ERROR

    "%SONAR_SCANNER_MSBUILD_EXE%" end^
        /d:sonar.login=6e1dbce6af614f59d75f1d78f0609aaaa60caee1
    IF %ERRORLEVEL% NEQ 0 GOTO ERROR

) ELSE (
    CD "%BUILD_DIR%"

    ECHO Building with %CMAKE_GENERATOR%...

    cmake -G "%CMAKE_GENERATOR%" -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% %CMAKE_FLAGS% "%SOURCE_DIR%"
    IF %ERRORLEVEL% NEQ 0 GOTO ERROR

    cmake --build . --config %BUILD_TYPE% --target install
    IF %ERRORLEVEL% NEQ 0 GOTO ERROR

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