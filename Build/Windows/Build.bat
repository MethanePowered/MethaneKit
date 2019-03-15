@REM To build execute 'Build.bat' from 'Visual Studio 2017 Developer Command Prompt'

@SET PLATFORM_TYPE=Win64
@SET CONFIG_TYPE=Release

@SET CONFIG_DIR=%~dp0..\Output\VisualStudio\%PLATFORM_TYPE%-%CONFIG_TYPE%
@SET BUILD_DIR=%CONFIG_DIR%\Build
@SET INSTALL_DIR=%CONFIG_DIR%\Install
@SET SOURCE_DIR=%~dp0..\..
@SET START_DIR=%cd%

@ECHO =========================================================
@ECHO Clean build and install Methane %PLATFORM_TYPE% %CONFIG_TYPE%
@ECHO =========================================================
@ECHO  * Build in:   '%BUILD_DIR%'
@ECHO  * Install to: '%INSTALL_DIR%'
@ECHO =========================================================

@RD /S /Q "%CONFIG_DIR%"
@MKDIR "%BUILD_DIR%"
@CD "%BUILD_DIR%"

@ECHO Initializing submodules and pulling latest changes
git submodule update --init --depth 1 --recursive "%SOURCE_DIR%"
git pull --recurse-submodules

cmake -G "Visual Studio 15 2017 %PLATFORM_TYPE%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% "%SOURCE_DIR%"
cmake --build . --config %CONFIG_TYPE% --target install
@CD "%START_DIR%"