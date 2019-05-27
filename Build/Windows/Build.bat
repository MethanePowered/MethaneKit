@REM To build execute 'Build.bat' from 'Visual Studio 2017 Developer Command Prompt'
@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION

SET PLATFORM_TYPE=Win64
SET CONFIG_TYPE=Release

SET CONFIG_DIR=%~dp0..\Output\VisualStudio\%PLATFORM_TYPE%-%CONFIG_TYPE%
SET INSTALL_DIR=%CONFIG_DIR%\Install
SET SOURCE_DIR=%~dp0..\..
SET START_DIR=%cd%

IF "%~1"=="--analyze" (

    SET IS_ANALYZE_BUILD=1
    SET BUILD_DIR=%CONFIG_DIR%\Analyze

    ECHO =========================================================
    ECHO Code analysis for build Methane %PLATFORM_TYPE% %CONFIG_TYPE%
    ECHO =========================================================
    ECHO  * Build in: '!BUILD_DIR!'
    ECHO =========================================================

) ELSE (

    SET IS_ANALYZE_BUILD=0
    SET BUILD_DIR=%CONFIG_DIR%\Build

    ECHO =========================================================
    ECHO Clean build and install Methane %PLATFORM_TYPE% %CONFIG_TYPE%
    ECHO =========================================================
    ECHO  * Build in:   '!BUILD_DIR!'
    ECHO  * Install to: '%INSTALL_DIR%'
    ECHO =========================================================
)

ECHO Pulling latest changes with submodules...
git pull --recurse-submodules

ECHO ---

IF %IS_ANALYZE_BUILD% EQU 1 (

ECHO Analyzing code with Sonar Scanner http://www.sonarcloud.io...
ECHO ON

@CD "%BUILD_DIR%"
cmake -G "Visual Studio 15 2017 %PLATFORM_TYPE%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% "%SOURCE_DIR%"
@IF %ERRORLEVEL% NEQ 0 GOTO STOP
@CD "%SOURCE_DIR%"

SonarScanner.MSBuild.exe begin^
    /k:egorodet_MethaneKit^
    /o:egorodet-github^
    /d:sonar.sources="%SOURCE_DIR%"^
    /d:sonar.projectBaseDir="%SOURCE_DIR%"^
    /d:sonar.host.url="https://sonarcloud.io"^
    /d:sonar.login=6e1dbce6af614f59d75f1d78f0609aaaa60caee1"
@IF %ERRORLEVEL% NEQ 0 GOTO STOP

MSBuild.exe "%BUILD_DIR%\MethaneKit.sln" /t:Rebuild
@IF %ERRORLEVEL% NEQ 0 GOTO STOP

SonarScanner.MSBuild.exe end^
    /d:sonar.login=6e1dbce6af614f59d75f1d78f0609aaaa60caee1

) ELSE (

RD /S /Q "%CONFIG_DIR%"
MKDIR "%BUILD_DIR%"
CD "%BUILD_DIR%"

ECHO Building with Visual Studio 2017...
ECHO ON

cmake -G "Visual Studio 15 2017 %PLATFORM_TYPE%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% "%SOURCE_DIR%"
@IF %ERRORLEVEL% NEQ 0 GOTO STOP

cmake --build . --config %CONFIG_TYPE% --target install
@IF %ERRORLEVEL% NEQ 0 GOTO STOP

)

:STOP
@CD "%START_DIR%"